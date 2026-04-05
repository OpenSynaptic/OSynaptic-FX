#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "osfx_core.h"
#include "osfx_build_config.h"

static int test_crc16_vector(void) {
    const uint8_t vec[] = {'1','2','3','4','5','6','7','8','9'};
    uint16_t got = osfx_crc16_ccitt(vec, sizeof(vec), 0x1021U, 0xFFFFU);
    if (got != 0x29B1U) {
        printf("[FAIL] crc16 vector: got=0x%04X expected=0x29B1\n", (unsigned)got);
        return 0;
    }
    return 1;
}

static int test_b62_roundtrip(void) {
    long long values[] = {0LL, 1LL, -1LL, 62LL, -62LL, 123456789LL, -123456789LL};
    size_t i;
    char out[128];
    for (i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
        int ok = 0;
        long long dec;
        if (!osfx_b62_encode_i64(values[i], out, sizeof(out))) {
            printf("[FAIL] b62 encode failed for %lld\n", values[i]);
            return 0;
        }
        dec = osfx_b62_decode_i64(out, &ok);
        if (!ok || dec != values[i]) {
            printf("[FAIL] b62 roundtrip: in=%lld out=%s dec=%lld ok=%d\n", values[i], out, dec, ok);
            return 0;
        }
    }
    return 1;
}

static int test_packet_full_min_decode(void) {
    const uint8_t body[] = "OSFX_PACKET_BODY";
    uint8_t frame[256];
    uint64_t f[9];
    int n = osfx_packet_encode_full(63U, 42U, 1U, 1710000000ULL, body, sizeof(body) - 1U, frame, sizeof(frame));
    if (n <= 0) {
        printf("[FAIL] packet encode failed\n");
        return 0;
    }
    if (!osfx_packet_decode_min(frame, (size_t)n, f)) {
        printf("[FAIL] packet decode_min failed\n");
        return 0;
    }
    if (f[0] != 63U || f[1] != 1U || f[2] != 42U || f[3] != 1U) {
        printf("[FAIL] packet header mismatch\n");
        return 0;
    }
    if (f[4] != 1710000000ULL) {
        printf("[FAIL] timestamp mismatch: got=%llu\n", (unsigned long long)f[4]);
        return 0;
    }
    if (f[7] != 1U || f[8] != 1U) {
        printf("[FAIL] crc flags mismatch crc8_ok=%llu crc16_ok=%llu\n", (unsigned long long)f[7], (unsigned long long)f[8]);
        return 0;
    }
    return 1;
}

static int test_standardization_basic(void) {
    double out = 0.0;
    char unit[16];
    if (!osfx_standardize_value(1.0, "kPa", &out, unit, sizeof(unit))) {
        printf("[FAIL] standardize kPa failed\n");
        return 0;
    }
    if (out != 1000.0 || strcmp(unit, "Pa") != 0) {
        printf("[FAIL] standardize kPa mismatch out=%f unit=%s\n", out, unit);
        return 0;
    }
    if (!osfx_standardize_value(32.0, "degF", &out, unit, sizeof(unit))) {
        printf("[FAIL] standardize degF failed\n");
        return 0;
    }
    if (out < 273.14 || out > 273.16 || strcmp(unit, "K") != 0) {
        printf("[FAIL] standardize degF mismatch out=%f unit=%s\n", out, unit);
        return 0;
    }
    if (!osfx_standardize_value(1.0, "mm[Hg]", &out, unit, sizeof(unit))) {
        printf("[FAIL] standardize mm[Hg] failed\n");
        return 0;
    }
    if (out < 133.32 || out > 133.33 || strcmp(unit, "Pa") != 0) {
        printf("[FAIL] standardize mm[Hg] mismatch out=%f unit=%s\n", out, unit);
        return 0;
    }
    return 1;
}

static int test_library_catalog_generated(void) {
    const char* sym = NULL;
    if (!osfx_library_catalog_ready()) {
        printf("[FAIL] library catalog not ready\n");
        return 0;
    }
    if (osfx_library_unit_count() < 30U || osfx_library_prefix_count() < 10U) {
        printf("[FAIL] library catalog counts too small units=%llu prefixes=%llu\n",
               (unsigned long long)osfx_library_unit_count(),
               (unsigned long long)osfx_library_prefix_count());
        return 0;
    }
    if (!osfx_library_unit_symbol("pa", &sym) || strcmp(sym, "900") != 0) {
        printf("[FAIL] unit symbol lookup mismatch\n");
        return 0;
    }
    if (!osfx_library_state_symbol("ok", &sym) || strcmp(sym, "K") != 0) {
        printf("[FAIL] state symbol lookup mismatch\n");
        return 0;
    }
    return 1;
}

static int test_handshake_builders(void) {
    uint8_t buf[64];
    int n = osfx_build_ack(0x1234U, buf, sizeof(buf));
    if (n != 3 || buf[0] != OSFX_CMD_HANDSHAKE_ACK || buf[1] != 0x12 || buf[2] != 0x34) {
        printf("[FAIL] build_ack mismatch\n");
        return 0;
    }
    n = osfx_build_id_assign(0x0001U, 42U, buf, sizeof(buf));
    if (n != 7 || buf[0] != OSFX_CMD_ID_ASSIGN) {
        printf("[FAIL] build_id_assign mismatch\n");
        return 0;
    }
    if (osfx_cmd_normalize_data(OSFX_CMD_DATA_DIFF_SEC) != OSFX_CMD_DATA_DIFF) {
        printf("[FAIL] normalize_data mismatch\n");
        return 0;
    }
    if (osfx_cmd_secure_variant(OSFX_CMD_DATA_HEART) != OSFX_CMD_DATA_HEART_SEC) {
        printf("[FAIL] secure_variant mismatch\n");
        return 0;
    }
    return 1;
}

