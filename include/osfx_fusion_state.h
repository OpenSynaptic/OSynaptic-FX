#ifndef OSFX_FUSION_STATE_H
#define OSFX_FUSION_STATE_H

#include <stddef.h>
#include <stdint.h>

#include "osfx_fusion_packet.h"
#include "osfx_handshake_cmd.h"
#include "osfx_build_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Arduino/ESP32: osfx_fusion_state lives in DRAM (.bss).
 * Each entry is ~260 bytes.
 *   4  entries (~1.0 KB) — single-node device
 *   32 entries (~8.3 KB) — gateway for up to ~30 nodes (default)
 *   64 entries (~16.6 KB) — large gateway; verify free heap before using
 * All constants are overridable via osfx_user_config.h. */
#ifndef OSFX_FUSION_MAX_ENTRIES
#define OSFX_FUSION_MAX_ENTRIES 32
#endif
#ifndef OSFX_FUSION_MAX_PREFIX
#define OSFX_FUSION_MAX_PREFIX  64
#endif
/* Maximum number of value slots per entry (2 slots per sensor: META + VAL).
 * Supports up to OSFX_FUSION_MAX_VALS/2 sensors per packet. */
#ifndef OSFX_FUSION_MAX_VALS
#define OSFX_FUSION_MAX_VALS    8
#endif
#ifndef OSFX_FUSION_MAX_VAL_LEN
#define OSFX_FUSION_MAX_VAL_LEN 16
#endif
/* Maximum number of sensors and tag-name length per entry. */
#ifndef OSFX_FUSION_MAX_SENSORS
#define OSFX_FUSION_MAX_SENSORS 4
#endif
#ifndef OSFX_FUSION_MAX_TAG_LEN
#define OSFX_FUSION_MAX_TAG_LEN 12
#endif

typedef struct osfx_fusion_entry {
    uint32_t source_aid;
    /* Pack small fields together after source_aid to avoid padding waste.
     * uint8_t × 4 = 4 bytes (fits the natural alignment gap after uint32_t). */
    uint8_t tid;
    uint8_t sensor_count;  /* max OSFX_FUSION_MAX_SENSORS  (<= 255) */
    uint8_t val_count;     /* == 2 * sensor_count           (<= 255) */
    uint8_t used;          /* boolean: 0 or 1 */
    /* Structural identity: "NodeID.State." (header without timestamp).
     * null-terminated; length is derived with strnlen when needed. */
    char sig_base[OSFX_FUSION_MAX_PREFIX];
    /* Sensor tag names (e.g. "TEMP", "HUM") — part of structural identity. */
    char tag_names[OSFX_FUSION_MAX_SENSORS][OSFX_FUSION_MAX_TAG_LEN];
    uint8_t tag_name_lens[OSFX_FUSION_MAX_SENSORS];
    /* Per-slot value cache: layout {META_0, VAL_0, META_1, VAL_1, ...}
     * Matches the order produced by Rust auto_decompose_input_inner so the
     * binary DIFF body is directly compatible with the Rust DLL decoder. */
    char last_vals[OSFX_FUSION_MAX_VALS][OSFX_FUSION_MAX_VAL_LEN];
    uint8_t last_val_lens[OSFX_FUSION_MAX_VALS];
} osfx_fusion_entry;

typedef struct osfx_fusion_state {
    osfx_fusion_entry entries[OSFX_FUSION_MAX_ENTRIES];
} osfx_fusion_state;

void osfx_fusion_state_init(osfx_fusion_state* st);
void osfx_fusion_state_reset(osfx_fusion_state* st);

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
);

int osfx_fusion_decode_apply(
    osfx_fusion_state* st,
    const uint8_t* packet,
    size_t packet_len,
    uint8_t* out_body,
    size_t out_body_cap,
    size_t* out_body_len,
    osfx_packet_meta* out_meta
);

#ifdef __cplusplus
}
#endif

#endif

