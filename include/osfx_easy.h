#ifndef OSFX_EASY_H
#define OSFX_EASY_H

#include "osfx_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * High-level helper context for Arduino-friendly usage.
 * Keeps frequently used runtime state in one place.
 */
typedef struct osfx_easy_context {
    osfx_fusion_state tx_state;
    osfx_id_allocator id_alloc;
    uint32_t aid;
    uint8_t tid;
    int aid_ready;
    int id_alloc_ready;
    char node_id[OSFX_TMPL_ID_MAX];
    char node_state[OSFX_TMPL_STATE_MAX];
} osfx_easy_context;

/* Initialize context with safe defaults. */
void osfx_easy_init(osfx_easy_context* ctx);

/* Optional overrides for node identity used by multi-sensor packets. */
int osfx_easy_set_node(osfx_easy_context* ctx, const char* node_id, const char* node_state);

/* Set protocol TID used by encode helpers. */
void osfx_easy_set_tid(osfx_easy_context* ctx, uint8_t tid);

/* Set fixed source AID (skips allocator flow). */
void osfx_easy_set_aid(osfx_easy_context* ctx, uint32_t aid);

/* Allocate and manage AID using built-in id allocator. */
int osfx_easy_init_id_allocator(
    osfx_easy_context* ctx,
    uint32_t start_id,
    uint32_t end_id,
    uint64_t default_lease_seconds
);
int osfx_easy_allocate_aid(osfx_easy_context* ctx, uint64_t now_ts, uint32_t* out_aid);
int osfx_easy_touch_aid(osfx_easy_context* ctx, uint64_t now_ts);
int osfx_easy_save_ids(const osfx_easy_context* ctx, const char* path);
int osfx_easy_load_ids(osfx_easy_context* ctx, const char* path);

/* Readiness check before encode calls. */
int osfx_easy_is_aid_ready(const osfx_easy_context* ctx);

/* One-call encode helpers for most Arduino use cases. */
int osfx_easy_encode_sensor_auto(
    osfx_easy_context* ctx,
    uint64_t timestamp_raw,
    const char* sensor_id,
    double input_value,
    const char* input_unit,
    uint8_t* out_packet,
    size_t out_packet_cap,
    int* out_packet_len,
    uint8_t* out_cmd
);

int osfx_easy_encode_multi_sensor_auto(
    osfx_easy_context* ctx,
    uint64_t timestamp_raw,
    const osfx_core_sensor_input* sensors,
    size_t sensor_count,
    uint8_t* out_packet,
    size_t out_packet_cap,
    int* out_packet_len,
    uint8_t* out_cmd
);

/* Human-readable packet command name helper. */
const char* osfx_easy_cmd_name(uint8_t cmd);

#ifdef __cplusplus
}
#endif

#endif