static int test_fusion_state_transitions(void) {
    osfx_fusion_state st;
    uint8_t pkt[256];
    int pkt_len = 0;
    uint8_t cmd = 0;
    const uint8_t full1[] = "N1.ONLINE.1710000000|TEMP>OK.Pa:abc|";
    const uint8_t full2[] = "N1.ONLINE.1710000001|TEMP>OK.Pa:abd|";
    const uint8_t full3[] = "N1.ONLINE.1710000002|TEMP>OK.Pa:abd|";

    osfx_fusion_state_init(&st);
    if (!osfx_fusion_encode(&st, 7U, 1U, 1ULL, full1, sizeof(full1) - 1U, pkt, sizeof(pkt), &pkt_len, &cmd)) {
        printf("[FAIL] fusion FULL encode failed\n");
        return 0;
    }
    if (cmd != OSFX_CMD_DATA_FULL) {
        printf("[FAIL] expected FULL cmd, got=%u\n", (unsigned)cmd);
        return 0;
    }
    if (!osfx_fusion_encode(&st, 7U, 1U, 2ULL, full2, sizeof(full2) - 1U, pkt, sizeof(pkt), &pkt_len, &cmd)) {
        printf("[FAIL] fusion DIFF encode failed\n");
        return 0;
    }
    if (cmd != OSFX_CMD_DATA_DIFF) {
        printf("[FAIL] expected DIFF cmd, got=%u\n", (unsigned)cmd);
        return 0;
    }
    if (!osfx_fusion_encode(&st, 7U, 1U, 3ULL, full3, sizeof(full3) - 1U, pkt, sizeof(pkt), &pkt_len, &cmd)) {
        printf("[FAIL] fusion HEART encode failed\n");
        return 0;
    }
    if (cmd != OSFX_CMD_DATA_HEART) {
        printf("[FAIL] expected HEART cmd, got=%u\n", (unsigned)cmd);
        return 0;
    }
    return 1;
}

static int test_template_grammar(void) {
    osfx_sensor_slot slots[2];
    osfx_template_msg msg;
    char out[256];
    memset(slots, 0, sizeof(slots));
    strcpy(slots[0].sensor_id, "TEMP");
    strcpy(slots[0].sensor_state, "OK");
    strcpy(slots[0].sensor_unit, "Pa");
    strcpy(slots[0].sensor_value, "abc");
#if OSFX_CFG_PAYLOAD_GEOHASH_ID
    strcpy(slots[0].geohash_id, "wx4g0ec1");
#endif
    strcpy(slots[0].supplementary_message, "msg0");
#if OSFX_CFG_PAYLOAD_RESOURCE_URL
    strcpy(slots[0].resource_url, "https://r/0");
#endif
    strcpy(slots[1].sensor_id, "HUM");
    strcpy(slots[1].sensor_state, "OK");
    strcpy(slots[1].sensor_unit, "%");
    strcpy(slots[1].sensor_value, "def");
#if OSFX_CFG_PAYLOAD_GEOHASH_ID
    strcpy(slots[1].geohash_id, "wx4g0ec2");
#endif
    strcpy(slots[1].supplementary_message, "msg1");
#if OSFX_CFG_PAYLOAD_RESOURCE_URL
    strcpy(slots[1].resource_url, "https://r/1");
#endif

    if (!osfx_template_encode("N1", "ONLINE", "1710000000", slots, 2U, out, sizeof(out))) {
        printf("[FAIL] template encode failed\n");
        return 0;
    }
    if (!osfx_template_decode(out, &msg)) {
        printf("[FAIL] template decode failed\n");
        return 0;
    }
    if (strcmp(msg.node_id, "N1") != 0 || msg.sensor_count != 2U || strcmp(msg.sensors[1].sensor_id, "HUM") != 0) {
        printf("[FAIL] template mismatch\n");
        return 0;
    }
#if OSFX_CFG_PAYLOAD_GEOHASH_ID
    if (strcmp(msg.sensors[0].geohash_id, "wx4g0ec1") != 0) {
        printf("[FAIL] template geohash mismatch\n");
        return 0;
    }
#endif
    if (strcmp(msg.sensors[1].supplementary_message, "msg1") != 0) {
        printf("[FAIL] template supplementary_message mismatch\n");
        return 0;
    }
#if OSFX_CFG_PAYLOAD_RESOURCE_URL
    if (strcmp(msg.sensors[0].resource_url, "https://r/0") != 0) {
        printf("[FAIL] template resource_url mismatch\n");
        return 0;
    }
#endif
    return 1;
}

static int test_multi_sensor_payload_switches(void) {
    osfx_fusion_state st_tx;
    osfx_fusion_state st_rx;
    osfx_core_sensor_input in[2];
    osfx_core_sensor_output out[2];
    uint8_t packet[1024];
    int packet_len = 0;
    uint8_t cmd = 0;
    char node_id[OSFX_TMPL_ID_MAX];
    char node_state[OSFX_TMPL_STATE_MAX];
    size_t out_count = 0;
    osfx_packet_meta meta;

    memset(in, 0, sizeof(in));
    memset(out, 0, sizeof(out));
    osfx_fusion_state_init(&st_tx);
    osfx_fusion_state_init(&st_rx);

    in[0].sensor_id = "TEMP1";
    in[0].sensor_state = "OK";
    in[0].value = 23.5;
    in[0].unit = "cel";
    in[0].geohash_id = "wx4g0ec1";
    in[0].supplementary_message = "m0";
    in[0].resource_url = "https://x/0";

    in[1].sensor_id = "PRESS1";
    in[1].sensor_state = "OK";
    in[1].value = 101.3;
    in[1].unit = "kPa";
    in[1].geohash_id = "wx4g0ec2";
    in[1].supplementary_message = "m1";
    in[1].resource_url = "https://x/1";

    if (!osfx_core_encode_multi_sensor_packet_auto(
            &st_tx,
            500U,
            2U,
            1710001234ULL,
            "NODE_A",
            "ONLINE",
            in,
            2U,
            packet,
            sizeof(packet),
            &packet_len,
            &cmd)) {
        printf("[FAIL] multi sensor encode failed\n");
        return 0;
    }

    if (!osfx_core_decode_multi_sensor_packet_auto(
            &st_rx,
            packet,
            (size_t)packet_len,
            node_id,
            sizeof(node_id),
            node_state,
            sizeof(node_state),
            out,
            2U,
            &out_count,
            &meta)) {
        printf("[FAIL] multi sensor decode failed\n");
        return 0;
    }

    if (out_count != 2U) {
        printf("[FAIL] multi sensor count mismatch\n");
        return 0;
    }

    #if OSFX_CFG_PAYLOAD_GEOHASH_ID
    if (strcmp(out[0].geohash_id, "wx4g0ec1") != 0) {
        printf("[FAIL] geohash switch expected on\n");
        return 0;
    }
    #else
    if (out[0].geohash_id[0] != '\0') {
        printf("[FAIL] geohash switch expected off\n");
        return 0;
    }
    #endif

    #if OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE
    if (strcmp(out[1].supplementary_message, "m1") != 0) {
        printf("[FAIL] supplementary message switch expected on\n");
        return 0;
    }
    #else
    if (out[1].supplementary_message[0] != '\0') {
        printf("[FAIL] supplementary message switch expected off\n");
        return 0;
    }
    #endif

    #if OSFX_CFG_PAYLOAD_RESOURCE_URL
    if (strcmp(out[0].resource_url, "https://x/0") != 0) {
        printf("[FAIL] resource url switch expected on\n");
        return 0;
    }
    #else
    if (out[0].resource_url[0] != '\0') {
        printf("[FAIL] resource url switch expected off\n");
        return 0;
    }
    #endif

    return 1;
}

