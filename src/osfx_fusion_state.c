#include "../include/osfx_fusion_state.h"

#include <stdio.h>
#include <string.h>

#include "../include/osfx_fusion_packet.h"

/* strnlen is POSIX, not standard C99.
 * Provide a portable fallback so the code compiles with strict -std=c99
 * toolchains (emscripten, xtensa, WASM). */
static size_t osfx_strnlen_(const char* s, size_t maxlen) {
    size_t n = 0;
    while (n < maxlen && s[n] != '\0') { n++; }
    return n;
}
#define strnlen osfx_strnlen_

/*
 * parse_body_slots
 * ----------------
 * Parse a multi-sensor body text and extract the structural identity plus
 * per-slot (META, VAL) pairs.
 *
 * Body format: "NodeID.State.TS|TAG1>META1:VAL1|TAG2>META2:VAL2|..."
 *
 * Outputs:
 *   out_sig_base / out_sig_base_len
 *       "NodeID.State." — the header up to and including the dot that
 *       precedes the timestamp.  Used as the structural identity: if this
 *       string changes, the server must be re-seeded with a new FULL packet.
 *
 *   tag_names[i] / tag_name_lens[i]
 *       Sensor tag names in body order, e.g. "TEMP", "HUM".
 *
 *   slots[2i]   / slot_lens[2i]     — META string (e.g. "OK.Cel")
 *   slots[2i+1] / slot_lens[2i+1]  — VAL  string (e.g. "2jBe")
 *
 *   The interleaved {META, VAL} order matches Rust's auto_decompose_input_inner
 *   so the resulting binary DIFF body is directly compatible with the Rust DLL.
 *
 * Returns sensor count (0 on error or empty body).
 */
static size_t parse_body_slots(
    const uint8_t* body,
    size_t         body_len,
    char*          out_sig_base,
    size_t         sig_base_cap,
    size_t*        out_sig_base_len,
    char           tag_names[][OSFX_FUSION_MAX_TAG_LEN],
    uint8_t*       tag_name_lens,
    char           slots[][OSFX_FUSION_MAX_VAL_LEN],
    uint8_t*       slot_lens,
    size_t         max_sensors
) {
    size_t bar1 = (size_t)-1;
    size_t last_dot = (size_t)-1;
    size_t seg_start;
    size_t sensor_count = 0;
    size_t i;

    if (!body || body_len == 0 || !out_sig_base || !out_sig_base_len ||
        !tag_names || !tag_name_lens || !slots || !slot_lens || max_sensors == 0) {
        return 0;
    }

    /* 1. Locate first '|' (end of header). */
    for (i = 0; i < body_len; ++i) {
        if (body[i] == '|') {
            bar1 = i;
            break;
        }
    }
    if (bar1 == (size_t)-1 || bar1 == 0) {
        return 0;
    }

    /* 2. Find last '.' in header to isolate sig_base ("NodeID.State."). */
    for (i = 0; i < bar1; ++i) {
        if (body[i] == '.') {
            last_dot = i;
        }
    }
    if (last_dot == (size_t)-1 || last_dot + 2 > sig_base_cap) {
        return 0;
    }
    memcpy(out_sig_base, body, last_dot + 1);
    out_sig_base[last_dot + 1] = '\0';
    *out_sig_base_len = last_dot + 1;

    /* 3. Parse sensor segments: "TAG>META:VAL" between '|' separators. */
    seg_start = bar1 + 1;
    while (seg_start < body_len && sensor_count < max_sensors) {
        size_t seg_end = seg_start;
        size_t gt_pos  = (size_t)-1;
        size_t colon_pos = (size_t)-1;
        size_t rest_start;
        size_t tag_len, meta_len, val_len;

        while (seg_end < body_len && body[seg_end] != '|') {
            ++seg_end;
        }
        if (seg_end == seg_start) {
            seg_start = seg_end + 1;
            continue;  /* empty segment */
        }

        /* Find '>' separating TAG from "META:VAL". */
        for (i = seg_start; i < seg_end; ++i) {
            if (body[i] == '>') {
                gt_pos = i;
                break;
            }
        }
        if (gt_pos == (size_t)-1) {
            seg_start = seg_end + 1;
            continue;
        }

        /* Find last ':' in the remainder to split META from VAL. */
        rest_start = gt_pos + 1;
        for (i = rest_start; i < seg_end; ++i) {
            if (body[i] == ':') {
                colon_pos = i;
            }
        }
        if (colon_pos == (size_t)-1 || colon_pos == rest_start || colon_pos + 1 >= seg_end) {
            seg_start = seg_end + 1;
            continue;
        }

        tag_len  = gt_pos - seg_start;
        meta_len = colon_pos - rest_start;
        val_len  = seg_end - (colon_pos + 1);

        if (tag_len  == 0 || tag_len  >= OSFX_FUSION_MAX_TAG_LEN ||
            meta_len == 0 || meta_len >= OSFX_FUSION_MAX_VAL_LEN ||
            val_len  == 0 || val_len  >= OSFX_FUSION_MAX_VAL_LEN) {
            seg_start = seg_end + 1;
            continue;
        }

        memcpy(tag_names[sensor_count], body + seg_start, tag_len);
        tag_names[sensor_count][tag_len] = '\0';
        tag_name_lens[sensor_count] = (uint8_t)tag_len;

        memcpy(slots[sensor_count * 2],     body + rest_start,    meta_len);
        slots[sensor_count * 2][meta_len]   = '\0';
        slot_lens[sensor_count * 2]         = (uint8_t)meta_len;

        memcpy(slots[sensor_count * 2 + 1], body + colon_pos + 1, val_len);
        slots[sensor_count * 2 + 1][val_len] = '\0';
        slot_lens[sensor_count * 2 + 1]      = (uint8_t)val_len;

        ++sensor_count;
        seg_start = seg_end + 1;
    }

    return sensor_count;
}

