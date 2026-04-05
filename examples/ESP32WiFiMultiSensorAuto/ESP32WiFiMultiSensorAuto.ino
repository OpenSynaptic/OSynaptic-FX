#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSynapticFX.h>

const char* WIFI_SSID = "毛君鹏";
const char* WIFI_PASS = "12345678";

const IPAddress REMOTE_IP(192, 168, 3, 198);
const uint16_t REMOTE_PORT = 9000;
const uint16_t LOCAL_UDP_PORT = 58925;

const unsigned long SEND_INTERVAL_MS = 1000UL;
const unsigned long WIFI_RETRY_INTERVAL_MS = 3000UL;
const unsigned long RESYNC_INTERVAL_PACKETS = 300UL;

WiFiUDP udp;

osfx_easy_context easy_ctx;
uint8_t packet_buf[384];
osfx_core_sensor_input sensors[2];

unsigned long emit_count = 0UL;
unsigned long last_send_ms = 0UL;
unsigned long last_wifi_retry_ms = 0UL;

double temp_hold_c = 25.0;
double hum_pct = 55.0;

static uint64_t now_unix_seconds() {
  return 1710000000ULL + (uint64_t)(millis() / 1000UL);
}

static bool ensure_udp_ready() {
  udp.stop();
  if (!udp.begin(LOCAL_UDP_PORT)) {
    Serial.println("udp.begin failed");
    return false;
  }
  return true;
}

static bool connect_wifi_blocking(unsigned long timeout_ms) {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("WiFi connecting");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
    if ((millis() - start) >= timeout_ms) {
      Serial.println();
      Serial.println("WiFi connect timeout");
      return false;
    }
  }

  Serial.println();
  Serial.print("WiFi connected, local IP: ");
  Serial.println(WiFi.localIP());

  return ensure_udp_ready();
}

static bool send_udp_packet(const uint8_t* data, size_t len) {
  if (!data || len == 0) {
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  if (!udp.beginPacket(REMOTE_IP, REMOTE_PORT)) {
    return false;
  }

  size_t written = udp.write(data, len);
  int ok = udp.endPacket();
  return (written == len) && (ok == 1);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }

  (void)connect_wifi_blocking(20000UL);

  osfx_easy_init(&easy_ctx);
  osfx_easy_set_aid(&easy_ctx, 500U);
  osfx_easy_set_tid(&easy_ctx, 2U);
  (void)osfx_easy_set_node(&easy_ctx, "NODE_A", "ONLINE");

  Serial.println("ready");
}

void loop() {
  int packet_len = 0;
  uint8_t cmd = 0;

  unsigned long now_ms = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if ((now_ms - last_wifi_retry_ms) >= WIFI_RETRY_INTERVAL_MS) {
      last_wifi_retry_ms = now_ms;
      Serial.println("WiFi lost, reconnecting...");
      (void)connect_wifi_blocking(15000UL);
    }
    delay(50);
    return;
  }

  if ((now_ms - last_send_ms) < SEND_INTERVAL_MS) {
    return;
  }
  last_send_ms = now_ms;

  if (RESYNC_INTERVAL_PACKETS > 0UL && emit_count > 0UL && (emit_count % RESYNC_INTERVAL_PACKETS) == 0UL) {
    osfx_fusion_state_reset(&easy_ctx.tx_state);
    Serial.println("resync: force FULL packet");
  }

  // Keep the first slot slow-changing and place the fast-changing value in the
  // last slot so auto fusion can produce DIFF/HEART more often.
  if ((emit_count % 30UL) == 0UL && emit_count > 0UL) {
    temp_hold_c += 0.1;
    if (temp_hold_c > 26.0) {
      temp_hold_c = 25.0;
    }
  }

  if ((emit_count % 6UL) < 3UL) {
    hum_pct = 55.0;
  } else {
    hum_pct = 55.2;
  }

  sensors[0].sensor_id = "TEMP";
  sensors[0].sensor_state = "OK";
  sensors[0].value = temp_hold_c;
  sensors[0].unit = "Cel";
  sensors[0].geohash_id = "";
  sensors[0].supplementary_message = "";
  sensors[0].resource_url = "";

  sensors[1].sensor_id = "HUM";
  sensors[1].sensor_state = "OK";
  sensors[1].value = hum_pct;
  sensors[1].unit = "%";
  sensors[1].geohash_id = "";
  sensors[1].supplementary_message = "";
  sensors[1].resource_url = "";

  int enc_ok = osfx_easy_encode_multi_sensor_auto(
      &easy_ctx,
      now_unix_seconds(),
      sensors,
      2U,
      packet_buf,
      sizeof(packet_buf),
      &packet_len,
      &cmd
  );

  if (enc_ok && packet_len > 0) {
    bool sent = send_udp_packet(packet_buf, (size_t)packet_len);
    Serial.print("cmd=");
    Serial.print(osfx_easy_cmd_name(cmd));
    Serial.print(" (");
    Serial.print((unsigned)cmd);
    Serial.print(") len=");
    Serial.print(packet_len);
    Serial.print(" sent=");
    Serial.print(sent ? "yes" : "no");
    Serial.print(" temp_c=");
    Serial.print(temp_hold_c, 2);
    Serial.print(" hum_pct=");
    Serial.println(hum_pct, 2);
  } else {
    Serial.println("encode failed");
  }

  emit_count++;
}