static int send_fail(const uint8_t* payload, size_t payload_len, void* user_ctx) {
    (void)payload;
    (void)payload_len;
    (void)user_ctx;
    return 0;
}

static int send_ok(const uint8_t* payload, size_t payload_len, void* user_ctx) {
    int* flag = (int*)user_ctx;
    (void)payload;
    if (flag) {
        *flag = (int)payload_len;
    }
    return 1;
}

static int test_transporter_runtime(void) {
    osfx_transporter_runtime rt;
    uint8_t data[] = {1,2,3};
    char used[32];
    int sent = 0;

    osfx_transporter_runtime_init(&rt);
    if (!osfx_transporter_register(&rt, "udp", 10, send_fail, NULL)) {
        return 0;
    }
    if (!osfx_transporter_register(&rt, "uart", 5, send_ok, &sent)) {
        return 0;
    }
    if (!osfx_transporter_dispatch_auto(&rt, data, sizeof(data), used, sizeof(used))) {
        printf("[FAIL] transporter auto dispatch failed\n");
        return 0;
    }
    if (strcmp(used, "uart") != 0 || sent != 3) {
        printf("[FAIL] transporter auto used=%s sent=%d\n", used, sent);
        return 0;
    }
    return 1;
}

static int test_secure_session_store(void) {
    osfx_secure_session_store st;
    osfx_secure_session_store st2;
    const char* path = "build/secure_sessions_test.txt";
    osfx_secure_store_init(&st, 3600U);
    if (!osfx_secure_note_plaintext_sent(&st, 42U, 1710000000ULL, 1710000001ULL)) {
        return 0;
    }
    if (!osfx_secure_confirm_dict(&st, 42U, 1710000000ULL, 1710000002ULL)) {
        return 0;
    }
    if (!osfx_secure_mark_channel(&st, 42U, 1710000003ULL)) {
        return 0;
    }
    if (!osfx_secure_should_encrypt(&st, 42U)) {
        printf("[FAIL] secure should_encrypt false\n");
        return 0;
    }
    if (!osfx_secure_store_save(&st, path)) {
        printf("[FAIL] secure save failed\n");
        return 0;
    }
    osfx_secure_store_init(&st2, 3600U);
    if (!osfx_secure_store_load(&st2, path)) {
        printf("[FAIL] secure load failed\n");
        return 0;
    }
    if (!osfx_secure_should_encrypt(&st2, 42U)) {
        printf("[FAIL] secure loaded should_encrypt false\n");
        return 0;
    }
    remove(path);
    return 1;
}

static int test_secure_payload_pipeline(void) {
    osfx_secure_session_store st;
    uint8_t packet[256];
    int packet_len = 0;
    char sensor_id[32];
    char unit[16];
    double value = 0.0;
    osfx_packet_meta meta;

    osfx_secure_store_init(&st, 3600U);
    if (!osfx_secure_note_plaintext_sent(&st, 77U, 1710000100ULL, 1710000101ULL)) {
        return 0;
    }
    if (!osfx_secure_confirm_dict(&st, 77U, 1710000100ULL, 1710000102ULL)) {
        return 0;
    }

    if (!osfx_core_encode_sensor_packet_secure(&st, 77U, 3U, 1710000103ULL, "P1", 14.7, "psi", packet, sizeof(packet), &packet_len)) {
        printf("[FAIL] secure encode failed\n");
        return 0;
    }
    if (!osfx_core_decode_sensor_packet_secure(&st, packet, (size_t)packet_len, sensor_id, sizeof(sensor_id), &value, unit, sizeof(unit), &meta)) {
        printf("[FAIL] secure decode failed\n");
        return 0;
    }
    if (meta.cmd != OSFX_CMD_DATA_FULL_SEC || strcmp(sensor_id, "P1") != 0 || strcmp(unit, "Pa") != 0) {
        printf("[FAIL] secure payload mismatch cmd=%u id=%s unit=%s\n", (unsigned)meta.cmd, sensor_id, unit);
        return 0;
    }
    if (value < 101300.0 || value > 101400.0) {
        printf("[FAIL] secure value mismatch value=%f\n", value);
        return 0;
    }
    return 1;
}

typedef struct test_mem_storage_ctx {
    char data[16384];
    size_t len;
    size_t read_pos;
    int readable;
} test_mem_storage_ctx;