/*
 * reconstruct_body
 * ----------------
 * Build the full multi-sensor body text from a cached fusion entry and a
 * fresh timestamp.  Used by the C-side decoder to expand DIFF/HEART packets.
 *
 * Output format: "NodeID.State.TS|TAG1>META1:VAL1|TAG2>META2:VAL2|..."
 *
 * Returns bytes written (0 on error).
 */
static size_t reconstruct_body(
    const osfx_fusion_entry* e,
    uint64_t timestamp_raw,
    uint8_t* out,
    size_t out_cap
) {
    char ts_buf[32];
    int ts_len;
    size_t off = 0;
    size_t i;

    size_t sbl;
    if (!e || !out || out_cap == 0 || e->sig_base[0] == '\0' || e->sensor_count == 0) {
        return 0;
    }
    sbl = strnlen(e->sig_base, OSFX_FUSION_MAX_PREFIX);

    if (sbl + 1 > out_cap) {
        return 0;
    }
    memcpy(out, e->sig_base, sbl);
    off = sbl;

    ts_len = snprintf(ts_buf, sizeof(ts_buf), "%llu", (unsigned long long)timestamp_raw);
    if (ts_len <= 0 || off + (size_t)ts_len + 1 > out_cap) {
        return 0;
    }
    memcpy(out + off, ts_buf, (size_t)ts_len);
    off += (size_t)ts_len;
    out[off++] = '|';

    for (i = 0; i < e->sensor_count; ++i) {
        size_t tag_len  = e->tag_name_lens[i];
        size_t meta_len = e->last_val_lens[i * 2];
        size_t val_len  = e->last_val_lens[i * 2 + 1];
        /* tag + '>' + meta + ':' + val + '|' */
        if (off + tag_len + 1 + meta_len + 1 + val_len + 1 > out_cap) {
            return 0;
        }
        memcpy(out + off, e->tag_names[i], tag_len);
        off += tag_len;
        out[off++] = '>';
        memcpy(out + off, e->last_vals[i * 2], meta_len);
        off += meta_len;
        out[off++] = ':';
        memcpy(out + off, e->last_vals[i * 2 + 1], val_len);
        off += val_len;
        out[off++] = '|';
    }

    return off;
}

static osfx_fusion_entry* find_entry(osfx_fusion_state* st, uint32_t aid, uint8_t tid) {
    size_t i;
    if (!st) {
        return NULL;
    }
    for (i = 0; i < OSFX_FUSION_MAX_ENTRIES; ++i) {
        if (st->entries[i].used && st->entries[i].source_aid == aid && st->entries[i].tid == tid) {
            return &st->entries[i];
        }
    }
    return NULL;
}

