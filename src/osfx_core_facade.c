#include "../include/osfx_core.h"
#include "../include/osfx_build_config.h"

#include <stdio.h>
#include <string.h>

#define OSFX_VALUE_SCALE 10000.0

#ifndef OSFX_CFG_MULTI_SENSOR_BODY_CAP
#define OSFX_CFG_MULTI_SENSOR_BODY_CAP 4096
#endif

#ifndef OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH
#define OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH 0
#endif

#if OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH
/*
 * Stack-safe mode for constrained boards:
 * use static scratch blocks instead of per-call 4KB+ local arrays.
 */
static osfx_sensor_slot g_multi_sensor_slots[OSFX_TMPL_MAX_SENSORS];
static char g_multi_sensor_encode_body[OSFX_CFG_MULTI_SENSOR_BODY_CAP];
static uint8_t g_multi_sensor_decode_body[OSFX_CFG_MULTI_SENSOR_BODY_CAP];
static osfx_template_msg g_multi_sensor_decode_msg;
#endif

static int copy_text(char* out, size_t cap, const char* src) {
    size_t n;
    if (!out || cap == 0 || !src) {
        return 0;
    }
    n = strlen(src);
    if (n + 1 > cap) {
        return 0;
    }
    memcpy(out, src, n + 1);
    return 1;
}

int osfx_core_encode_sensor_packet(
    uint32_t source_aid,
    uint8_t tid,
    uint64_t timestamp_raw,
    const char* sensor_id,
    double input_value,
    const char* input_unit,
    uint8_t* out_packet,
    size_t out_packet_cap,
    int* out_packet_len
) {
    double std_value = 0.0;
    char std_unit[16];
    char value_b62[64];
    char aid_str[11];
    char ts_b64[9];
    char body[256];
    long long scaled;
    size_t sid_len;
    size_t unit_len;
    size_t b62_len;
    size_t aid_len;
    size_t body_len;
    int pkt_len;
    int wlen;
    size_t off;

    if (!sensor_id || !input_unit || !out_packet || !out_packet_len) {
        return 0;
    }

    if (!osfx_standardize_value(input_value, input_unit, &std_value, std_unit, sizeof(std_unit))) {
        return 0;
    }

    scaled = (long long)(std_value * OSFX_VALUE_SCALE);
    if (!osfx_b62_encode_i64(scaled, value_b62, sizeof(value_b62))) {
        return 0;
    }

    wlen = osfx_u32toa(source_aid, aid_str, sizeof(aid_str));
    if (wlen <= 0) { return 0; }
    aid_len = (size_t)wlen;

    osfx_b64url_ts6(timestamp_raw, ts_b64);

    sid_len  = strlen(sensor_id);
    unit_len = strlen(std_unit);
    b62_len  = strlen(value_b62);

    /*
     * body = "{aid}.U.{ts_b64}|{sid}>U.{unit}:{b62}|"
     * OpenSynaptic wire body format.
     */
    body_len = aid_len + 3U        /* ".U." */
             + 8U                  /* ts_b64 */
             + 1U                  /* '|' header sentinel */
             + sid_len + 3U        /* sid + ">U." */
             + unit_len + 1U       /* unit + ':' */
             + b62_len + 1U;       /* b62 + '|' trailer */

    if (body_len + 1U > sizeof(body)) { return 0; }

    off = 0;
    /* Header segment: "{aid}.U.{ts_b64}|" */
    memcpy(body + off, aid_str, aid_len); off += aid_len;
    body[off++] = '.'; body[off++] = 'U'; body[off++] = '.';
    memcpy(body + off, ts_b64, 8);        off += 8;
    body[off++] = '|';
    /* Sensor segment: "{sid}>U.{unit}:{b62}|" */
    memcpy(body + off, sensor_id, sid_len); off += sid_len;
    body[off++] = '>'; body[off++] = 'U'; body[off++] = '.';
    memcpy(body + off, std_unit, unit_len); off += unit_len;
    body[off++] = ':';
    memcpy(body + off, value_b62, b62_len); off += b62_len;
    body[off++] = '|';

    pkt_len = osfx_packet_encode_full(
        (uint8_t)OSFX_CMD_DATA_FULL,
        source_aid,
        tid,
        timestamp_raw,
        (const uint8_t*)body,
        body_len,
        out_packet,
        out_packet_cap
    );

    if (pkt_len <= 0) {
        return 0;
    }

    *out_packet_len = pkt_len;
    return 1;
}