static int test_mem_storage_write(void* handle, const char* text) {
    test_mem_storage_ctx* ctx = (test_mem_storage_ctx*)handle;
    size_t n;
    if (!ctx || !text) {
        return 0;
    }
    n = strlen(text);
    if (ctx->len + n + 1U > sizeof(ctx->data)) {
        return 0;
    }
    memcpy(ctx->data + ctx->len, text, n);
    ctx->len += n;
    ctx->data[ctx->len] = '\0';
    return 1;
}

static int test_mem_storage_writer_close(void* handle) {
    test_mem_storage_ctx* ctx = (test_mem_storage_ctx*)handle;
    if (!ctx) {
        return 0;
    }
    ctx->read_pos = 0U;
    ctx->readable = 1;
    return 1;
}

static int test_mem_storage_read_line(void* handle, char* out, size_t out_cap) {
    test_mem_storage_ctx* ctx = (test_mem_storage_ctx*)handle;
    size_t start;
    size_t n;
    if (!ctx || !out || out_cap == 0U || !ctx->readable) {
        return -1;
    }
    if (ctx->read_pos >= ctx->len) {
        return 0;
    }
    start = ctx->read_pos;
    while (ctx->read_pos < ctx->len && ctx->data[ctx->read_pos] != '\n') {
        ctx->read_pos++;
    }
    n = ctx->read_pos - start;
    if (ctx->read_pos < ctx->len && ctx->data[ctx->read_pos] == '\n') {
        ctx->read_pos++;
    }
    if (n + 1U > out_cap) {
        return -1;
    }
    memcpy(out, ctx->data + start, n);
    out[n] = '\0';
    return 1;
}

static int test_mem_storage_reader_close(void* handle) {
    test_mem_storage_ctx* ctx = (test_mem_storage_ctx*)handle;
    if (!ctx) {
        return 0;
    }
    return 1;
}

static int test_mem_storage_open_writer(void* user_ctx, const char* key, osfx_storage_writer* out_writer) {
    test_mem_storage_ctx* ctx = (test_mem_storage_ctx*)user_ctx;
    (void)key;
    if (!ctx || !out_writer) {
        return 0;
    }
    ctx->len = 0U;
    ctx->read_pos = 0U;
    ctx->readable = 0;
    ctx->data[0] = '\0';
    out_writer->handle = ctx;
    out_writer->write = test_mem_storage_write;
    out_writer->close = test_mem_storage_writer_close;
    return 1;
}

static int test_mem_storage_open_reader(void* user_ctx, const char* key, osfx_storage_reader* out_reader) {
    test_mem_storage_ctx* ctx = (test_mem_storage_ctx*)user_ctx;
    (void)key;
    if (!ctx || !out_reader || !ctx->readable) {
        return 0;
    }
    ctx->read_pos = 0U;
    out_reader->handle = ctx;
    out_reader->read_line = test_mem_storage_read_line;
    out_reader->close = test_mem_storage_reader_close;
    return 1;
}

static int test_id_allocator_persistence(void) {
    osfx_id_allocator alloc;
    osfx_id_allocator alloc2;
    const char* path = "build/id_alloc_test.txt";
    uint32_t a1 = 0;
    uint32_t a2 = 0;

    osfx_id_allocator_init(&alloc, 10U, 20U, 60U);
    if (!osfx_id_allocate(&alloc, 1000U, &a1)) {
        return 0;
    }
    if (!osfx_id_allocate(&alloc, 1001U, &a2)) {
        return 0;
    }
    if (a1 != 10U || a2 != 11U) {
        printf("[FAIL] allocator sequence mismatch\n");
        return 0;
    }
    if (!osfx_id_release(&alloc, 10U)) {
        return 0;
    }
    osfx_id_set_policy(&alloc, 5U, 60U, 1.0, 0.1, 1);
    osfx_id_set_policy_ex(&alloc, 5U, 30U, 60U, 1.0, 0.1, 0.2, 0.2, 0.5, 1);
    if (!osfx_id_allocate(&alloc, 1002U, &a1)) {
        return 0;
    }
    {
        osfx_id_lease_entry* e = NULL;
        size_t i;
        for (i = 0; i < OSFX_ID_MAX_ENTRIES; ++i) {
            if (alloc.entries[i].in_use && alloc.entries[i].aid == a1) {
                e = &alloc.entries[i];
                break;
            }
        }
        uint64_t lease_span;
        if (!e) {
            printf("[FAIL] allocator lease entry missing\n");
            return 0;
        }
        lease_span = (e->leased_until - e->last_seen);
        if (lease_span > alloc.default_lease_seconds || lease_span > alloc.max_lease_seconds) {
            printf("[FAIL] allocator adaptive lease mismatch\n");
            return 0;
        }
        if (!osfx_id_touch(&alloc, a1, 1010U)) {
            return 0;
        }
        lease_span = e->leased_until - e->last_seen;
        if (lease_span != 30U) {
            printf("[FAIL] allocator touch extension mismatch span=%llu\n", (unsigned long long)lease_span);
            return 0;
        }
    }
    if (!osfx_id_save(&alloc, path)) {
        return 0;
    }

    osfx_id_allocator_init(&alloc2, 0U, 0U, 0U);
    if (!osfx_id_load(&alloc2, path)) {
        return 0;
    }
    if (alloc2.start_id != 10U || alloc2.end_id != 20U) {
        printf("[FAIL] allocator range load mismatch\n");
        return 0;
    }
    if (alloc2.min_lease_seconds == 0U || alloc2.rate_window_seconds == 0U) {
        printf("[FAIL] allocator policy load mismatch\n");
        return 0;
    }
    if (alloc2.max_lease_seconds != 30U || alloc2.touch_extend_factor < 0.49 || alloc2.touch_extend_factor > 0.51) {
        printf("[FAIL] allocator extended policy load mismatch max=%llu touch=%f\n",
               (unsigned long long)alloc2.max_lease_seconds,
               alloc2.touch_extend_factor);
        return 0;
    }
    remove(path);
    return 1;
}