static osfx_fusion_entry* alloc_entry(osfx_fusion_state* st, uint32_t aid, uint8_t tid) {
    size_t i;
    if (!st) {
        return NULL;
    }
    for (i = 0; i < OSFX_FUSION_MAX_ENTRIES; ++i) {
        if (!st->entries[i].used) {
            memset(&st->entries[i], 0, sizeof(st->entries[i]));
            st->entries[i].used       = 1;
            st->entries[i].source_aid = aid;
            st->entries[i].tid        = tid;
            return &st->entries[i];
        }
    }
    return NULL;
}

void osfx_fusion_state_init(osfx_fusion_state* st) {
    if (st) {
        memset(st, 0, sizeof(*st));
    }
}

void osfx_fusion_state_reset(osfx_fusion_state* st) {
    osfx_fusion_state_init(st);
}

int osfx_fusion_encode(
    osfx_fusion_state* st,
    uint32_t source_aid,
    uint8_t tid,
    uint64_t timestamp_raw,
    const uint8_t* full_body,
    size_t full_body_len,
    uint8_t* out_packet,
    size_t out_packet_cap,
    int* out_packet_len,
    uint8_t* out_cmd
) {
    char sig_base[OSFX_FUSION_MAX_PREFIX];
    size_t sig_base_len = 0;
    char new_tag_names[OSFX_FUSION_MAX_SENSORS][OSFX_FUSION_MAX_TAG_LEN];
    uint8_t new_tag_name_lens[OSFX_FUSION_MAX_SENSORS];
    char new_slots[OSFX_FUSION_MAX_VALS][OSFX_FUSION_MAX_VAL_LEN];
    uint8_t new_slot_lens[OSFX_FUSION_MAX_VALS];
    size_t sensor_count;
    size_t new_val_count;
    osfx_fusion_entry* e;
    int struct_changed;
    size_t i;
    uint8_t cmd;
    const uint8_t* body_to_send;
    size_t body_to_send_len;
    int pkt_len;
    /* 2 mask bytes + 16 slots × (1 len byte + 32 value bytes) */
    uint8_t diff_body[2 + OSFX_FUSION_MAX_VALS * (1 + OSFX_FUSION_MAX_VAL_LEN)];
    size_t mask_len;
    size_t diff_off;
    uint32_t mask;

    if (!st || !full_body || full_body_len == 0 || !out_packet || !out_packet_len) {
        return 0;
    }

    memset(new_tag_names,   0, sizeof(new_tag_names));
    memset(new_tag_name_lens, 0, sizeof(new_tag_name_lens));
    memset(new_slots,       0, sizeof(new_slots));
    memset(new_slot_lens,   0, sizeof(new_slot_lens));

    sensor_count = parse_body_slots(
        full_body, full_body_len,
        sig_base, sizeof(sig_base), &sig_base_len,
        new_tag_names, new_tag_name_lens,
        new_slots, new_slot_lens,
        OSFX_FUSION_MAX_SENSORS
    );

    if (sensor_count == 0) {
        /*
         * Body cannot be parsed into slots (e.g. oversized or non-template
         * format).  Degrade to an uncached FULL packet.
         */
        pkt_len = osfx_packet_encode_full(
            (uint8_t)OSFX_CMD_DATA_FULL,
            source_aid, tid, timestamp_raw,
            full_body, full_body_len,
            out_packet, out_packet_cap
        );
        if (pkt_len <= 0) {
            return 0;
        }
        *out_packet_len = pkt_len;
        if (out_cmd) {
            *out_cmd = (uint8_t)OSFX_CMD_DATA_FULL;
        }
        return 1;
    }

    new_val_count = sensor_count * 2;

    e = find_entry(st, source_aid, tid);
    if (!e) {
        e = alloc_entry(st, source_aid, tid);
    }
    if (!e) {
        return 0;
    }

    /* Determine whether the template structure has changed. */
    struct_changed = (e->sig_base[0] == '\0' ||
                      e->val_count != new_val_count ||
                      strnlen(e->sig_base, OSFX_FUSION_MAX_PREFIX) != sig_base_len ||
                      memcmp(e->sig_base, sig_base, sig_base_len) != 0);
    if (!struct_changed) {
        for (i = 0; i < sensor_count && !struct_changed; ++i) {
            if (e->tag_name_lens[i] != new_tag_name_lens[i] ||
                memcmp(e->tag_names[i], new_tag_names[i], new_tag_name_lens[i]) != 0) {
                struct_changed = 1;
            }
        }
    }

    if (struct_changed) {
        /* Template changed: send FULL and refresh the cached entry. */
        cmd              = (uint8_t)OSFX_CMD_DATA_FULL;
        body_to_send     = full_body;
        body_to_send_len = full_body_len;

        memcpy(e->sig_base, sig_base, sig_base_len + 1);
        e->sensor_count  = (uint8_t)sensor_count;
        e->val_count     = (uint8_t)new_val_count;
        for (i = 0; i < sensor_count; ++i) {
            memcpy(e->tag_names[i], new_tag_names[i], new_tag_name_lens[i] + 1);
            e->tag_name_lens[i] = new_tag_name_lens[i];
        }
        for (i = 0; i < new_val_count; ++i) {
            memcpy(e->last_vals[i], new_slots[i], new_slot_lens[i] + 1);
            e->last_val_lens[i] = new_slot_lens[i];
        }
    } else {
        int any_changed = 0;
        for (i = 0; i < new_val_count; ++i) {
            if (e->last_val_lens[i] != new_slot_lens[i] ||
                memcmp(e->last_vals[i], new_slots[i], new_slot_lens[i]) != 0) {
                any_changed = 1;
                break;
            }
        }

        if (!any_changed) {
            /* No slot changed: send HEART (empty body). */
            cmd              = (uint8_t)OSFX_CMD_DATA_HEART;
            body_to_send     = NULL;
            body_to_send_len = 0;
        } else {
            /*
             * Build binary DIFF body:
             *   [mask_bytes (big-endian, ceil(N/8) bytes)]
             *   followed by, for each changed slot i (bit i set in mask):
             *   [uint8 value_len] [value_bytes]
             *
             * The format matches the Rust DLL apply_fusion_receive_state
             * bitmask+length convention, so the server can decode it natively
             * without any Python-side fallback.
             */
            mask_len = (new_val_count + 7) / 8;
            diff_off = mask_len;   /* reserve space for mask at front */
            mask     = 0;

            for (i = 0; i < new_val_count; ++i) {
                if (e->last_val_lens[i] != new_slot_lens[i] ||
                    memcmp(e->last_vals[i], new_slots[i], new_slot_lens[i]) != 0) {
                    mask |= (1U << i);
                    diff_body[diff_off++] = new_slot_lens[i];
                    memcpy(diff_body + diff_off, new_slots[i], new_slot_lens[i]);
                    diff_off += new_slot_lens[i];
                    /* Update cached slot. */
                    memcpy(e->last_vals[i], new_slots[i], new_slot_lens[i] + 1);
                    e->last_val_lens[i] = new_slot_lens[i];
                }
            }

            /* Write mask big-endian into the leading mask_len bytes. */
            {
                uint32_t m = mask;
                size_t k;
                for (k = 0; k < mask_len; ++k) {
                    diff_body[mask_len - 1 - k] = (uint8_t)(m & 0xFFU);
                    m >>= 8;
                }
            }

            cmd              = (uint8_t)OSFX_CMD_DATA_DIFF;
            body_to_send     = diff_body;
            body_to_send_len = diff_off;
        }
    }

    pkt_len = osfx_packet_encode_full(
        cmd, source_aid, tid, timestamp_raw,
        body_to_send, body_to_send_len,
        out_packet, out_packet_cap
    );
    if (pkt_len <= 0) {
        return 0;
    }
    *out_packet_len = pkt_len;
    if (out_cmd) {
        *out_cmd = cmd;
    }
    return 1;
}

