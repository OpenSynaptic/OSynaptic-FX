#include "../include/osfx_template_grammar.h"

#include <stdio.h>
#include <string.h>

static const char* find_next_ext_marker(const char* p, const char* end) {
    const char* m1 = NULL;
    const char* m2 = NULL;
    const char* m3 = NULL;
    const char* m = NULL;
    if (!p || !end || p >= end) {
        return NULL;
    }
    m1 = strchr(p, '#');
    m2 = strchr(p, '!');
    m3 = strchr(p, '@');
    if (m1 && m1 < end) {
        m = m1;
    }
    if (m2 && m2 < end && (!m || m2 < m)) {
        m = m2;
    }
    if (m3 && m3 < end && (!m || m3 < m)) {
        m = m3;
    }
    return m;
}

static int copy_token(char* out, size_t cap, const char* src, size_t n) {
    if (!out || !src || cap == 0 || n + 1 > cap) {
        return 0;
    }
    memcpy(out, src, n);
    out[n] = '\0';
    return 1;
}

int osfx_template_encode(
    const char* node_id,
    const char* node_state,
    const char* ts_token,
    const osfx_sensor_slot* sensors,
    size_t sensor_count,
    char* out,
    size_t out_cap
) {
    size_t off = 0;
    size_t i;
    int n;

    if (!node_id || !node_state || !ts_token || !out || out_cap == 0) {
        return 0;
    }
    if (sensor_count > OSFX_TMPL_MAX_SENSORS) {
        return 0;
    }

    n = snprintf(out + off, out_cap - off, "%s.%s.%s|", node_id, node_state, ts_token);
    if (n < 0 || (size_t)n >= out_cap - off) {
        return 0;
    }
    off += (size_t)n;

    for (i = 0; i < sensor_count; ++i) {
        n = snprintf(
            out + off,
            out_cap - off,
            "%s>%s.%s:%s",
            sensors[i].sensor_id,
            sensors[i].sensor_state,
            sensors[i].sensor_unit,
            sensors[i].sensor_value
        );
        if (n < 0 || (size_t)n >= out_cap - off) {
            return 0;
        }
        off += (size_t)n;

#if OSFX_CFG_PAYLOAD_GEOHASH_ID
        if (sensors[i].geohash_id[0] != '\0') {
            n = snprintf(out + off, out_cap - off, "#%s", sensors[i].geohash_id);
            if (n < 0 || (size_t)n >= out_cap - off) {
                return 0;
            }
            off += (size_t)n;
        }
#endif
#if OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE
        if (sensors[i].supplementary_message[0] != '\0') {
            n = snprintf(out + off, out_cap - off, "!%s", sensors[i].supplementary_message);
            if (n < 0 || (size_t)n >= out_cap - off) {
                return 0;
            }
            off += (size_t)n;
        }
#endif
#if OSFX_CFG_PAYLOAD_RESOURCE_URL
        if (sensors[i].resource_url[0] != '\0') {
            n = snprintf(out + off, out_cap - off, "@%s", sensors[i].resource_url);
            if (n < 0 || (size_t)n >= out_cap - off) {
                return 0;
            }
            off += (size_t)n;
        }
#endif

        n = snprintf(out + off, out_cap - off, "|");
        if (n < 0 || (size_t)n >= out_cap - off) {
            return 0;
        }
        off += (size_t)n;
    }

    return 1;
}

int osfx_template_decode(const char* text, osfx_template_msg* out_msg) {
    const char* p;
    const char* seg_end;
    const char* dot1;
    const char* dot2;
    size_t count = 0;

    if (!text || !out_msg) {
        return 0;
    }
    memset(out_msg, 0, sizeof(*out_msg));

    seg_end = strchr(text, '|');
    if (!seg_end) {
        return 0;
    }

    dot1 = strchr(text, '.');
    if (!dot1 || dot1 >= seg_end) {
        return 0;
    }
    dot2 = strchr(dot1 + 1, '.');
    if (!dot2 || dot2 >= seg_end) {
        return 0;
    }

    if (!copy_token(out_msg->node_id, sizeof(out_msg->node_id), text, (size_t)(dot1 - text))) {
        return 0;
    }
    if (!copy_token(out_msg->node_state, sizeof(out_msg->node_state), dot1 + 1, (size_t)(dot2 - (dot1 + 1)))) {
        return 0;
    }
    if (!copy_token(out_msg->ts_token, sizeof(out_msg->ts_token), dot2 + 1, (size_t)(seg_end - (dot2 + 1)))) {
        return 0;
    }

    p = seg_end + 1;
    while (*p) {
        const char* end = strchr(p, '|');
        const char* gt;
        const char* dot;
        const char* colon;
        const char* ext;
        const char* cursor;
        if (!end) {
            break;
        }
        if (end == p) {
            p = end + 1;
            continue;
        }
        if (count >= OSFX_TMPL_MAX_SENSORS) {
            return 0;
        }

        gt = strchr(p, '>');
        dot = gt ? strchr(gt + 1, '.') : NULL;
        colon = dot ? strchr(dot + 1, ':') : NULL;
        if (!gt || !dot || !colon || gt >= end || dot >= end || colon >= end) {
            return 0;
        }

        if (!copy_token(out_msg->sensors[count].sensor_id, sizeof(out_msg->sensors[count].sensor_id), p, (size_t)(gt - p))) {
            return 0;
        }
        if (!copy_token(out_msg->sensors[count].sensor_state, sizeof(out_msg->sensors[count].sensor_state), gt + 1, (size_t)(dot - (gt + 1)))) {
            return 0;
        }
        if (!copy_token(out_msg->sensors[count].sensor_unit, sizeof(out_msg->sensors[count].sensor_unit), dot + 1, (size_t)(colon - (dot + 1)))) {
            return 0;
        }
        ext = find_next_ext_marker(colon + 1, end);
        if (!ext) {
            ext = end;
        }
        if (!copy_token(out_msg->sensors[count].sensor_value, sizeof(out_msg->sensors[count].sensor_value), colon + 1, (size_t)(ext - (colon + 1)))) {
            return 0;
        }

        cursor = ext;
        while (cursor && cursor < end) {
            const char mark = *cursor;
            const char* val_start = cursor + 1;
            const char* val_end = find_next_ext_marker(val_start, end);
            if (!val_end) {
                val_end = end;
            }
            if (mark == '#') {
#if OSFX_CFG_PAYLOAD_GEOHASH_ID
                if (!copy_token(out_msg->sensors[count].geohash_id, sizeof(out_msg->sensors[count].geohash_id), val_start, (size_t)(val_end - val_start))) {
                    return 0;
                }
#endif
            } else if (mark == '!') {
#if OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE
                if (!copy_token(out_msg->sensors[count].supplementary_message, sizeof(out_msg->sensors[count].supplementary_message), val_start, (size_t)(val_end - val_start))) {
                    return 0;
                }
#endif
            } else if (mark == '@') {
#if OSFX_CFG_PAYLOAD_RESOURCE_URL
                if (!copy_token(out_msg->sensors[count].resource_url, sizeof(out_msg->sensors[count].resource_url), val_start, (size_t)(val_end - val_start))) {
                    return 0;
                }
#endif
            }
            cursor = val_end;
        }

        ++count;
        p = end + 1;
    }

    out_msg->sensor_count = count;
    return 1;
}