int osfx_core_decode_sensor_body(
    const uint8_t* body,
    size_t body_len,
    char* out_sensor_id,
    size_t out_sensor_id_cap,
    double* out_value,
    char* out_unit,
    size_t out_unit_cap
) {
    size_t p;
    size_t sid_start;
    size_t gt_pos;   /* position of '>'            */
    size_t dot_pos;  /* position of '.' after state */
    size_t col_pos;  /* position of ':'             */
    size_t end_pos;  /* position of trailing '|'    */
    size_t seg_len;
    char tmp[64];
    int ok = 0;
    long long scaled;

    if (!body || body_len == 0 || !out_sensor_id || !out_value || !out_unit) {
        return 0;
    }

    /*
     * Wire body format: "{aid}.{status}.{ts_b64}|{sid}>{state}.{unit}:{b62}|"
     *
     * Step 1: Skip body header - find first '|'.
     */
    p = 0;
    while (p < body_len && body[p] != (uint8_t)'|') { ++p; }
    if (p >= body_len) { return 0; }
    ++p; /* now at first char of sensor segment */

    sid_start = p;

    /* Step 2: Find '>' - end of sensor_id. */
    gt_pos = p;
    while (gt_pos < body_len && body[gt_pos] != (uint8_t)'>') { ++gt_pos; }
    if (gt_pos >= body_len || gt_pos <= sid_start) { return 0; }

    /* --- sensor_id --------------------------------------------------- */
    seg_len = gt_pos - sid_start;
    if (seg_len == 0 || seg_len + 1U > out_sensor_id_cap) { return 0; }
    memcpy(out_sensor_id, body + sid_start, seg_len);
    out_sensor_id[seg_len] = '\0';

    /* Step 3: Skip state - find '.' after '>'. */
    dot_pos = gt_pos + 1U;
    while (dot_pos < body_len && body[dot_pos] != (uint8_t)'.') { ++dot_pos; }
    if (dot_pos >= body_len) { return 0; }

    /* Step 4: Find ':' - end of unit code. */
    col_pos = dot_pos + 1U;
    while (col_pos < body_len && body[col_pos] != (uint8_t)':') { ++col_pos; }
    if (col_pos >= body_len) { return 0; }

    /* --- unit -------------------------------------------------------- */
    seg_len = col_pos - (dot_pos + 1U);
    if (seg_len == 0 || seg_len + 1U > out_unit_cap) { return 0; }
    memcpy(out_unit, body + dot_pos + 1U, seg_len);
    out_unit[seg_len] = '\0';

    /* Step 5: Find trailing '|' - end of b62 value. */
    end_pos = col_pos + 1U;
    while (end_pos < body_len && body[end_pos] != (uint8_t)'|') { ++end_pos; }

    /* --- b62 value --------------------------------------------------- */
    seg_len = end_pos - (col_pos + 1U);
    if (seg_len == 0 || seg_len >= sizeof(tmp)) { return 0; }
    memcpy(tmp, body + col_pos + 1U, seg_len);
    tmp[seg_len] = '\0';

    scaled = osfx_b62_decode_i64(tmp, &ok);
    if (!ok) { return 0; }

    *out_value = (double)scaled / OSFX_VALUE_SCALE;
    return 1;
}

