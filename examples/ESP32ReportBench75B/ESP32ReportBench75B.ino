/*
 * ESP32ReportBench75B
 *
 * Report-friendly throughput benchmark for OSynaptic-FX packet encoding.
 *
 * Design goals:
 * - Internal tight loop only; no per-iteration serial output.
 * - One summary block printed at the end for screenshot / appendix use.
 * - Report both single-core and dual-core aggregated throughput.
 * - Fixed 59-byte payload, which produces a 75-byte FULL frame
 *   (16-byte protocol overhead + 59-byte body).
 *
 * Usage:
 * 1. Select an ESP32 board in Arduino IDE.
 * 2. Upload this sketch.
 * 3. Open Serial Monitor at 115200 baud.
 * 4. Wait for the final summary block.
 */

#if !defined(ARDUINO_ARCH_ESP32)
#error This sketch is intended for ESP32 boards only.
#endif

#include <OSynapticFX.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SINGLE_CORE_ITERATIONS 200000UL
#define DUAL_CORE_ITERATIONS_PER_CORE 100000UL
#define WARMUP_ITERATIONS 5000UL
#define FRAME_BODY_BYTES 59U
#define PACKET_BUFFER_CAP 128U
#define DUAL_CORE_YIELD_INTERVAL 500UL
#define DUAL_CORE_TASK_STACK 4096U

static uint8_t g_body[FRAME_BODY_BYTES];
static uint8_t g_packet_single[PACKET_BUFFER_CAP];
static uint8_t g_packet_core0[PACKET_BUFFER_CAP];
static uint8_t g_packet_core1[PACKET_BUFFER_CAP];
static volatile uint32_t g_sink_single = 0U;

typedef struct {
  uint8_t* packet_buf;
  volatile uint32_t sink;
  uint32_t ok_count;
  uint32_t error_count;
  uint64_t total_bytes;
  uint64_t elapsed_us;
  volatile uint8_t done;
  uint32_t iterations;
  uint32_t core_id;
  uint8_t tid_seed;
} dual_bench_result;

static dual_bench_result g_dual_core0;
static dual_bench_result g_dual_core1;

static void serial_print_u64(uint64_t value) {
  char buf[21];
  size_t pos = sizeof(buf) - 1U;

  buf[pos] = '\0';
  do {
    buf[--pos] = (char)('0' + (value % 10ULL));
    value /= 10ULL;
  } while (value > 0ULL);

  Serial.print(&buf[pos]);
}

static void print_kv_text(const char* key, const char* value) {
  Serial.print(key);
  Serial.print('=');
  Serial.println(value);
}

static void print_kv_u32(const char* key, uint32_t value) {
  Serial.print(key);
  Serial.print('=');
  Serial.println(value);
}

static void print_kv_u64(const char* key, uint64_t value) {
  Serial.print(key);
  Serial.print('=');
  serial_print_u64(value);
  Serial.println();
}

static void print_kv_i32(const char* key, int32_t value) {
  Serial.print(key);
  Serial.print('=');
  Serial.println(value);
}

static void print_kv_f64(const char* key, double value, int digits) {
  Serial.print(key);
  Serial.print('=');
  Serial.println(value, digits);
}

static void fill_body(void) {
  size_t i;
  for (i = 0; i < FRAME_BODY_BYTES; ++i) {
    g_body[i] = (uint8_t)((i * 37U + 11U) & 0xFFU);
  }
}

static int encode_once(uint8_t* packet_buf, volatile uint32_t* sink, uint8_t tid, uint64_t timestamp_raw) {
  int packet_len = osfx_packet_encode_full(
      0x3Fu,
      0x12345678u,
      tid,
      timestamp_raw,
      g_body,
      sizeof(g_body),
      packet_buf,
      PACKET_BUFFER_CAP);

  if (packet_len > 0 && sink != NULL) {
    *sink += (uint32_t)packet_buf[(size_t)packet_len - 1U];
  }

  return packet_len;
}