static int test_id_allocator_storage_backend(void) {
    osfx_id_allocator alloc;
    osfx_id_allocator alloc2;
    uint32_t aid = 0U;
    int ok = 0;
    const osfx_storage_backend* prev = osfx_storage_get_backend();
    test_mem_storage_ctx storage;
    osfx_storage_backend backend;

    memset(&storage, 0, sizeof(storage));
    backend.user_ctx = &storage;
    backend.open_writer = test_mem_storage_open_writer;
    backend.open_reader = test_mem_storage_open_reader;
    osfx_storage_set_backend(&backend);

    osfx_id_allocator_init(&alloc, 300U, 305U, 120U);
    if (!osfx_id_allocate(&alloc, 2000U, &aid) || aid != 300U) {
        goto done;
    }
    if (!osfx_id_touch(&alloc, aid, 2001U)) {
        goto done;
    }
    if (!osfx_id_save(&alloc, "id_mem_test")) {
        goto done;
    }

    osfx_id_allocator_init(&alloc2, 0U, 0U, 0U);
    if (!osfx_id_load(&alloc2, "id_mem_test")) {
        goto done;
    }
    if (alloc2.start_id != 300U || alloc2.end_id != 305U) {
        goto done;
    }
    if (!osfx_id_release(&alloc2, aid)) {
        goto done;
    }

    ok = 1;

done:
    osfx_storage_set_backend(prev);
    if (!ok) {
        printf("[FAIL] id storage backend abstraction failed\n");
    }
    return ok;
}

typedef struct test_matrix_ctx {
    int udp_seen;
    int tcp_seen;
    int uart_seen;
    int can_seen;
} test_matrix_ctx;

static int matrix_emit(const char* protocol, const uint8_t* frame, size_t frame_len, void* user_ctx) {
    test_matrix_ctx* ctx = (test_matrix_ctx*)user_ctx;
    (void)frame;
    (void)frame_len;
    if (!ctx || !protocol) {
        return 0;
    }
    if (strcmp(protocol, "udp") == 0) { ctx->udp_seen++; }
    if (strcmp(protocol, "tcp") == 0) { ctx->tcp_seen++; }
    if (strcmp(protocol, "uart") == 0) { ctx->uart_seen++; }
    if (strcmp(protocol, "can") == 0) { ctx->can_seen++; }
    return 1;
}

static int test_protocol_matrix_runtime(void) {
    osfx_protocol_matrix pm;
    test_matrix_ctx ctx;
    uint8_t payload[20];
    char used[16];
    memset(&ctx, 0, sizeof(ctx));
    memset(payload, 0xAB, sizeof(payload));

    osfx_protocol_matrix_init(&pm, matrix_emit, &ctx);
    if (!osfx_protocol_matrix_register_defaults(&pm)) {
        return 0;
    }
    if (!osfx_protocol_send_tcp(&pm, payload, sizeof(payload))) {
        return 0;
    }
    if (!osfx_protocol_send_uart(&pm, payload, 5U)) {
        return 0;
    }
    if (!osfx_protocol_send_can(&pm, payload, sizeof(payload))) {
        return 0;
    }
    if (!osfx_protocol_dispatch_auto(&pm, payload, sizeof(payload), used, sizeof(used))) {
        return 0;
    }
    if (ctx.tcp_seen < 1 || ctx.uart_seen < 1 || ctx.can_seen < 1 || strcmp(used, "udp") != 0) {
        printf("[FAIL] protocol matrix mismatch tcp=%d uart=%d can=%d used=%s\n", ctx.tcp_seen, ctx.uart_seen, ctx.can_seen, used);
        return 0;
    }
    return 1;
}