int osfx_core_encode_sensor_packet_auto(
    osfx_fusion_state* st,
    uint32_t source_aid,
    uint8_t tid,
    uint64_t timestamp_raw,
    const char* sensor_id,
    double input_value,
    const char* input_unit,
    uint8_t* out_packet,
    size_t out_packet_cap,
    int* out_packet_len,
    uint8_t* out_cmd
) {
    double std_value = 0.0;
    char std_unit[OSFX_TMPL_UNIT_MAX];
    char value_b62[OSFX_TMPL_VALUE_MAX];
    char ts_token[OSFX_TMPL_TS_MAX];
    char aid_str[11];
    osfx_sensor_slot slot;
    char body[256];
    long long scaled;
    int wlen;

    if (!st || !sensor_id || !input_unit || !out_packet || !out_packet_len) {
        return 0;
    }

    if (!osfx_standardize_value(input_value, input_unit, &std_value, std_unit, sizeof(std_unit))) {
        return 0;
    }

    scaled = (long long)(std_value * OSFX_VALUE_SCALE);
    if (!osfx_b62_encode_i64(scaled, value_b62, sizeof(value_b62))) {
        return 0;
    }

    /* AID decimal string + base64url timestamp (OpenSynaptic wire format) */
    wlen = osfx_u32toa(source_aid, aid_str, sizeof(aid_str));
    if (wlen <= 0) { return 0; }
    osfx_b64url_ts6(timestamp_raw, ts_token);

    memset(&slot, 0, sizeof(slot));
    if (!copy_text(slot.sensor_id, sizeof(slot.sensor_id), sensor_id)) {
        return 0;
    }
    if (!copy_text(slot.sensor_state, sizeof(slot.sensor_state), "U")) {
        return 0;
    }
    if (!copy_text(slot.sensor_unit, sizeof(slot.sensor_unit), std_unit)) {
        return 0;
    }
    if (!copy_text(slot.sensor_value, sizeof(slot.sensor_value), value_b62)) {
        return 0;
    }

    if (!osfx_template_encode(aid_str, "U", ts_token, &slot, 1, body, sizeof(body))) {
        return 0;
    }

    return osfx_fusion_encode(
        st,
        source_aid,
        tid,
        timestamp_raw,
        (const uint8_t*)body,
        strlen(body),
        out_packet,
        out_packet_cap,
        out_packet_len,
        out_cmd
    );
}

int osfx_core_decode_sensor_packet_auto(
    osfx_fusion_state* st,
    const uint8_t* packet,
    size_t packet_len,
    char* out_sensor_id,
    size_t out_sensor_id_cap,
    double* out_value,
    char* out_unit,
    size_t out_unit_cap,
    osfx_packet_meta* out_meta
) {
    uint8_t body[256];
    size_t body_len = 0;
    osfx_template_msg msg;
    int ok = 0;
    long long scaled;

    if (!st || !packet || packet_len == 0) {
        return 0;
    }
    if (!osfx_fusion_decode_apply(st, packet, packet_len, body, sizeof(body), &body_len, out_meta)) {
        return 0;
    }
    if (body_len + 1U > sizeof(body)) {
        return 0;
    }
    body[body_len] = '\0';

    if (!osfx_template_decode((const char*)body, &msg) || msg.sensor_count == 0) {
        return 0;
    }

    if (out_sensor_id && out_sensor_id_cap > 0) {
        if (!copy_text(out_sensor_id, out_sensor_id_cap, msg.sensors[0].sensor_id)) {
            return 0;
        }
    }
    if (out_unit && out_unit_cap > 0) {
        if (!copy_text(out_unit, out_unit_cap, msg.sensors[0].sensor_unit)) {
            return 0;
        }
    }
    if (out_value) {
        scaled = osfx_b62_decode_i64(msg.sensors[0].sensor_value, &ok);
        if (!ok) {
            return 0;
        }
        *out_value = (double)scaled / OSFX_VALUE_SCALE;
    }
    return 1;
}

