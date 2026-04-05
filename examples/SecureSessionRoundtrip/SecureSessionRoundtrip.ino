/*
  SecureSessionRoundtrip.ino

  Purpose:
    Show secure-session transition and encrypted packet roundtrip decode.

  Suggested audience:
    Users validating secure send/decode behavior.
*/

#include <OSynapticFX.h>

static osfx_secure_session_store secure_store;
static uint8_t secure_packet[256];

void setup() {
  const uint32_t aid = 0x10203040U;
  int packet_len = 0;
  char sensor_id[32];
  char unit[24];
  double value = 0.0;
  osfx_packet_meta meta;

  Serial.begin(115200);
  while (!Serial) {
  }

  Serial.println();
  Serial.println("OSynaptic-FX Example: SecureSessionRoundtrip (core API)");

  osfx_secure_store_init(&secure_store, 3600ULL);

  if (!osfx_secure_note_plaintext_sent(&secure_store, aid, 1000ULL, 1000ULL)) {
    Serial.println("note plaintext failed");
    return;
  }
  if (!osfx_secure_confirm_dict(&secure_store, aid, 1000ULL, 1001ULL)) {
    Serial.println("confirm dict failed");
    return;
  }

  Serial.print("should_encrypt=");
  Serial.println(osfx_secure_should_encrypt(&secure_store, aid));

  if (!osfx_core_encode_sensor_packet_secure(
          &secure_store,
          aid,
          3,
          1010ULL,
          "temperature_secure",
          26.2,
          "C",
          secure_packet,
          sizeof(secure_packet),
          &packet_len)) {
    Serial.println("secure encode failed");
    return;
  }

  if (!osfx_core_decode_sensor_packet_secure(
          &secure_store,
          secure_packet,
          (size_t)packet_len,
          sensor_id,
          sizeof(sensor_id),
          &value,
          unit,
          sizeof(unit),
          &meta)) {
    Serial.println("secure decode failed");
    return;
  }

  Serial.print("decoded sensor=");
  Serial.println(sensor_id);
  Serial.print("decoded value=");
  Serial.println(value, 4);
  Serial.print("decoded unit=");
  Serial.println(unit);
  Serial.print("wire_cmd=");
  Serial.println((unsigned int)meta.cmd);
}

void loop() {
}