static int test_handshake_dispatch_flow(void) {
    osfx_secure_session_store sec;
    osfx_id_allocator alloc;
    osfx_hs_dispatch_ctx ctx;
    osfx_hs_result res;
    uint8_t body[] = "TEMP|Pa|abc";
    uint8_t pkt[256];
    int n;
    uint32_t assigned = 0;
    uint8_t key[OSFX_KEY_LEN];

    osfx_secure_store_init(&sec, 3600U);
    osfx_id_allocator_init(&alloc, 100U, 102U, 60U);
    memset(&ctx, 0, sizeof(ctx));
    ctx.secure_store = &sec;
    ctx.id_allocator = &alloc;
    ctx.now_ts = 2000U;

    n = osfx_packet_encode_full(OSFX_CMD_DATA_FULL, 55U, 1U, 1710000200ULL, body, sizeof(body) - 1U, pkt, sizeof(pkt));
    if (n <= 0 || !osfx_hs_classify_dispatch(&ctx, pkt, (size_t)n, &res)) {
        printf("[FAIL] hs dispatch plaintext data\n");
        return 0;
    }
    if (!res.has_response || res.response[0] != OSFX_CMD_SECURE_DICT_READY) {
        printf("[FAIL] hs dispatch dict-ready response missing\n");
        return 0;
    }

    if (!osfx_secure_get_key(&sec, 55U, key)) {
        printf("[FAIL] secure key missing after plaintext\n");
        return 0;
    }
    n = osfx_packet_encode_ex(OSFX_CMD_DATA_FULL, 55U, 1U, 1710000201ULL, body, sizeof(body) - 1U, 1, key, OSFX_KEY_LEN, pkt, sizeof(pkt));
    if (n <= 0 || !osfx_hs_classify_dispatch(&ctx, pkt, (size_t)n, &res)) {
        printf("[FAIL] hs dispatch secure data\n");
        return 0;
    }
    if (!res.has_response || res.response[0] != OSFX_CMD_SECURE_CHANNEL_ACK) {
        printf("[FAIL] hs dispatch secure channel ack missing\n");
        return 0;
    }

    if (osfx_hs_classify_dispatch(&ctx, pkt, (size_t)n, &res) != 0 || res.reject != OSFX_HS_REJECT_REPLAY) {
        printf("[FAIL] hs replay not rejected reject=%d\n", (int)res.reject);
        return 0;
    }

    n = osfx_packet_encode_ex(OSFX_CMD_DATA_FULL, 55U, 1U, 1710000199ULL, body, sizeof(body) - 1U, 1, key, OSFX_KEY_LEN, pkt, sizeof(pkt));
    if (n <= 0) {
        return 0;
    }
    if (osfx_hs_classify_dispatch(&ctx, pkt, (size_t)n, &res) != 0 || res.reject != OSFX_HS_REJECT_OUT_OF_ORDER) {
        printf("[FAIL] hs out-of-order not rejected reject=%d\n", (int)res.reject);
        return 0;
    }

    pkt[0] = OSFX_CMD_ID_REQUEST;
    pkt[1] = 0x00;
    pkt[2] = 0x02;
    if (!osfx_hs_classify_dispatch(&ctx, pkt, 3U, &res) || !res.has_response || res.response[0] != OSFX_CMD_ID_ASSIGN) {
        printf("[FAIL] hs dispatch id request\n");
        return 0;
    }
    assigned = ((uint32_t)res.response[3] << 24) | ((uint32_t)res.response[4] << 16) | ((uint32_t)res.response[5] << 8) | (uint32_t)res.response[6];
    if (assigned != 100U) {
        printf("[FAIL] hs assigned id mismatch=%u\n", (unsigned)assigned);
        return 0;
    }

    pkt[0] = OSFX_CMD_ID_REQUEST;
    pkt[1] = 0x00;
    if (osfx_hs_classify_dispatch(&ctx, pkt, 2U, &res) != 0 || res.reject != OSFX_HS_REJECT_MALFORMED || !res.has_response || res.response[0] != OSFX_CMD_HANDSHAKE_NACK) {
        printf("[FAIL] hs malformed id request not rejected\n");
        return 0;
    }

    pkt[0] = OSFX_CMD_PING;
    pkt[1] = 0x12;
    pkt[2] = 0x34;
    if (!osfx_hs_classify_dispatch(&ctx, pkt, 3U, &res) || !res.has_response || res.response[0] != OSFX_CMD_PONG) {
        printf("[FAIL] hs ping/pong\n");
        return 0;
    }

    pkt[0] = OSFX_CMD_PING;
    pkt[1] = 0x12;
    if (osfx_hs_classify_dispatch(&ctx, pkt, 2U, &res) != 0 || res.reject != OSFX_HS_REJECT_MALFORMED) {
        printf("[FAIL] hs malformed ping not rejected\n");
        return 0;
    }

    pkt[0] = OSFX_CMD_TIME_REQUEST;
    pkt[1] = 0x00;
    pkt[2] = 0x03;
    if (!osfx_hs_classify_dispatch(&ctx, pkt, 3U, &res) || !res.has_response || res.response[0] != OSFX_CMD_TIME_RESPONSE) {
        printf("[FAIL] hs time response\n");
        return 0;
    }

    pkt[0] = 0xFE;
    if (osfx_hs_classify_dispatch(&ctx, pkt, 1U, &res) != 0 || res.kind != OSFX_HS_KIND_UNKNOWN) {
        printf("[FAIL] hs unknown cmd classification\n");
        return 0;
    }
    return 1;
}

typedef struct test_service_ctx {
    int init_count;
    int tick_count;
    int close_count;
    int reload_count;
    int fail_first_init;
    char last_cfg[64];
} test_service_ctx;

static int svc_init(void* instance, const char* config) {
    test_service_ctx* ctx = (test_service_ctx*)instance;
    if (!ctx) {
        return 0;
    }
    ctx->init_count++;
    if (config) {
        strncpy(ctx->last_cfg, config, sizeof(ctx->last_cfg) - 1U);
        ctx->last_cfg[sizeof(ctx->last_cfg) - 1U] = '\0';
    }
    if (ctx->fail_first_init && ctx->init_count == 1) {
        return 0;
    }
    return 1;
}

static int svc_tick(void* instance, uint64_t now_ts) {
    test_service_ctx* ctx = (test_service_ctx*)instance;
    (void)now_ts;
    if (!ctx) {
        return 0;
    }
    ctx->tick_count++;
    return 1;
}

static int svc_close(void* instance) {
    test_service_ctx* ctx = (test_service_ctx*)instance;
    if (!ctx) {
        return 0;
    }
    ctx->close_count++;
    return 1;
}

static int svc_reload(void* instance, const char* config) {
    test_service_ctx* ctx = (test_service_ctx*)instance;
    if (!ctx) {
        return 0;
    }
    ctx->reload_count++;
    if (config) {
        strncpy(ctx->last_cfg, config, sizeof(ctx->last_cfg) - 1U);
        ctx->last_cfg[sizeof(ctx->last_cfg) - 1U] = '\0';
    }
    return 1;
}

static int svc_cmd(void* instance, const char* cmd, const char* args, char* out, size_t out_cap) {
    test_service_ctx* ctx = (test_service_ctx*)instance;
    int n;
    if (!ctx || !cmd || !out || out_cap == 0U) {
        return 0;
    }
    n = snprintf(out, out_cap, "cmd=%s args=%s cfg=%s", cmd, args ? args : "", ctx->last_cfg);
    return (n > 0 && (size_t)n < out_cap) ? 1 : 0;
}

