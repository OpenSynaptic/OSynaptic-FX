/*
  BootCliOrRun.ino

  Goal:
    - Boot window: 10 seconds for serial input "cli"
    - If entered, run lightweight OSynaptic CLI loop
    - Otherwise enter persistent sensor run mode

  Suggested audience:
    Advanced users who need runtime plugins, persistence and diagnostics
    in a single long-running sketch.

  Notes:
    - CLI capability depends on build flag OSFX_ENABLE_CLI.
    - Default package config enables OSFX_ENABLE_CLI and OSFX_ENABLE_FILE_IO.
*/

#include <OSynapticFX.h>
#include <osfx_storage_littlefs.h>
#include <string.h>
#include <stdlib.h>
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

static const unsigned long CLI_WINDOW_MS = 10000UL;
static const unsigned long SAMPLE_PERIOD_MS = 1000UL;
static const unsigned long ID_AUTOSAVE_PERIOD_MS = 10000UL;
static const unsigned long ID_SAVE_RETRY_BASE_MS = 3000UL;
static const uint8_t ID_SAVE_MAX_RETRY = 5U;

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
static const char* ID_STATE_PATH = "/osfx_id_state.csv";
#else
static const char* ID_STATE_PATH = "osfx_id_state.csv";
#endif

static osfx_fusion_state g_tx_state;
static osfx_protocol_matrix g_matrix;
static osfx_platform_runtime g_platform;
static osfx_id_allocator g_id_alloc;

static uint8_t g_packet[256];
static int g_packet_len = 0;
static uint8_t g_packet_cmd = 0;

static uint32_t g_local_aid = 42U;
static int g_runtime_ready = 0;
static int g_run_mode = 0;

static char g_line[256];
static size_t g_line_len = 0U;

static unsigned long g_last_emit_ms = 0UL;
static int g_id_dirty = 0;
static unsigned long g_last_id_save_ms = 0UL;
static unsigned long g_id_next_retry_ms = 0UL;
static uint8_t g_id_retry_attempt = 0U;
static uint32_t g_id_save_ok_count = 0U;
static uint32_t g_id_save_fail_count = 0U;
static uint32_t g_id_load_ok_count = 0U;
static uint32_t g_id_load_fail_count = 0U;
static int g_littlefs_ready = 0;

/* Time handling: use monotonic seconds plus a mutable unix offset. */
static int64_t g_unix_offset_seconds = 1710000000LL;

static void print_id_persist_status(void);
static int id_save_now(const char* reason);
static int id_load_now(const char* reason);
static void id_mark_dirty(void);
static void id_persist_tick(void);
static void storage_backend_init(void);

static const char* cmd_name(uint8_t cmd) {
  if (cmd == OSFX_CMD_DATA_FULL) return "FULL";
  if (cmd == OSFX_CMD_DATA_DIFF) return "DIFF";
  if (cmd == OSFX_CMD_DATA_HEART) return "HEART";
  return "UNKNOWN";
}

static size_t static_mem_estimate_bytes(void) {
  size_t bytes = 0U;
  bytes += sizeof(g_tx_state);
  bytes += sizeof(g_matrix);
  bytes += sizeof(g_platform);
  bytes += sizeof(g_id_alloc);
  bytes += sizeof(g_packet);
  bytes += sizeof(g_packet_len);
  bytes += sizeof(g_packet_cmd);
  bytes += sizeof(g_local_aid);
  bytes += sizeof(g_runtime_ready);
  bytes += sizeof(g_run_mode);
  bytes += sizeof(g_line);
  bytes += sizeof(g_line_len);
  bytes += sizeof(g_last_emit_ms);
  bytes += sizeof(g_unix_offset_seconds);
  return bytes;
}