int osfx_core_encode_sensor_packet_secure(
    const osfx_secure_session_store* secure_store,
    uint32_t source_aid,
    uint8_t tid,
    uint64_t timestamp_raw,
    const char* sensor_id,
    double input_value,
    const char* input_unit,
    uint8_t* out_packet,
    size_t out_packet_cap,
    int* out_packet_len
) {
    uint8_t key[OSFX_KEY_LEN];
    double std_value = 0.0;
    char std_unit[16];
    char value_b62[64];
    char aid_str[11];
    char ts_b64[9];
    char body[256];
    long long scaled;
    size_t sid_len;
    size_t unit_len;
    size_t b62_len;
    size_t aid_len;
    size_t body_len;
    int pkt_len;
    int wlen;
    size_t off;

    if (!secure_store || !sensor_id || !input_unit || !out_packet || !out_packet_len) {
        return 0;
    }
    if (!osfx_secure_get_key(secure_store, source_aid, key)) {
        return 0;
    }
    if (!osfx_standardize_value(input_value, input_unit, &std_value, std_unit, sizeof(std_unit))) {
        return 0;
    }
    scaled = (long long)(std_value * OSFX_VALUE_SCALE);
    if (!osfx_b62_encode_i64(scaled, value_b62, sizeof(value_b62))) {
        return 0;
    }

    wlen = osfx_u32toa(source_aid, aid_str, sizeof(aid_str));
    if (wlen <= 0) { return 0; }
    aid_len = (size_t)wlen;

    osfx_b64url_ts6(timestamp_raw, ts_b64);

    sid_len  = strlen(sensor_id);
    unit_len = strlen(std_unit);
    b62_len  = strlen(value_b62);

    body_len = aid_len + 3U + 8U + 1U + sid_len + 3U + unit_len + 1U + b62_len + 1U;
    if (body_len + 1U > sizeof(body)) { return 0; }

    off = 0;
    memcpy(body + off, aid_str, aid_len); off += aid_len;
    body[off++] = '.'; body[off++] = 'U'; body[off++] = '.';
    memcpy(body + off, ts_b64, 8);        off += 8;
    body[off++] = '|';
    memcpy(body + off, sensor_id, sid_len); off += sid_len;
    body[off++] = '>'; body[off++] = 'U'; body[off++] = '.';
    memcpy(body + off, std_unit, unit_len); off += unit_len;
    body[off++] = ':';
    memcpy(body + off, value_b62, b62_len); off += b62_len;
    body[off++] = '|';

    pkt_len = osfx_packet_encode_ex(
        (uint8_t)OSFX_CMD_DATA_FULL,
        source_aid,
        tid,
        timestamp_raw,
        (const uint8_t*)body,
        body_len,
        1,
        key,
        OSFX_KEY_LEN,
        out_packet,
        out_packet_cap
    );
    if (pkt_len <= 0) {
        return 0;
    }
    *out_packet_len = pkt_len;
    return 1;
}

int osfx_core_decode_sensor_packet_secure(
    const osfx_secure_session_store* secure_store,
    const uint8_t* packet,
    size_t packet_len,
    char* out_sensor_id,
    size_t out_sensor_id_cap,
    double* out_value,
    char* out_unit,
    size_t out_unit_cap,
    osfx_packet_meta* out_meta
) {
    osfx_packet_meta meta;
    uint8_t key[OSFX_KEY_LEN];
    uint8_t body[192];
    size_t body_len = 0;

    if (!secure_store || !packet || packet_len == 0) {
        return 0;
    }
    if (!osfx_packet_decode_meta(packet, packet_len, &meta)) {
        return 0;
    }
    if (!osfx_secure_get_key(secure_store, meta.source_aid, key)) {
        return 0;
    }
    if (!osfx_packet_extract_body(packet, packet_len, key, OSFX_KEY_LEN, body, sizeof(body), &body_len, &meta)) {
        return 0;
    }
    if (out_meta) {
        *out_meta = meta;
    }
    return osfx_core_decode_sensor_body(body, body_len, out_sensor_id, out_sensor_id_cap, out_value, out_unit, out_unit_cap);
}

