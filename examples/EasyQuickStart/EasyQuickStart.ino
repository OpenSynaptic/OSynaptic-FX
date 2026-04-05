/*
  EasyQuickStart.ino

  Goal:
    Show minimal Arduino-side usage through osfx_easy API.

  Suggested audience:
    First-time Arduino users of OSynaptic-FX.
    This is the recommended starting sketch before advanced examples.

  Steps:
    1) init easy context
    2) init/allocate AID
    3) encode multi-sensor template packet in loop
    4) periodically force FULL resync for late-join receivers
*/

#include <OSynapticFX.h>

static osfx_easy_context g_easy;
static uint8_t g_packet[384];
static int g_packet_len = 0;
static uint8_t g_packet_cmd = 0;
static osfx_core_sensor_input g_sensors[2];
static unsigned long g_emit_count = 0UL;

static uint64_t now_unix_seconds(void) {
  return 1710000000ULL + (uint64_t)(millis() / 1000UL);
}

void setup() {
  uint32_t aid = 0;

  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }
  delay(200);

  Serial.println();
  Serial.println("========================================");
  Serial.println("OSynaptic-FX Easy Quick Start");
  Serial.println("========================================");

  osfx_easy_init(&g_easy);
  (void)osfx_easy_set_node(&g_easy, "NODE_A", "ONLINE");
  osfx_easy_set_tid(&g_easy, 2U);

  if (!osfx_easy_init_id_allocator(&g_easy, 100U, 10000U, 86400U)) {
    Serial.println("FAIL: id allocator init");
    return;
  }

  if (!osfx_easy_allocate_aid(&g_easy, now_unix_seconds(), &aid)) {
    Serial.println("FAIL: aid allocate");
    return;
  }

  Serial.print("AID=");
  Serial.println((unsigned long)aid);
}

void loop() {
  static unsigned long last_ms = 0UL;
  double temp_c;
  double hum_pct;

  if ((millis() - last_ms) < 1000UL) {
    return;
  }
  last_ms = millis();

  /*
    Force a periodic FULL packet so a receiver that starts late can
    re-learn template state and stop reporting "Template missing" on HEART.
  */
  if ((g_emit_count % 30UL) == 0UL) {
    osfx_fusion_state_reset(&g_easy.tx_state);
    Serial.println("resync: force FULL packet");
  }
  g_emit_count++;

    temp_c = 25.0 + (double)((millis() / 1000UL) % 5UL) * 0.1;
    hum_pct = 55.0 + (double)((millis() / 1000UL) % 7UL) * 0.2;

    g_sensors[0].sensor_id = "TEMP";
    g_sensors[0].sensor_state = "OK";
    g_sensors[0].value = temp_c;
    g_sensors[0].unit = "Cel";
    g_sensors[0].geohash_id = "";
    g_sensors[0].supplementary_message = "";
    g_sensors[0].resource_url = "";

    g_sensors[1].sensor_id = "HUM";
    g_sensors[1].sensor_state = "OK";
    g_sensors[1].value = hum_pct;
    g_sensors[1].unit = "%";
    g_sensors[1].geohash_id = "";
    g_sensors[1].supplementary_message = "";
    g_sensors[1].resource_url = "";

    if (!osfx_easy_encode_multi_sensor_auto(
          &g_easy,
          now_unix_seconds(),
      g_sensors,
      2U,
          g_packet,
          sizeof(g_packet),
          &g_packet_len,
          &g_packet_cmd)) {
    Serial.println("encode failed");
    return;
  }

  Serial.print("cmd=");
  Serial.print(osfx_easy_cmd_name(g_packet_cmd));
  Serial.print(" (");
  Serial.print((unsigned int)g_packet_cmd);
  Serial.print(") len=");
  Serial.print(g_packet_len);
  Serial.print(" aid=");
  Serial.print((unsigned long)g_easy.aid);
  Serial.print(" temp_c=");
  Serial.print(temp_c, 2);
  Serial.print(" hum_pct=");
  Serial.println(hum_pct, 2);
}