static void print_memory_report(void) {
  size_t static_est = static_mem_estimate_bytes();

  Serial.print("mem.static_est_bytes=");
  Serial.println((unsigned long)static_est);
  Serial.print("mem.cfg.id_max_entries=");
  Serial.println((unsigned long)OSFX_ID_MAX_ENTRIES);
  Serial.print("mem.size.id_lease_entry_bytes=");
  Serial.println((unsigned long)sizeof(osfx_id_lease_entry));
  Serial.print("mem.size.id_allocator_bytes=");
  Serial.println((unsigned long)sizeof(g_id_alloc));
  Serial.print("mem.size.platform_runtime_bytes=");
  Serial.println((unsigned long)sizeof(g_platform));
  Serial.print("mem.size.fusion_state_bytes=");
  Serial.println((unsigned long)sizeof(g_tx_state));
  Serial.print("mem.size.protocol_matrix_bytes=");
  Serial.println((unsigned long)sizeof(g_matrix));
  Serial.print("mem.size.line_buffer_bytes=");
  Serial.println((unsigned long)sizeof(g_line));
  Serial.print("mem.size.packet_buffer_bytes=");
  Serial.println((unsigned long)sizeof(g_packet));
  print_id_persist_status();

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  {
    size_t heap_free = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t heap_min = heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
    size_t heap_largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    UBaseType_t stack_hwm_words = uxTaskGetStackHighWaterMark(NULL);
    size_t stack_hwm_bytes = ((size_t)stack_hwm_words) * sizeof(StackType_t);

    Serial.print("mem.heap_free_bytes=");
    Serial.println((unsigned long)heap_free);
    Serial.print("mem.heap_min_free_bytes=");
    Serial.println((unsigned long)heap_min);
    Serial.print("mem.heap_largest_block_bytes=");
    Serial.println((unsigned long)heap_largest);
    Serial.print("mem.stack_hwm_bytes=");
    Serial.println((unsigned long)stack_hwm_bytes);
  }
#else
  Serial.println("mem.heap_metrics=unsupported_on_this_board");
#endif
}

static uint64_t now_unix_seconds(void) {
  uint64_t mono_s = (uint64_t)(millis() / 1000UL);
  int64_t ts = (int64_t)mono_s + g_unix_offset_seconds;
  if (ts < 0) {
    return 0U;
  }
  return (uint64_t)ts;
}

static void set_unix_seconds(uint64_t unix_ts) {
  uint64_t mono_s = (uint64_t)(millis() / 1000UL);
  g_unix_offset_seconds = (int64_t)unix_ts - (int64_t)mono_s;
}

static int time_reached(unsigned long now_ms, unsigned long target_ms) {
  return (long)(now_ms - target_ms) >= 0;
}

static void id_mark_dirty(void) {
  g_id_dirty = 1;
}

static void storage_backend_init(void) {
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  g_littlefs_ready = osfx_storage_use_littlefs();
#else
  g_littlefs_ready = 0;
#endif

  Serial.print("storage.backend=");
  if (g_littlefs_ready) {
    Serial.println("littlefs");
  } else {
    Serial.println("default");
  }
}

static void print_id_persist_status(void) {
  unsigned long now_ms = millis();
  unsigned long retry_in_ms = 0UL;

  if (g_id_retry_attempt > 0U && !time_reached(now_ms, g_id_next_retry_ms)) {
    retry_in_ms = g_id_next_retry_ms - now_ms;
  }

  Serial.print("id.persist.path=");
  Serial.println(ID_STATE_PATH);
  Serial.print("id.persist.backend=");
  Serial.println(g_littlefs_ready ? "littlefs" : "default");
  Serial.print("id.persist.dirty=");
  Serial.println(g_id_dirty ? 1 : 0);
  Serial.print("id.persist.save_ok=");
  Serial.println((unsigned long)g_id_save_ok_count);
  Serial.print("id.persist.save_fail=");
  Serial.println((unsigned long)g_id_save_fail_count);
  Serial.print("id.persist.load_ok=");
  Serial.println((unsigned long)g_id_load_ok_count);
  Serial.print("id.persist.load_fail=");
  Serial.println((unsigned long)g_id_load_fail_count);
  Serial.print("id.persist.retry_attempt=");
  Serial.println((unsigned int)g_id_retry_attempt);
  Serial.print("id.persist.retry_in_ms=");
  Serial.println((unsigned long)retry_in_ms);
}

static int id_save_now(const char* reason) {
  if (osfx_id_save(&g_id_alloc, ID_STATE_PATH)) {
    g_id_dirty = 0;
    g_last_id_save_ms = millis();
    g_id_retry_attempt = 0U;
    g_id_next_retry_ms = 0UL;
    g_id_save_ok_count++;
    Serial.print("id-save ok reason=");
    Serial.println(reason ? reason : "?");
    return 1;
  }

  g_id_save_fail_count++;
  if (g_id_retry_attempt < ID_SAVE_MAX_RETRY) {
    g_id_retry_attempt++;
  }
  g_id_next_retry_ms = millis() + ((unsigned long)g_id_retry_attempt * ID_SAVE_RETRY_BASE_MS);

  Serial.print("id-save fail reason=");
  Serial.print(reason ? reason : "?");
  Serial.print(" retry_attempt=");
  Serial.println((unsigned int)g_id_retry_attempt);
  return 0;
}