static void dual_core_task(void* param) {
  uint32_t i;
  uint64_t start_us;
  uint64_t ts_base;
  dual_bench_result* result = (dual_bench_result*)param;

  ts_base = 1710000000ULL + ((uint64_t)result->core_id * 1000000ULL);

  for (i = 0U; i < WARMUP_ITERATIONS; ++i) {
    (void)encode_once(
        result->packet_buf,
        &result->sink,
        (uint8_t)(result->tid_seed + (uint8_t)i),
        ts_base + (uint64_t)i);
  }

  start_us = (uint64_t)esp_timer_get_time();

  for (i = 0U; i < result->iterations; ++i) {
    int packet_len = encode_once(
        result->packet_buf,
        &result->sink,
        (uint8_t)(result->tid_seed + (uint8_t)i),
        ts_base + (uint64_t)WARMUP_ITERATIONS + (uint64_t)i);

    if (packet_len > 0) {
      ++result->ok_count;
      result->total_bytes += (uint64_t)packet_len;
    } else {
      ++result->error_count;
    }

    if (((i + 1U) % DUAL_CORE_YIELD_INTERVAL) == 0U) {
      vTaskDelay(1);
    }
  }

  result->elapsed_us = (uint64_t)esp_timer_get_time() - start_us;
  result->done = 1U;
  vTaskDelete(NULL);
}

static void run_dual_core_bench(uint64_t* out_wall_elapsed_us,
                                double* out_pps,
                                double* out_mbps_decimal,
                                uint32_t* out_total_ok,
                                uint32_t* out_total_error,
                                uint64_t* out_total_bytes,
                                uint32_t* out_combined_sink) {
  uint64_t start_us;
  uint64_t wall_elapsed_us;
  uint32_t total_ok;
  uint32_t total_error;
  uint64_t total_bytes;
  uint32_t combined_sink;
  double elapsed_s;

  memset(&g_dual_core0, 0, sizeof(g_dual_core0));
  memset(&g_dual_core1, 0, sizeof(g_dual_core1));

  g_dual_core0.packet_buf = g_packet_core0;
  g_dual_core0.iterations = DUAL_CORE_ITERATIONS_PER_CORE;
  g_dual_core0.core_id = 0U;
  g_dual_core0.tid_seed = 0U;

  g_dual_core1.packet_buf = g_packet_core1;
  g_dual_core1.iterations = DUAL_CORE_ITERATIONS_PER_CORE;
  g_dual_core1.core_id = 1U;
  g_dual_core1.tid_seed = 97U;

  start_us = (uint64_t)esp_timer_get_time();

  if (xTaskCreatePinnedToCore(dual_core_task, "osfx_bench0", DUAL_CORE_TASK_STACK,
                              (void*)&g_dual_core0, 1, NULL, 0) != pdPASS) {
    *out_wall_elapsed_us = 0ULL;
    *out_pps = 0.0;
    *out_mbps_decimal = 0.0;
    *out_total_ok = 0U;
    *out_total_error = 1U;
    *out_total_bytes = 0ULL;
    *out_combined_sink = 0U;
    return;
  }

  if (xTaskCreatePinnedToCore(dual_core_task, "osfx_bench1", DUAL_CORE_TASK_STACK,
                              (void*)&g_dual_core1, 1, NULL, 1) != pdPASS) {
    *out_wall_elapsed_us = 0ULL;
    *out_pps = 0.0;
    *out_mbps_decimal = 0.0;
    *out_total_ok = 0U;
    *out_total_error = 1U;
    *out_total_bytes = 0ULL;
    *out_combined_sink = 0U;
    return;
  }

  while (g_dual_core0.done == 0U || g_dual_core1.done == 0U) {
    delay(1);
  }

  wall_elapsed_us = (uint64_t)esp_timer_get_time() - start_us;
  total_ok = g_dual_core0.ok_count + g_dual_core1.ok_count;
  total_error = g_dual_core0.error_count + g_dual_core1.error_count;
  total_bytes = g_dual_core0.total_bytes + g_dual_core1.total_bytes;
  combined_sink = g_dual_core0.sink + g_dual_core1.sink;
  elapsed_s = (wall_elapsed_us > 0ULL) ? ((double)wall_elapsed_us / 1000000.0) : 0.0;

  *out_wall_elapsed_us = wall_elapsed_us;
  *out_pps = (elapsed_s > 0.0) ? ((double)total_ok / elapsed_s) : 0.0;
  *out_mbps_decimal = (elapsed_s > 0.0) ? ((double)total_bytes / elapsed_s / 1000000.0) : 0.0;
  *out_total_ok = total_ok;
  *out_total_error = total_error;
  *out_total_bytes = total_bytes;
  *out_combined_sink = combined_sink;
}