int osfx_core_encode_multi_sensor_packet_auto(
    osfx_fusion_state* st,
    uint32_t source_aid,
    uint8_t tid,
    uint64_t timestamp_raw,
    const char* node_id,
    const char* node_state,
    const osfx_core_sensor_input* sensors,
    size_t sensor_count,
    uint8_t* out_packet,
    size_t out_packet_cap,
    int* out_packet_len,
    uint8_t* out_cmd
) {
    osfx_sensor_slot* slots;
    char* body;
    const size_t body_cap = (size_t)OSFX_CFG_MULTI_SENSOR_BODY_CAP;
#if !OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH
    osfx_sensor_slot slots_local[OSFX_TMPL_MAX_SENSORS];
    char body_local[OSFX_CFG_MULTI_SENSOR_BODY_CAP];
#endif
    char ts_token[OSFX_TMPL_TS_MAX];
    size_t i;

    uint8_t effective_tid = tid;
    const char* effective_node_id = node_id;
    const char* effective_node_state = node_state;
    const char* effective_ts_token = ts_token;

    if (!st || !node_id || !node_state || !sensors || !out_packet || !out_packet_len) {
        return 0;
    }
    if (sensor_count == 0 || sensor_count > OSFX_TMPL_MAX_SENSORS) {
        return 0;
    }

#if OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH
    slots = g_multi_sensor_slots;
    body = g_multi_sensor_encode_body;
#else
    slots = slots_local;
    body = body_local;
#endif

    #if OSFX_CFG_PAYLOAD_SUB_TEMPLATE_ID == 0
    effective_tid = 0U;
    #endif

    #if OSFX_CFG_PAYLOAD_DEVICE_ID == 0
    effective_node_id = "NODE";
    #endif

    #if OSFX_CFG_PAYLOAD_DEVICE_STATUS == 0
    effective_node_state = "NA";
    #endif

    memset(slots, 0, sizeof(osfx_sensor_slot) * OSFX_TMPL_MAX_SENSORS);
    for (i = 0; i < sensor_count; ++i) {
        double std_value = 0.0;
        char std_unit[OSFX_TMPL_UNIT_MAX];
        char val_b62[OSFX_TMPL_VALUE_MAX];
        long long scaled;

        if (!sensors[i].sensor_id || !sensors[i].sensor_state || !sensors[i].unit) {
            return 0;
        }
        #if OSFX_CFG_PAYLOAD_SENSOR_ID == 0
        if (!copy_text(slots[i].sensor_id, sizeof(slots[i].sensor_id), "S")) {
            return 0;
        }
        #else
        if (!copy_text(slots[i].sensor_id, sizeof(slots[i].sensor_id), sensors[i].sensor_id)) {
            return 0;
        }
        #endif

        #if OSFX_CFG_PAYLOAD_SENSOR_STATUS == 0
        if (!copy_text(slots[i].sensor_state, sizeof(slots[i].sensor_state), "NA")) {
            return 0;
        }
        #else
        if (!copy_text(slots[i].sensor_state, sizeof(slots[i].sensor_state), sensors[i].sensor_state)) {
            return 0;
        }
        #endif

        if (!osfx_standardize_value(sensors[i].value, sensors[i].unit, &std_value, std_unit, sizeof(std_unit))) {
            return 0;
        }

        #if OSFX_CFG_PAYLOAD_PHYSICAL_ATTRIBUTE == 0
        if (!copy_text(slots[i].sensor_unit, sizeof(slots[i].sensor_unit), "NA")) {
            return 0;
        }
        #else
        if (!copy_text(slots[i].sensor_unit, sizeof(slots[i].sensor_unit), std_unit)) {
            return 0;
        }
        #endif

        #if OSFX_CFG_PAYLOAD_NORMALIZED_VALUE == 0
        if (!copy_text(slots[i].sensor_value, sizeof(slots[i].sensor_value), "0")) {
            return 0;
        }
        #else
        scaled = (long long)(std_value * OSFX_VALUE_SCALE);
        if (!osfx_b62_encode_i64(scaled, val_b62, sizeof(val_b62))) {
            return 0;
        }
        if (!copy_text(slots[i].sensor_value, sizeof(slots[i].sensor_value), val_b62)) {
            return 0;
        }
        #endif

        #if OSFX_CFG_PAYLOAD_GEOHASH_ID
        if (!copy_text(slots[i].geohash_id, sizeof(slots[i].geohash_id), sensors[i].geohash_id ? sensors[i].geohash_id : "")) {
            return 0;
        }
        #endif

        #if OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE
        if (!copy_text(slots[i].supplementary_message, sizeof(slots[i].supplementary_message), sensors[i].supplementary_message ? sensors[i].supplementary_message : "")) {
            return 0;
        }
        #endif

        #if OSFX_CFG_PAYLOAD_RESOURCE_URL
        if (!copy_text(slots[i].resource_url, sizeof(slots[i].resource_url), sensors[i].resource_url ? sensors[i].resource_url : "")) {
            return 0;
        }
        #endif
    }

    if (snprintf(ts_token, sizeof(ts_token), "%llu", (unsigned long long)timestamp_raw) <= 0) {
        return 0;
    }
    #if OSFX_CFG_PAYLOAD_TIMESTAMP == 0
    effective_ts_token = "0";
    #endif

    if (!osfx_template_encode(effective_node_id, effective_node_state, effective_ts_token, slots, sensor_count, body, body_cap)) {
        return 0;
    }

    return osfx_fusion_encode(
        st,
        source_aid,
        effective_tid,
        timestamp_raw,
        (const uint8_t*)body,
        strlen(body),
        out_packet,
        out_packet_cap,
        out_packet_len,
        out_cmd
    );
}

