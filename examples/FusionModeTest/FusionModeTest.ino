/*
  FusionModeTest.ino

  Purpose:
    Validate packet mode transitions for multi-sensor auto encode:
      1) FULL
      2) DIFF
      3) HEART

  Expected command sequence:
    FULL (63), DIFF (170), HEART (127)

  Suggested audience:
    Users validating fusion state transitions and low-stack verification
    on MCU targets.
*/

#include <OSynapticFX.h>
#include <string.h>
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

static osfx_fusion_state g_tx_state;
static uint8_t g_packet[512];

static const char* cmd_name(uint8_t cmd) {
  if (cmd == OSFX_CMD_DATA_FULL) {
    return "FULL";
  }
  if (cmd == OSFX_CMD_DATA_DIFF) {
    return "DIFF";
  }
  if (cmd == OSFX_CMD_DATA_HEART) {
    return "HEART";
  }
  return "UNKNOWN";
}

static bool decode_meta_and_check(int packet_len, uint8_t expected_cmd) {
  osfx_packet_meta meta;
  memset(&meta, 0, sizeof(meta));
  int ok = osfx_packet_decode_meta(g_packet, (size_t)packet_len, &meta);

  Serial.print("meta decode ok=");
  Serial.print(ok);
  Serial.print(", meta.cmd=");
  Serial.print(cmd_name(meta.cmd));
  Serial.print(" (");
  Serial.print((unsigned int)meta.cmd);
  Serial.print(")");
  Serial.print(", body_len=");
  Serial.print((unsigned long)meta.body_len);
  Serial.print(", crc8_ok=");
  Serial.print(meta.crc8_ok);
  Serial.print(", crc16_ok=");
  Serial.println(meta.crc16_ok);

  if (!ok) {
    Serial.println("FAIL: meta decode returned 0");
    return false;
  }
  if (meta.cmd != expected_cmd) {
    Serial.print("FAIL: meta cmd mismatch, expected ");
    Serial.print(cmd_name(expected_cmd));
    Serial.print(", got ");
    Serial.println(cmd_name(meta.cmd));
    return false;
  }
  if (!meta.crc8_ok || !meta.crc16_ok) {
    Serial.println("FAIL: packet CRC check failed");
    return false;
  }
  return true;
}

static bool run_case(const char* tag, uint64_t ts, double hum_value, uint8_t expected_cmd) {
  osfx_core_sensor_input sensors[2];

  uint8_t cmd = 0;
  int packet_len = 0;
  int ok = 0;

  sensors[0].sensor_id = "TEMP";
  sensors[0].sensor_state = "OK";
  sensors[0].value = 1.0;        // 1.0 kPa
  sensors[0].unit = "kPa";
  sensors[0].geohash_id = "";
  sensors[0].supplementary_message = "";
  sensors[0].resource_url = "";

  sensors[1].sensor_id = "HUM";
  sensors[1].sensor_state = "OK";
  sensors[1].value = hum_value;
  sensors[1].unit = "%";
  sensors[1].geohash_id = "";
  sensors[1].supplementary_message = "";
  sensors[1].resource_url = "";

  ok = osfx_core_encode_multi_sensor_packet_auto(
      &g_tx_state,
      42U,
      2U,
      ts,
      "NODE_A",
      "ONLINE",
      sensors,
      2U,
      g_packet,
      sizeof(g_packet),
      &packet_len,
      &cmd);

  Serial.println("----------------------------------------");
  Serial.print("[");
  Serial.print(tag);
  Serial.println("]");
  Serial.print("encode ok=");
  Serial.print(ok);
  Serial.print(", packet_len=");
  Serial.print(packet_len);
  Serial.print(", cmd=");
  Serial.print(cmd_name(cmd));
  Serial.print(" (");
  Serial.print((unsigned int)cmd);
  Serial.println(")");

  if (!ok) {
    Serial.println("FAIL: encode returned 0");
    return false;
  }

  if (cmd != expected_cmd) {
    Serial.print("FAIL: expected cmd ");
    Serial.print(cmd_name(expected_cmd));
    Serial.print(" but got ");
    Serial.println(cmd_name(cmd));
    return false;
  }

  if (!decode_meta_and_check(packet_len, expected_cmd)) {
    return false;
  }

  Serial.println("PASS");
  return true;
}

static void print_final_result(bool all_ok) {
  Serial.println("========================================");
  if (all_ok) {
    Serial.println("RESULT: ALL TESTS PASSED");
  } else {
    Serial.println("RESULT: TEST FAILED");
  }
  Serial.println("========================================");
}

static void run_all_cases_once(void) {
  bool all_ok = true;

  osfx_fusion_state_init(&g_tx_state);

  all_ok &= run_case("CASE-1", 1710000010ULL, 55.0, OSFX_CMD_DATA_FULL);
  all_ok &= run_case("CASE-2", 1710000011ULL, 56.0, OSFX_CMD_DATA_DIFF);
  all_ok &= run_case("CASE-3", 1710000012ULL, 56.0, OSFX_CMD_DATA_HEART);

  print_final_result(all_ok);
}

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
static void fusion_test_task(void* param) {
  (void)param;
  UBaseType_t watermark_before = uxTaskGetStackHighWaterMark(NULL);
  Serial.print("stack high-watermark before=");
  Serial.println((unsigned long)watermark_before);
  run_all_cases_once();
  UBaseType_t watermark_after = uxTaskGetStackHighWaterMark(NULL);
  Serial.print("stack high-watermark after=");
  Serial.println((unsigned long)watermark_after);
  vTaskDelete(NULL);
}
#endif

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }
  delay(300);

  Serial.println();
  Serial.println("========================================");
  Serial.println("OSynaptic-FX Fusion Mode Test");
  Serial.println("Expect: FULL -> DIFF -> HEART");
  Serial.println("Low-stack mode: verify via packet meta only");
  Serial.println("========================================");

#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
  // osfx multi-sensor encode path uses large stack frames in C core.
  // Run it in a dedicated task with a larger stack than Arduino loopTask.
  BaseType_t created = xTaskCreatePinnedToCore(
      fusion_test_task,
      "osfx-fusion-test",
      24576,
      NULL,
      1,
      NULL,
      tskNO_AFFINITY);
  if (created != pdPASS) {
    Serial.println("FAIL: cannot create fusion_test_task");
    print_final_result(false);
  } else {
    Serial.println("fusion_test_task started (stack=24576)");
  }
#else
  run_all_cases_once();
#endif
}

void loop() {
  delay(1000);
}
