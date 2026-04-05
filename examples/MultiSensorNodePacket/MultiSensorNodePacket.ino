/*
  MultiSensorNodePacket.ino

  Purpose:
    Encode and decode aggregated multi-sensor node packets.

  Suggested audience:
    Node-side aggregation scenarios.
*/

#include <OSynapticFX.h>

static osfx_fusion_state tx_state;
static osfx_fusion_state rx_state;
static uint8_t packet_buf[512];

void setup() {
  osfx_core_sensor_input in_sensors[2];
  osfx_core_sensor_output out_sensors[4];
  char node_id[32];
  char node_state[16];
  size_t out_count = 0;
  int packet_len = 0;
  uint8_t out_cmd = 0;
  osfx_packet_meta meta;
  size_t i;

  Serial.begin(115200);
  while (!Serial) {
  }

  Serial.println();
  Serial.println("OSynaptic-FX Example: MultiSensorNodePacket (core API)");

  osfx_fusion_state_init(&tx_state);
  osfx_fusion_state_init(&rx_state);

  in_sensors[0].sensor_id = "temp_A";
  in_sensors[0].sensor_state = "ok";
  in_sensors[0].value = 25.4;
  in_sensors[0].unit = "C";
  in_sensors[0].geohash_id = "";
  in_sensors[0].supplementary_message = "normal";
  in_sensors[0].resource_url = "";

  in_sensors[1].sensor_id = "temp_B";
  in_sensors[1].sensor_state = "ok";
  in_sensors[1].value = 24.9;
  in_sensors[1].unit = "C";
  in_sensors[1].geohash_id = "";
  in_sensors[1].supplementary_message = "normal";
  in_sensors[1].resource_url = "";

  if (!osfx_core_encode_multi_sensor_packet_auto(
          &tx_state,
          0x55667788U,
          9,
          3000ULL,
          "node_alpha",
          "active",
          in_sensors,
          2,
          packet_buf,
          sizeof(packet_buf),
          &packet_len,
          &out_cmd)) {
    Serial.println("encode multi sensor failed");
    return;
  }

  if (!osfx_core_decode_multi_sensor_packet_auto(
          &rx_state,
          packet_buf,
          (size_t)packet_len,
          node_id,
          sizeof(node_id),
          node_state,
          sizeof(node_state),
          out_sensors,
          4,
          &out_count,
          &meta)) {
    Serial.println("decode multi sensor failed");
    return;
  }

  Serial.print("cmd=");
  Serial.println((unsigned int)out_cmd);
  Serial.print("node_id=");
  Serial.println(node_id);
  Serial.print("node_state=");
  Serial.println(node_state);
  Serial.print("sensor_count=");
  Serial.println((unsigned int)out_count);

  for (i = 0; i < out_count; ++i) {
    Serial.print("sensor[");
    Serial.print((unsigned int)i);
    Serial.print("] id=");
    Serial.print(out_sensors[i].sensor_id);
    Serial.print(" value=");
    Serial.print(out_sensors[i].value, 4);
    Serial.print(" unit=");
    Serial.println(out_sensors[i].unit);
  }
}

void loop() {
}
