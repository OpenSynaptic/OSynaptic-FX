/*
  FusionAutoMode.ino

  Purpose:
    Demonstrate FULL/DIFF auto mode transitions for single sensor flow.

  Suggested audience:
    Users tuning bandwidth with fusion state behavior.
*/

#include <OSynapticFX.h>

static osfx_fusion_state tx_state;
static osfx_fusion_state rx_state;
static uint8_t packet_a[256];
static uint8_t packet_b[256];

void setup() {
  int len_a = 0;
  int len_b = 0;
  uint8_t cmd_a = 0;
  uint8_t cmd_b = 0;
  char sensor_id[32];
  char unit[24];
  double value = 0.0;
  osfx_packet_meta meta;

  Serial.begin(115200);
  while (!Serial) {
  }

  Serial.println();
  Serial.println("OSynaptic-FX Example: FusionAutoMode (core API)");

  osfx_fusion_state_init(&tx_state);
  osfx_fusion_state_init(&rx_state);

  if (!osfx_core_encode_sensor_packet_auto(
          &tx_state,
          0x11223344U,
          1,
          2000ULL,
          "temp_zone_A",
          24.5,
          "C",
          packet_a,
          sizeof(packet_a),
          &len_a,
          &cmd_a)) {
    Serial.println("encode A failed");
    return;
  }

  if (!osfx_core_encode_sensor_packet_auto(
          &tx_state,
          0x11223344U,
          1,
          2010ULL,
          "temp_zone_A",
          24.6,
          "C",
          packet_b,
          sizeof(packet_b),
          &len_b,
          &cmd_b)) {
    Serial.println("encode B failed");
    return;
  }

  Serial.print("cmd_a=");
  Serial.println((unsigned int)cmd_a);
  Serial.print("cmd_b=");
  Serial.println((unsigned int)cmd_b);

  if (!osfx_core_decode_sensor_packet_auto(
          &rx_state,
          packet_a,
          (size_t)len_a,
          sensor_id,
          sizeof(sensor_id),
          &value,
          unit,
          sizeof(unit),
          &meta)) {
    Serial.println("decode A failed");
    return;
  }

  Serial.print("A sensor=");
  Serial.println(sensor_id);
  Serial.print("A value=");
  Serial.println(value, 4);

  if (!osfx_core_decode_sensor_packet_auto(
          &rx_state,
          packet_b,
          (size_t)len_b,
          sensor_id,
          sizeof(sensor_id),
          &value,
          unit,
          sizeof(unit),
          &meta)) {
    Serial.println("decode B failed");
    return;
  }

  Serial.print("B sensor=");
  Serial.println(sensor_id);
  Serial.print("B value=");
  Serial.println(value, 4);
}

void loop() {
}
