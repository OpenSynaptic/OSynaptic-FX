/*
  BasicEncode.ino

  Purpose:
    Minimal single-sensor encoding using core API.

  Suggested audience:
    Core API beginners.
    If you want fewer steps, see EasyQuickStart.
*/

#include <OSynapticFX.h>

static uint8_t packet_buf[256];

void setup() {
  int packet_len = 0;
  int ok = 0;

  Serial.begin(115200);
  while (!Serial) {
  }

  Serial.println();
  Serial.println("OSynaptic-FX Example: BasicEncode (core API)");

  ok = osfx_core_encode_sensor_packet(
      0x01020304U,
      1,
      123456ULL,
      "temp_sensor",
      23.75,
      "C",
      packet_buf,
      sizeof(packet_buf),
      &packet_len);

  Serial.print("osfx encode ok=");
  Serial.println(ok);
  Serial.print("packet length=");
  Serial.println(packet_len);
}

void loop() {
}
