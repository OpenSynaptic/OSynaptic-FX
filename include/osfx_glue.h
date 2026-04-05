#ifndef OSFX_GLUE_H
#define OSFX_GLUE_H

#include <stddef.h>
#include <stdint.h>

#include "osfx_core.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    OSFX_GLUE_OK = 0,
    OSFX_GLUE_ERR_ARG = -1,
    OSFX_GLUE_ERR_RUNTIME = -2,
    OSFX_GLUE_ERR_CODEC = -3
};

typedef struct osfx_glue_config {
    uint32_t local_aid;
    uint32_t id_start;
    uint32_t id_end;
    uint64_t id_default_lease_seconds;
    uint64_t secure_expire_seconds;
    osfx_matrix_emit_fn matrix_emit_fn;
    void* matrix_emit_ctx;
    osfx_pf_emit_fn pf_emit_fn;
    void* pf_emit_ctx;
} osfx_glue_config;

/* osfx_glue_ctx embeds several large host-only runtime structures.
 * It is only available on non-AVR targets where SRAM is not constrained. */
#ifndef __AVR__

typedef struct osfx_glue_ctx {
    uint32_t local_aid;
    osfx_fusion_state tx_state;
    osfx_fusion_state rx_state;
    osfx_secure_session_store secure_store;
    osfx_id_allocator id_allocator;
    osfx_protocol_matrix protocol_matrix;
    osfx_platform_runtime platform_runtime;
} osfx_glue_ctx;

void osfx_glue_default_config(osfx_glue_config* out_cfg);
int osfx_glue_init(osfx_glue_ctx* ctx, const osfx_glue_config* cfg);

int osfx_glue_process_packet(
    osfx_glue_ctx* ctx,
    const uint8_t* packet,
    size_t packet_len,
    uint64_t now_ts,
    osfx_hs_result* out_result
);

int osfx_glue_encode_sensor_auto(
    osfx_glue_ctx* ctx,
    uint8_t tid,
    uint64_t timestamp_raw,
    const char* sensor_id,
    double input_value,
    const char* input_unit,
    uint8_t* out_packet,
    size_t out_packet_cap,
    int* out_packet_len,
    uint8_t* out_cmd
);

int osfx_glue_decode_sensor_auto(
    osfx_glue_ctx* ctx,
    const uint8_t* packet,
    size_t packet_len,
    char* out_sensor_id,
    size_t out_sensor_id_cap,
    double* out_value,
    char* out_unit,
    size_t out_unit_cap,
    osfx_packet_meta* out_meta
);

int osfx_glue_plugin_cmd(
    osfx_glue_ctx* ctx,
    const char* plugin_name,
    const char* cmd,
    const char* args,
    char* out,
    size_t out_cap
);

#endif /* __AVR__ */

#ifdef __cplusplus
}
#endif

#endif