void setup() {
  uint32_t i;
  uint32_t ok_count = 0U;
  uint32_t error_count = 0U;
  uint64_t total_bytes = 0ULL;
  uint64_t dual_wall_elapsed_us = 0ULL;
  uint64_t dual_total_bytes = 0ULL;
  uint64_t elapsed_us;
  uint64_t start_us;
  uint64_t ts_base = 1710000000ULL;
  int first_len;
  int last_len = 0;
  uint32_t dual_total_ok = 0U;
  uint32_t dual_total_error = 0U;
  uint32_t dual_combined_sink = 0U;
  double elapsed_s;
  double pps;
  double avg_us_per_frame;
  double mbps_decimal;
  double dual_pps = 0.0;
  double dual_mbps_decimal = 0.0;

  Serial.begin(115200);
  delay(1500);

  fill_body();

  first_len = encode_once(g_packet_single, &g_sink_single, 0U, ts_base);
  if (first_len <= 0) {
    Serial.println("BENCH_ERROR: initial encode failed");
    return;
  }

  for (i = 0U; i < WARMUP_ITERATIONS; ++i) {
    (void)encode_once(g_packet_single, &g_sink_single, (uint8_t)(i & 0xFFU), ts_base + (uint64_t)i);
  }

  start_us = (uint64_t)esp_timer_get_time();

  for (i = 0U; i < SINGLE_CORE_ITERATIONS; ++i) {
    last_len = encode_once(g_packet_single, &g_sink_single, (uint8_t)(i & 0xFFU), ts_base + (uint64_t)WARMUP_ITERATIONS + (uint64_t)i);
    if (last_len > 0) {
      ++ok_count;
      total_bytes += (uint64_t)last_len;
    } else {
      ++error_count;
    }
  }

  elapsed_us = (uint64_t)esp_timer_get_time() - start_us;
  elapsed_s = (elapsed_us > 0ULL) ? ((double)elapsed_us / 1000000.0) : 0.0;
  pps = (elapsed_s > 0.0) ? ((double)ok_count / elapsed_s) : 0.0;
  avg_us_per_frame = (ok_count > 0U) ? ((double)elapsed_us / (double)ok_count) : 0.0;
  mbps_decimal = (elapsed_s > 0.0) ? ((double)total_bytes / elapsed_s / 1000000.0) : 0.0;

  run_dual_core_bench(
      &dual_wall_elapsed_us,
      &dual_pps,
      &dual_mbps_decimal,
      &dual_total_ok,
      &dual_total_error,
      &dual_total_bytes,
      &dual_combined_sink);

  Serial.println();
  Serial.println("=== OSynaptic-FX ESP32 Report Benchmark ===");
  Serial.print("board=");
  Serial.println(ESP.getChipModel());
  print_kv_u32("cpu_mhz", (uint32_t)getCpuFrequencyMhz());
  print_kv_text("arduino_core", ESP.getSdkVersion());
  print_kv_u32("warmup_iterations", (uint32_t)WARMUP_ITERATIONS);
  print_kv_u32("body_bytes", (uint32_t)FRAME_BODY_BYTES);
  print_kv_i32("frame_bytes", first_len);
  print_kv_u32("single_core_iterations", (uint32_t)SINGLE_CORE_ITERATIONS);
  print_kv_u64("single_core_elapsed_us", elapsed_us);
  print_kv_u32("single_core_packets_ok", ok_count);
  print_kv_u32("single_core_packets_error", error_count);
  print_kv_f64("single_core_avg_us_per_frame", avg_us_per_frame, 3);
  print_kv_f64("single_core_pps", pps, 1);
  print_kv_f64("single_core_MBps_decimal", mbps_decimal, 3);
  print_kv_u64("single_core_bytes_total", total_bytes);
  print_kv_u32("single_core_sink_checksum", g_sink_single);
  print_kv_u32("dual_core_iterations_per_core", (uint32_t)DUAL_CORE_ITERATIONS_PER_CORE);
  print_kv_u32("dual_core_total_packets_ok", dual_total_ok);
  print_kv_u32("dual_core_total_packets_error", dual_total_error);
  print_kv_u64("dual_core_wall_elapsed_us", dual_wall_elapsed_us);
  print_kv_u64("dual_core_core0_elapsed_us", g_dual_core0.elapsed_us);
  print_kv_u64("dual_core_core1_elapsed_us", g_dual_core1.elapsed_us);
  print_kv_f64("dual_core_aggregate_pps", dual_pps, 1);
  print_kv_f64("dual_core_MBps_decimal", dual_mbps_decimal, 3);
  print_kv_u64("dual_core_bytes_total", dual_total_bytes);
  print_kv_u32("dual_core_sink_checksum", dual_combined_sink);
  print_kv_i32("final_frame_bytes", last_len);
  print_kv_text("note", "single_core and dual_core are measured with internal encode loops only; serial output occurs after timing");
  Serial.println("============================================");
  Serial.flush();
}

void loop() {
}