int osfx_fusion_decode_apply(
    osfx_fusion_state* st,
    const uint8_t* packet,
    size_t packet_len,
    uint8_t* out_body,
    size_t out_body_cap,
    size_t* out_body_len,
    osfx_packet_meta* out_meta
) {
    osfx_packet_meta meta;
    osfx_fusion_entry* e;
    const uint8_t* body;
    size_t body_len;
    char sig_base[OSFX_FUSION_MAX_PREFIX];
    size_t sig_base_len = 0;
    char new_tag_names[OSFX_FUSION_MAX_SENSORS][OSFX_FUSION_MAX_TAG_LEN];
    uint8_t new_tag_name_lens[OSFX_FUSION_MAX_SENSORS];
    char new_slots[OSFX_FUSION_MAX_VALS][OSFX_FUSION_MAX_VAL_LEN];
    uint8_t new_slot_lens[OSFX_FUSION_MAX_VALS];
    size_t sensor_count;

    if (!st || !packet || !out_body || !out_body_len) {
        return 0;
    }
    if (!osfx_packet_decode_meta(packet, packet_len, &meta)) {
        return 0;
    }
    body     = packet + meta.body_offset;
    body_len = meta.body_len;

    e = find_entry(st, meta.source_aid, meta.tid);
    if (!e && meta.cmd != (uint8_t)OSFX_CMD_DATA_FULL) {
        return 0;
    }

    if (meta.cmd == (uint8_t)OSFX_CMD_DATA_FULL) {
        size_t i;

        memset(new_tag_names,    0, sizeof(new_tag_names));
        memset(new_tag_name_lens,0, sizeof(new_tag_name_lens));
        memset(new_slots,        0, sizeof(new_slots));
        memset(new_slot_lens,    0, sizeof(new_slot_lens));

        sensor_count = parse_body_slots(
            body, body_len,
            sig_base, sizeof(sig_base), &sig_base_len,
            new_tag_names, new_tag_name_lens,
            new_slots, new_slot_lens,
            OSFX_FUSION_MAX_SENSORS
        );

        if (sensor_count > 0) {
            if (!e) {
                e = alloc_entry(st, meta.source_aid, meta.tid);
            }
            if (e) {
                memcpy(e->sig_base, sig_base, sig_base_len + 1);
                e->sensor_count = (uint8_t)sensor_count;
                e->val_count    = (uint8_t)(sensor_count * 2);
                for (i = 0; i < sensor_count; ++i) {
                    memcpy(e->tag_names[i], new_tag_names[i], new_tag_name_lens[i] + 1);
                    e->tag_name_lens[i] = new_tag_name_lens[i];
                }
                for (i = 0; i < sensor_count * 2; ++i) {
                    memcpy(e->last_vals[i], new_slots[i], new_slot_lens[i] + 1);
                    e->last_val_lens[i] = new_slot_lens[i];
                }
            }
        }

        if (body_len > out_body_cap) {
            return 0;
        }
        memcpy(out_body, body, body_len);
        *out_body_len = body_len;

    } else if (meta.cmd == (uint8_t)OSFX_CMD_DATA_DIFF) {
        size_t mask_len;
        uint32_t mask;
        size_t off;
        size_t j;

        if (!e || e->val_count == 0 || body_len == 0) {
            return 0;
        }
        mask_len = (e->val_count + 7) / 8;
        if (body_len < mask_len) {
            return 0;
        }

        /* Read mask (big-endian). */
        mask = 0;
        for (j = 0; j < mask_len; ++j) {
            mask = (mask << 8) | body[j];
        }
        off = mask_len;

        /* Apply changed slots. */
        for (j = 0; j < e->val_count; ++j) {
            if ((mask >> j) & 1U) {
                uint8_t v_len;
                if (off >= body_len) {
                    return 0;
                }
                v_len = body[off++];
                if (off + v_len > body_len || v_len >= OSFX_FUSION_MAX_VAL_LEN) {
                    return 0;
                }
                memcpy(e->last_vals[j], body + off, v_len);
                e->last_vals[j][v_len] = '\0';
                e->last_val_lens[j]    = v_len;
                off += v_len;
            }
        }

        *out_body_len = reconstruct_body(e, meta.timestamp_raw, out_body, out_body_cap);
        if (*out_body_len == 0) {
            return 0;
        }

    } else if (meta.cmd == (uint8_t)OSFX_CMD_DATA_HEART) {
        if (!e || e->val_count == 0) {
            return 0;
        }
        *out_body_len = reconstruct_body(e, meta.timestamp_raw, out_body, out_body_cap);
        if (*out_body_len == 0) {
            return 0;
        }
    } else {
        return 0;
    }

    if (out_meta) {
        *out_meta = meta;
    }
    return 1;
}