static int test_service_runtime_p2(void) {
    osfx_service_runtime rt;
    osfx_service_status st;
    char name_buf[32];
    char cmd_out[128];
    test_service_ctx a;
    test_service_ctx b;
    test_service_ctx c;

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    memset(&c, 0, sizeof(c));
    b.fail_first_init = 1;

    osfx_service_runtime_init(&rt);
    if (!osfx_service_register_ex(&rt, "svc_a", &a, svc_init, svc_tick, svc_close, svc_reload, svc_cmd, "mode=A")) {
        return 0;
    }
    if (!osfx_service_register(&rt, "svc_b", &b, svc_init, svc_tick, svc_close, NULL, NULL)) {
        return 0;
    }
    if (!osfx_service_register(&rt, "svc_c", &c, svc_init, svc_tick, svc_close, NULL, NULL)) {
        return 0;
    }
    if (!osfx_service_set_enabled(&rt, "svc_c", 0)) {
        return 0;
    }

    if (osfx_service_count(&rt) != 3U) {
        printf("[FAIL] service list count mismatch\n");
        return 0;
    }
    if (!osfx_service_name_at(&rt, 1U, name_buf, sizeof(name_buf)) || strcmp(name_buf, "svc_b") != 0) {
        printf("[FAIL] service list index mismatch name=%s\n", name_buf);
        return 0;
    }
    if (osfx_service_init_all(&rt, "default=1") != 0) {
        printf("[FAIL] expected init_all partial failure for svc_b\n");
        return 0;
    }
    if (strcmp(a.last_cfg, "mode=A") != 0 || strcmp(b.last_cfg, "default=1") != 0) {
        printf("[FAIL] service config injection mismatch a=%s b=%s\n", a.last_cfg, b.last_cfg);
        return 0;
    }

    if (!osfx_service_reload(&rt, "svc_b", "mode=B")) {
        printf("[FAIL] service reload via re-init fallback failed\n");
        return 0;
    }
    if (!osfx_service_command(&rt, "svc_a", "status", "verbose", cmd_out, sizeof(cmd_out))) {
        printf("[FAIL] service command dispatch failed\n");
        return 0;
    }
    if (strstr(cmd_out, "cmd=status") == NULL || strstr(cmd_out, "cfg=mode=A") == NULL) {
        printf("[FAIL] service command output mismatch out=%s\n", cmd_out);
        return 0;
    }
    if (!osfx_service_tick_all(&rt, 1234U)) {
        return 0;
    }
    if (a.tick_count != 1 || b.tick_count != 1) {
        printf("[FAIL] service tick count mismatch a=%d b=%d\n", a.tick_count, b.tick_count);
        return 0;
    }

    if (!osfx_service_set_enabled(&rt, "svc_a", 0)) {
        return 0;
    }
    if (!osfx_service_tick_all(&rt, 1235U)) {
        return 0;
    }
    if (a.tick_count != 1 || b.tick_count != 2) {
        printf("[FAIL] service enable gating mismatch a=%d b=%d\n", a.tick_count, b.tick_count);
        return 0;
    }

    if (!osfx_service_set_enabled(&rt, "svc_c", 1)) {
        return 0;
    }
    if (!osfx_service_load(&rt, "svc_c", "mode=C")) {
        printf("[FAIL] service load by name failed\n");
        return 0;
    }
    if (strcmp(c.last_cfg, "mode=C") != 0) {
        printf("[FAIL] service load config mismatch cfg=%s\n", c.last_cfg);
        return 0;
    }

    if (!osfx_service_get_status(&rt, "svc_b", &st) || !st.running || st.reload_count == 0U) {
        printf("[FAIL] service status mismatch running=%d reload=%u\n", st.running, st.reload_count);
        return 0;
    }
    if (!osfx_service_get_status(&rt, "svc_a", &st) || st.cmd_count == 0U) {
        printf("[FAIL] service cmd status mismatch cmd_count=%u\n", st.cmd_count);
        return 0;
    }

    if (!osfx_service_close_all(&rt)) {
        return 0;
    }
    if (b.close_count == 0) {
        printf("[FAIL] service close count mismatch\n");
        return 0;
    }
    return 1;
}

typedef struct test_pf_emit_ctx {
    int calls;
    char proto[16];
    uint16_t port;
    size_t payload_len;
} test_pf_emit_ctx;

static int pf_emit_capture(const char* to_proto, uint16_t to_port, const uint8_t* payload, size_t payload_len, void* user_ctx) {
    test_pf_emit_ctx* ctx = (test_pf_emit_ctx*)user_ctx;
    (void)payload;
    if (!ctx || !to_proto) {
        return 0;
    }
    ctx->calls++;
    strncpy(ctx->proto, to_proto, sizeof(ctx->proto) - 1U);
    ctx->proto[sizeof(ctx->proto) - 1U] = '\0';
    ctx->port = to_port;
    ctx->payload_len = payload_len;
    return 1;
}