static int id_load_now(const char* reason) {
  if (osfx_id_load(&g_id_alloc, ID_STATE_PATH)) {
    g_id_load_ok_count++;
    g_id_dirty = 0;
    g_id_retry_attempt = 0U;
    g_id_next_retry_ms = 0UL;
    g_last_id_save_ms = millis();
    Serial.print("id-load ok reason=");
    Serial.println(reason ? reason : "?");
    return 1;
  }

  g_id_load_fail_count++;
  Serial.print("id-load miss/fail reason=");
  Serial.println(reason ? reason : "?");
  return 0;
}

static void id_persist_tick(void) {
  unsigned long now_ms = millis();

  if (!g_id_dirty) {
    return;
  }

  if (g_id_retry_attempt > 0U) {
    if (time_reached(now_ms, g_id_next_retry_ms)) {
      (void)id_save_now("retry");
    }
    return;
  }

  if ((now_ms - g_last_id_save_ms) >= ID_AUTOSAVE_PERIOD_MS) {
    (void)id_save_now("periodic");
  }
}

static int matrix_emit_cb(const char* protocol, const uint8_t* frame, size_t frame_len, void* user_ctx) {
  (void)user_ctx;
  (void)frame;
  Serial.print("[matrix emit] proto=");
  Serial.print(protocol ? protocol : "?");
  Serial.print(", len=");
  Serial.println((unsigned long)frame_len);
  return 1;
}

static int pf_emit_cb(const char* to_proto, uint16_t to_port, const uint8_t* payload, size_t payload_len, void* user_ctx) {
  (void)user_ctx;
  (void)payload;
  Serial.print("[pf emit] to=");
  Serial.print(to_proto ? to_proto : "?");
  Serial.print(":");
  Serial.print((unsigned int)to_port);
  Serial.print(", len=");
  Serial.println((unsigned long)payload_len);
  return 1;
}

static void runtime_init(void) {
  osfx_protocol_matrix_init(&g_matrix, matrix_emit_cb, NULL);
  if (!osfx_protocol_matrix_register_defaults(&g_matrix)) {
    Serial.println("runtime init failed: protocol matrix defaults");
    g_runtime_ready = 0;
    return;
  }

  osfx_platform_runtime_init(&g_platform, &g_matrix, pf_emit_cb, NULL);

#if OSFX_CFG_AUTOLOAD_TRANSPORT
  (void)osfx_platform_plugin_load(&g_platform, "transport", "");
#endif
#if OSFX_CFG_AUTOLOAD_TEST_PLUGIN
  (void)osfx_platform_plugin_load(&g_platform, "test_plugin", "");
#endif
#if OSFX_CFG_AUTOLOAD_PORT_FORWARDER
  (void)osfx_platform_plugin_load(&g_platform, "port_forwarder", "");
#endif

  g_runtime_ready = 1;
}

static int poll_line(char* out, size_t out_cap) {
  while (Serial.available() > 0) {
    int c = Serial.read();
    if (c < 0) {
      break;
    }

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      if (g_line_len >= out_cap) {
        g_line_len = 0U;
        return 0;
      }
      g_line[g_line_len] = '\0';
      strncpy(out, g_line, out_cap - 1U);
      out[out_cap - 1U] = '\0';
      g_line_len = 0U;
      return 1;
    }

    if (g_line_len + 1U < sizeof(g_line)) {
      g_line[g_line_len++] = (char)c;
    }
  }
  return 0;
}

static int split_argv(char* line, const char* argv[], int max_argv) {
  int argc = 0;
  char* tok = strtok(line, " ");
  while (tok && argc < max_argv) {
    argv[argc++] = tok;
    tok = strtok(NULL, " ");
  }
  return argc;
}

static int join_args_from_argv(int start, int argc, const char* argv[], char* out, size_t out_cap) {
  size_t off = 0U;
  int i;
  if (!out || out_cap == 0U) {
    return 0;
  }
  out[0] = '\0';
  for (i = start; i < argc; ++i) {
    size_t n = strlen(argv[i]);
    if (i > start) {
      if (off + 1U >= out_cap) {
        return 0;
      }
      out[off++] = ' ';
    }
    if (off + n + 1U > out_cap) {
      return 0;
    }
    memcpy(out + off, argv[i], n);
    off += n;
  }
  out[off] = '\0';
  return 1;
}

