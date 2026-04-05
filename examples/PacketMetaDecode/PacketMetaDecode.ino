/*
  PacketMetaDecode.ino

  Purpose:
    Encode one packet, parse metadata, extract body,
    and decode sensor fields back to readable values.

  Suggested audience:
    Gateway-side auditing and decoding workflows.
*/

#include <OSynapticFX.h>

static uint8_t packet_buf[256];
static uint8_t body_buf[192];

static void print_hex(const uint8_t* data, size_t len) {
  size_t i;
  for (i = 0; i < len; ++i) {
    if (data[i] < 16U) {
      Serial.print('0');
    }
    Serial.print(data[i], HEX);
  }
  Serial.println();
}

void setup() {
  int packet_len = 0;
  osfx_packet_meta meta;
  size_t body_len = 0;
  char sensor_id[32];
  char unit[24];
  double value = 0.0;

  Serial.begin(115200);
  while (!Serial) {
  }

  Serial.println();
  Serial.println("OSynaptic-FX Example: PacketMetaDecode (core API)");

  if (!osfx_core_encode_sensor_packet(
          0x0A0B0C0DU,
          2,
          1200ULL,
          "pressure_line",
          101.325,
          "kPa",
          packet_buf,
          sizeof(packet_buf),
          &packet_len)) {
    Serial.println("encode failed");
    return;
  }

  Serial.print("packet_hex=");
  print_hex(packet_buf, (size_t)packet_len);

  if (!osfx_packet_decode_meta(packet_buf, (size_t)packet_len, &meta)) {
    Serial.println("decode meta failed");
    return;
  }

  if (!osfx_packet_extract_body(packet_buf, (size_t)packet_len, NULL, 0, body_buf, sizeof(body_buf), &body_len, &meta)) {
    Serial.println("extract body failed");
    return;
  }

  if (!osfx_core_decode_sensor_body(body_buf, body_len, sensor_id, sizeof(sensor_id), &value, unit, sizeof(unit))) {
    Serial.println("decode sensor body failed");
    return;
  }

  Serial.print("meta.cmd=");
  Serial.println((unsigned int)meta.cmd);
  Serial.print("meta.source_aid=");
  Serial.println((unsigned long)meta.source_aid);
  Serial.print("meta.tid=");
  Serial.println((unsigned int)meta.tid);

  Serial.print("sensor_id=");
  Serial.println(sensor_id);
  Serial.print("value=");
  Serial.println(value, 4);
  Serial.print("unit=");
  Serial.println(unit);
}

void loop() {
}