int osfx_core_decode_multi_sensor_packet_auto(
    osfx_fusion_state* st,
    const uint8_t* packet,
    size_t packet_len,
    char* out_node_id,
    size_t out_node_id_cap,
    char* out_node_state,
    size_t out_node_state_cap,
    osfx_core_sensor_output* out_sensors,
    size_t out_sensors_cap,
    size_t* out_sensor_count,
    osfx_packet_meta* out_meta
) {
    uint8_t* body;
    osfx_template_msg* msg;
    const size_t body_cap = (size_t)OSFX_CFG_MULTI_SENSOR_BODY_CAP;
#if !OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH
    uint8_t body_local[OSFX_CFG_MULTI_SENSOR_BODY_CAP];
    osfx_template_msg msg_local;
#endif
    size_t body_len = 0;
    size_t i;

    if (!st || !packet || !out_node_id || !out_node_state || !out_sensors || !out_sensor_count) {
        return 0;
    }

#if OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH
    body = g_multi_sensor_decode_body;
    msg = &g_multi_sensor_decode_msg;
#else
    body = body_local;
    msg = &msg_local;
#endif

    if (!osfx_fusion_decode_apply(st, packet, packet_len, body, body_cap, &body_len, out_meta)) {
        return 0;
    }
    if (body_len + 1 > body_cap) {
        return 0;
    }
    body[body_len] = '\0';

    if (!osfx_template_decode((const char*)body, msg)) {
        return 0;
    }
    if (!copy_text(out_node_id, out_node_id_cap, msg->node_id)) {
        return 0;
    }
    if (!copy_text(out_node_state, out_node_state_cap, msg->node_state)) {
        return 0;
    }
    if (msg->sensor_count > out_sensors_cap) {
        return 0;
    }

    for (i = 0; i < msg->sensor_count; ++i) {
        int ok = 0;
        long long scaled = osfx_b62_decode_i64(msg->sensors[i].sensor_value, &ok);
        if (!ok) {
            return 0;
        }
        memset(&out_sensors[i], 0, sizeof(out_sensors[i]));
        if (!copy_text(out_sensors[i].sensor_id, sizeof(out_sensors[i].sensor_id), msg->sensors[i].sensor_id)) {
            return 0;
        }
        if (!copy_text(out_sensors[i].sensor_state, sizeof(out_sensors[i].sensor_state), msg->sensors[i].sensor_state)) {
            return 0;
        }
        if (!copy_text(out_sensors[i].unit, sizeof(out_sensors[i].unit), msg->sensors[i].sensor_unit)) {
            return 0;
        }
#if OSFX_CFG_PAYLOAD_GEOHASH_ID
        if (!copy_text(out_sensors[i].geohash_id, sizeof(out_sensors[i].geohash_id), msg->sensors[i].geohash_id)) {
            return 0;
        }
#endif
#if OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE
        if (!copy_text(out_sensors[i].supplementary_message, sizeof(out_sensors[i].supplementary_message), msg->sensors[i].supplementary_message)) {
            return 0;
        }
#endif
#if OSFX_CFG_PAYLOAD_RESOURCE_URL
        if (!copy_text(out_sensors[i].resource_url, sizeof(out_sensors[i].resource_url), msg->sensors[i].resource_url)) {
            return 0;
        }
#endif
        out_sensors[i].value = (double)scaled / OSFX_VALUE_SCALE;
    }

    *out_sensor_count = msg->sensor_count;
    return 1;
}