static void print_cli_help(void) {
  Serial.println("CLI help:");
  Serial.println("  help");
  Serial.println("  exit");
  Serial.println("  mem");
  Serial.println("  id-status");
  Serial.println("  storage-status");
  Serial.println("  save-id");
  Serial.println("  load-id");
  Serial.println("  show-time");
  Serial.println("  set-time <unix_s>");
  Serial.println("  alloc-id");
  Serial.println("  touch-id <aid>");
  Serial.println("  release-id <aid>");
  Serial.println("  send [sensor_id] [value] [unit]");
  Serial.println("  plugin-list");
  Serial.println("  plugin-load <name> [config]");
  Serial.println("  plugin-cmd <name> <cmd> [args...]");
  Serial.println("  transport-status");
  Serial.println("  test-plugin [cmd] [args...]");
  Serial.println("  port-forwarder [cmd] [args...]");
}

static int run_local_cli_cmd(int argc, const char* argv[]) {
  if (argc <= 0 || !argv || !argv[0]) {
    return 0;
  }

  if (strcmp(argv[0], "help") == 0) {
    print_cli_help();
    return 1;
  }

  if (strcmp(argv[0], "exit") == 0) {
    Serial.println("leave CLI, enter run mode");
    return -1;
  }

  if (strcmp(argv[0], "mem") == 0) {
    print_memory_report();
    return 1;
  }

  if (strcmp(argv[0], "id-status") == 0) {
    print_id_persist_status();
    return 1;
  }

  if (strcmp(argv[0], "storage-status") == 0) {
    Serial.print("storage.littlefs.ready=");
    Serial.println(g_littlefs_ready ? 1 : 0);
    Serial.print("storage.active=");
    Serial.println(g_littlefs_ready ? "littlefs" : "default");
    return 1;
  }

  if (strcmp(argv[0], "save-id") == 0) {
    Serial.println(id_save_now("cli") ? "ok" : "fail");
    return 1;
  }

  if (strcmp(argv[0], "load-id") == 0) {
    Serial.println(id_load_now("cli") ? "ok" : "fail");
    return 1;
  }

  if (strcmp(argv[0], "show-time") == 0) {
    Serial.print("unix_ts=");
    Serial.println((unsigned long)now_unix_seconds());
    return 1;
  }

  if (strcmp(argv[0], "set-time") == 0) {
    if (argc < 2) {
      Serial.println("usage: set-time <unix_s>");
      return 1;
    }
    uint64_t ts = (uint64_t)strtoull(argv[1], NULL, 10);
    set_unix_seconds(ts);
    Serial.print("ok unix_ts=");
    Serial.println((unsigned long)now_unix_seconds());
    return 1;
  }

  if (strcmp(argv[0], "alloc-id") == 0) {
    uint32_t aid = 0;
    if (osfx_id_allocate(&g_id_alloc, now_unix_seconds(), &aid)) {
      id_mark_dirty();
      Serial.print("allocated aid=");
      Serial.println((unsigned long)aid);
    } else {
      Serial.println("allocate failed");
    }
    return 1;
  }

  if (strcmp(argv[0], "touch-id") == 0) {
    if (argc < 2) {
      Serial.println("usage: touch-id <aid>");
      return 1;
    }
    uint32_t aid = (uint32_t)strtoul(argv[1], NULL, 10);
    if (osfx_id_touch(&g_id_alloc, aid, now_unix_seconds())) {
      id_mark_dirty();
      Serial.println("ok");
    } else {
      Serial.println("fail");
    }
    return 1;
  }

  if (strcmp(argv[0], "release-id") == 0) {
    if (argc < 2) {
      Serial.println("usage: release-id <aid>");
      return 1;
    }
    uint32_t aid = (uint32_t)strtoul(argv[1], NULL, 10);
    if (osfx_id_release(&g_id_alloc, aid)) {
      id_mark_dirty();
      Serial.println("ok");
    } else {
      Serial.println("fail");
    }
    return 1;
  }

  if (strcmp(argv[0], "send") == 0) {
    const char* sensor_id = "TEMP";
    const char* unit = "C";
    double value = 25.0;
    uint64_t ts;
    char* endptr = NULL;

    if (argc >= 2) {
      sensor_id = argv[1];
    }
    if (argc >= 3) {
      value = strtod(argv[2], &endptr);
      if (!endptr || *endptr != '\0') {
        Serial.println("usage: send [sensor_id] [value] [unit]");
        return 1;
      }
    }
    if (argc >= 4) {
      unit = argv[3];
    }

    ts = now_unix_seconds();
    if (!osfx_core_encode_sensor_packet_auto(
            &g_tx_state,
            g_local_aid,
            2U,
            ts,
            sensor_id,
            value,
            unit,
            g_packet,
            sizeof(g_packet),
            &g_packet_len,
            &g_packet_cmd)) {
      Serial.println("send encode failed");
      return 1;
    }

    Serial.print("send packet cmd=");
    Serial.print(cmd_name(g_packet_cmd));
    Serial.print(" (");
    Serial.print((unsigned int)g_packet_cmd);
    Serial.print(") len=");
    Serial.print(g_packet_len);
    Serial.print(" ts=");
    Serial.println((unsigned long)ts);

    if (g_runtime_ready) {
      char used_name[24];
      if (osfx_protocol_dispatch_auto(&g_matrix, g_packet, (size_t)g_packet_len, used_name, sizeof(used_name))) {
        Serial.print("send dispatched via=");
        Serial.println(used_name);
      } else {
        Serial.println("send dispatch failed");
      }
    }
    return 1;
  }

  return 0;
}