static int test_platform_plugins_scoped_and_cli_lite(void) {
    osfx_protocol_matrix pm;
    test_matrix_ctx matrix_ctx;
    osfx_platform_runtime platform;
    test_pf_emit_ctx pf_ctx;
    char out[512];
    const char* cmd_list[] = {"plugin-list"};
    const char* cmd_load_transport[] = {"plugin-load", "transport"};
    const char* cmd_load_test[] = {"plugin-load", "test_plugin"};
    const char* cmd_load_pf[] = {"plugin-load", "port_forwarder"};
    const char* cmd_transport_dispatch[] = {"plugin-cmd", "transport", "dispatch", "auto", "010203"};
    const char* cmd_test_run[] = {"test-plugin", "run", "component"};
    const char* cmd_pf_add[] = {"port-forwarder", "add-rule", "r1", "udp", "8080", "tcp", "9000"};
    const char* cmd_pf_forward[] = {"port-forwarder", "forward", "udp", "8080", "A1B2"};

    memset(&matrix_ctx, 0, sizeof(matrix_ctx));
    memset(&pf_ctx, 0, sizeof(pf_ctx));

    osfx_protocol_matrix_init(&pm, matrix_emit, &matrix_ctx);
    if (!osfx_protocol_matrix_register_defaults(&pm)) {
        printf("[FAIL] platform test protocol defaults\n");
        return 0;
    }

    osfx_platform_runtime_init(&platform, &pm, pf_emit_capture, &pf_ctx);
    if (osfx_platform_plugin_count(&platform) != 3U) {
        printf("[FAIL] scoped plugin count mismatch\n");
        return 0;
    }

    if (!osfx_cli_lite_run(&platform, 1, cmd_list, out, sizeof(out))) {
        printf("[FAIL] cli plugin-list failed\n");
        return 0;
    }
    if (strstr(out, "transport") == NULL || strstr(out, "test_plugin") == NULL || strstr(out, "port_forwarder") == NULL) {
        printf("[FAIL] cli plugin-list missing scoped plugins out=%s\n", out);
        return 0;
    }
    if (strstr(out, "web") != NULL || strstr(out, "dependency_manager") != NULL || strstr(out, "env_guard") != NULL) {
        printf("[FAIL] cli plugin-list contains excluded plugins out=%s\n", out);
        return 0;
    }

    if (!osfx_cli_lite_run(&platform, 2, cmd_load_transport, out, sizeof(out))) {
        printf("[FAIL] cli load transport failed\n");
        return 0;
    }
    if (!osfx_cli_lite_run(&platform, 2, cmd_load_test, out, sizeof(out))) {
        printf("[FAIL] cli load test_plugin failed\n");
        return 0;
    }
    if (!osfx_cli_lite_run(&platform, 2, cmd_load_pf, out, sizeof(out))) {
        printf("[FAIL] cli load port_forwarder failed\n");
        return 0;
    }

    if (!osfx_cli_lite_run(&platform, 5, cmd_transport_dispatch, out, sizeof(out))) {
        printf("[FAIL] cli transport dispatch failed out=%s\n", out);
        return 0;
    }
    if (matrix_ctx.udp_seen < 1) {
        printf("[FAIL] transport plugin dispatch not emitted\n");
        return 0;
    }

    if (!osfx_cli_lite_run(&platform, 3, cmd_test_run, out, sizeof(out)) || strstr(out, "ok=1") == NULL) {
        printf("[FAIL] cli test-plugin run failed out=%s\n", out);
        return 0;
    }

    if (!osfx_cli_lite_run(&platform, 7, cmd_pf_add, out, sizeof(out))) {
        printf("[FAIL] cli port-forwarder add-rule failed out=%s\n", out);
        return 0;
    }
    if (!osfx_cli_lite_run(&platform, 5, cmd_pf_forward, out, sizeof(out))) {
        printf("[FAIL] cli port-forwarder forward failed out=%s\n", out);
        return 0;
    }
    if (pf_ctx.calls != 1 || strcmp(pf_ctx.proto, "tcp") != 0 || pf_ctx.port != 9000U || pf_ctx.payload_len != 2U) {
        printf("[FAIL] port_forwarder route mismatch calls=%d proto=%s port=%u len=%llu\n",
               pf_ctx.calls,
               pf_ctx.proto,
               (unsigned)pf_ctx.port,
               (unsigned long long)pf_ctx.payload_len);
        return 0;
    }

    return 1;
}

static int test_glue_layer_chain(void) {
    osfx_glue_ctx glue;
    osfx_glue_config cfg;
    osfx_hs_result hs;
    uint8_t id_req[3] = {OSFX_CMD_ID_REQUEST, 0x12, 0x34};
    char cmd_out[128];
    uint8_t packet[256];
    int packet_len = 0;
    uint8_t cmd = 0;
    char sensor_id[32];
    char unit[16];
    double value = 0.0;
    osfx_packet_meta meta;

    osfx_glue_default_config(&cfg);
    cfg.local_aid = 500U;
    cfg.id_start = 200U;
    cfg.id_end = 202U;
    cfg.id_default_lease_seconds = 120U;

    if (osfx_glue_init(&glue, &cfg) != OSFX_GLUE_OK) {
        printf("[FAIL] glue init failed\n");
        return 0;
    }

    if (osfx_glue_process_packet(&glue, id_req, sizeof(id_req), 1000U, &hs) != OSFX_GLUE_OK) {
        printf("[FAIL] glue process id request failed\n");
        return 0;
    }
    if (!hs.has_response || hs.response_len < 7U || hs.response[0] != OSFX_CMD_ID_ASSIGN) {
        printf("[FAIL] glue id assign response invalid\n");
        return 0;
    }

    if (osfx_glue_plugin_cmd(&glue, "test_plugin", "run", "component", cmd_out, sizeof(cmd_out)) != OSFX_GLUE_OK) {
        printf("[FAIL] glue plugin cmd failed\n");
        return 0;
    }
    if (strstr(cmd_out, "ok=1") == NULL) {
        printf("[FAIL] glue plugin cmd output mismatch out=%s\n", cmd_out);
        return 0;
    }

    if (osfx_glue_encode_sensor_auto(&glue, 1U, 1710003000ULL, "TEMP", 1.0, "kPa", packet, sizeof(packet), &packet_len, &cmd) != OSFX_GLUE_OK) {
        printf("[FAIL] glue encode sensor auto failed\n");
        return 0;
    }
    if (osfx_glue_decode_sensor_auto(&glue, packet, (size_t)packet_len, sensor_id, sizeof(sensor_id), &value, unit, sizeof(unit), &meta) != OSFX_GLUE_OK) {
        printf("[FAIL] glue decode sensor auto failed\n");
        return 0;
    }
    if (strcmp(sensor_id, "TEMP") != 0 || strcmp(unit, "Pa") != 0 || value != 1000.0) {
        printf("[FAIL] glue decoded payload mismatch id=%s unit=%s value=%f\n", sensor_id, unit, value);
        return 0;
    }
    return 1;
}

int main(void) {
    int ok = 1;
    ok = ok && test_crc16_vector();
    ok = ok && test_b62_roundtrip();
    ok = ok && test_packet_full_min_decode();
    ok = ok && test_library_catalog_generated();
    ok = ok && test_standardization_basic();
    ok = ok && test_handshake_builders();
    ok = ok && test_fusion_state_transitions();
    ok = ok && test_template_grammar();
    ok = ok && test_multi_sensor_payload_switches();
    ok = ok && test_transporter_runtime();
    ok = ok && test_secure_session_store();
    ok = ok && test_secure_payload_pipeline();
    ok = ok && test_id_allocator_persistence();
    ok = ok && test_id_allocator_storage_backend();
    ok = ok && test_protocol_matrix_runtime();
    ok = ok && test_handshake_dispatch_flow();
    ok = ok && test_service_runtime_p2();
    ok = ok && test_platform_plugins_scoped_and_cli_lite();
    ok = ok && test_glue_layer_chain();

    if (!ok) {
        return 1;
    }
    printf("[PASS] osfx-c99 native tests passed\n");
    return 0;
}