static int run_plugin_cli_bridge(int argc, const char* argv[]) {
  char out[768];
  if (argc <= 0 || !argv || !argv[0]) {
    return 0;
  }

  if (strcmp(argv[0], "plugin-list") == 0) {
    size_t n = osfx_platform_plugin_count(&g_platform);
    size_t i;
    int first = 1;
    Serial.print("plugins=");
    for (i = 0U; i < n; ++i) {
      char name[32];
      if (!osfx_platform_plugin_name_at(&g_platform, i, name, sizeof(name))) {
        continue;
      }
      if (!first) {
        Serial.print(",");
      }
      Serial.print(name);
      first = 0;
    }
    Serial.println();
    return 1;
  }

  if (strcmp(argv[0], "plugin-load") == 0) {
    const char* cfg = "";
    if (argc < 2) {
      Serial.println("err: error=usage plugin-load <name> [config]");
      return 1;
    }
    if (argc >= 3) {
      cfg = argv[2];
    }
    if (osfx_platform_plugin_load(&g_platform, argv[1], cfg)) {
      Serial.print("ok: ok=1 loaded=");
      Serial.println(argv[1]);
    } else {
      Serial.print("err: error=load_failed name=");
      Serial.println(argv[1]);
    }
    return 1;
  }

  if (strcmp(argv[0], "plugin-cmd") == 0) {
    char args[512];
    if (argc < 3) {
      Serial.println("err: error=usage plugin-cmd <name> <cmd> [args...]");
      return 1;
    }
    if (!join_args_from_argv(3, argc, argv, args, sizeof(args))) {
      Serial.println("err: error=args_too_long");
      return 1;
    }
    out[0] = '\0';
    if (osfx_platform_plugin_cmd(&g_platform, argv[1], argv[2], args, out, sizeof(out))) {
      Serial.print("ok: ");
    } else {
      Serial.print("err: ");
    }
    Serial.println(out);
    return 1;
  }

  if (strcmp(argv[0], "transport-status") == 0) {
    out[0] = '\0';
    if (osfx_platform_plugin_cmd(&g_platform, "transport", "status", "", out, sizeof(out))) {
      Serial.print("ok: ");
    } else {
      Serial.print("err: ");
    }
    Serial.println(out);
    return 1;
  }

  if (strcmp(argv[0], "test-plugin") == 0) {
    const char* cmd = (argc >= 2) ? argv[1] : "status";
    char args[512];
    args[0] = '\0';
    if (argc >= 3 && !join_args_from_argv(2, argc, argv, args, sizeof(args))) {
      Serial.println("err: error=args_too_long");
      return 1;
    }
    out[0] = '\0';
    if (osfx_platform_plugin_cmd(&g_platform, "test_plugin", cmd, args, out, sizeof(out))) {
      Serial.print("ok: ");
    } else {
      Serial.print("err: ");
    }
    Serial.println(out);
    return 1;
  }

  if (strcmp(argv[0], "port-forwarder") == 0) {
    const char* cmd = (argc >= 2) ? argv[1] : "status";
    char args[512];
    args[0] = '\0';
    if (argc >= 3 && !join_args_from_argv(2, argc, argv, args, sizeof(args))) {
      Serial.println("err: error=args_too_long");
      return 1;
    }
    out[0] = '\0';
    if (osfx_platform_plugin_cmd(&g_platform, "port_forwarder", cmd, args, out, sizeof(out))) {
      Serial.print("ok: ");
    } else {
      Serial.print("err: ");
    }
    Serial.println(out);
    return 1;
  }

  return 0;
}

static void cli_loop(void) {
  char line[256];

  Serial.println("CLI mode active. type 'help' for commands.");
  while (1) {
    id_persist_tick();

    if (!poll_line(line, sizeof(line))) {
      delay(5);
      continue;
    }

    if (line[0] == '\0') {
      continue;
    }

    {
      const char* argv[16];
      int argc;
      char work[256];
      char out[768];
      int handled;
      int ok;

      strncpy(work, line, sizeof(work) - 1U);
      work[sizeof(work) - 1U] = '\0';
      argc = split_argv(work, argv, 16);

      handled = run_local_cli_cmd(argc, argv);
      if (handled == -1) {
        g_run_mode = 1;
        return;
      }
      if (handled == 1) {
        continue;
      }

      if (!g_runtime_ready) {
        Serial.println("runtime unavailable");
        continue;
      }

      if (run_plugin_cli_bridge(argc, argv)) {
        continue;
      }

#if OSFX_ENABLE_CLI
      out[0] = '\0';
      ok = osfx_cli_lite_run(&g_platform, argc, argv, out, sizeof(out));
      Serial.print(ok ? "ok: " : "err: ");
      Serial.println(out);
#else
      (void)ok;
      out[0] = '\0';
      Serial.println("err: error=unknown_command");
#endif
    }
  }
}

static void run_tick(void) {
  uint64_t ts;
  double value;
  int ok;

  if (!g_run_mode) {
    return;
  }

  if ((millis() - g_last_emit_ms) < SAMPLE_PERIOD_MS) {
    return;
  }
  g_last_emit_ms = millis();

  ts = now_unix_seconds();
  value = 25.0 + (double)((millis() / 1000UL) % 5UL) * 0.1;

  ok = osfx_core_encode_sensor_packet_auto(
      &g_tx_state,
      g_local_aid,
      2U,
      ts,
      "TEMP",
      value,
      "C",
      g_packet,
      sizeof(g_packet),
      &g_packet_len,
      &g_packet_cmd);

  if (!ok) {
    Serial.println("run encode failed");
    return;
  }

  Serial.print("run packet cmd=");
  Serial.print(cmd_name(g_packet_cmd));
  Serial.print(" (");
  Serial.print((unsigned int)g_packet_cmd);
  Serial.print(") len=");
  Serial.print(g_packet_len);
  Serial.print(" ts=");
  Serial.println((unsigned long)ts);
}

void setup() {
  unsigned long start_ms;
  char line[64];

  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }
  delay(200);

  Serial.println();
  Serial.println("========================================");
  Serial.println("OSynaptic-FX Boot CLI / Run Demo");
  Serial.println("type 'cli' within 10s to enter CLI mode");
  Serial.println("========================================");

  osfx_fusion_state_init(&g_tx_state);
  storage_backend_init();

  osfx_id_allocator_init(&g_id_alloc, 100U, 10000U, 86400U);
  (void)id_load_now("boot");
  if (osfx_id_allocate(&g_id_alloc, now_unix_seconds(), &g_local_aid)) {
    id_mark_dirty();
    Serial.print("local aid allocated=");
    Serial.println((unsigned long)g_local_aid);
  } else {
    Serial.println("aid allocate failed, fallback aid=42");
    g_local_aid = 42U;
  }

  runtime_init();
  print_memory_report();

  start_ms = millis();
  while ((millis() - start_ms) < CLI_WINDOW_MS) {
    id_persist_tick();
    if (poll_line(line, sizeof(line))) {
      if (strcmp(line, "cli") == 0) {
        cli_loop();
        break;
      }
      Serial.println("input ignored during boot window, type exactly: cli");
    }
    delay(5);
  }

  if (!g_run_mode) {
    g_run_mode = 1;
    Serial.println("enter persistent run mode");
  }
}

void loop() {
  id_persist_tick();
  run_tick();
}
