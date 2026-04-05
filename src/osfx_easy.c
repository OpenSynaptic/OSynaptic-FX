#include "../include/osfx_easy.h"

#include <string.h>

static int copy_with_fallback(char* out, size_t cap, const char* value, const char* fallback) {
    const char* src = (value && value[0] != '\0') ? value : fallback;
    size_t n;
    if (!out || cap == 0U || !src) {
        return 0;
    }
    n = strlen(src);
    if (n + 1U > cap) {
        return 0;
    }
    memcpy(out, src, n + 1U);
    return 1;
}

void osfx_easy_init(osfx_easy_context* ctx) {
    if (!ctx) {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    osfx_fusion_state_init(&ctx->tx_state);
    ctx->tid = 2U;
    (void)copy_with_fallback(ctx->node_id, sizeof(ctx->node_id), NULL, "NODE_A");
    (void)copy_with_fallback(ctx->node_state, sizeof(ctx->node_state), NULL, "ONLINE");
}

int osfx_easy_set_node(osfx_easy_context* ctx, const char* node_id, const char* node_state) {
    if (!ctx) {
        return 0;
    }
    if (!copy_with_fallback(ctx->node_id, sizeof(ctx->node_id), node_id, "NODE_A")) {
        return 0;
    }
    if (!copy_with_fallback(ctx->node_state, sizeof(ctx->node_state), node_state, "ONLINE")) {
        return 0;
    }
    return 1;
}

void osfx_easy_set_tid(osfx_easy_context* ctx, uint8_t tid) {
    if (!ctx) {
        return;
    }
    ctx->tid = tid;
}

void osfx_easy_set_aid(osfx_easy_context* ctx, uint32_t aid) {
    if (!ctx) {
        return;
    }
    ctx->aid = aid;
    ctx->aid_ready = (aid > 0U) ? 1 : 0;
}

int osfx_easy_init_id_allocator(
    osfx_easy_context* ctx,
    uint32_t start_id,
    uint32_t end_id,
    uint64_t default_lease_seconds
) {
    if (!ctx || start_id > end_id) {
        return 0;
    }
    osfx_id_allocator_init(&ctx->id_alloc, start_id, end_id, default_lease_seconds);
    ctx->id_alloc_ready = 1;
    return 1;
}

int osfx_easy_allocate_aid(osfx_easy_context* ctx, uint64_t now_ts, uint32_t* out_aid) {
    uint32_t aid = 0U;
    if (!ctx || !ctx->id_alloc_ready) {
        return 0;
    }
    if (!osfx_id_allocate(&ctx->id_alloc, now_ts, &aid)) {
        return 0;
    }
    ctx->aid = aid;
    ctx->aid_ready = 1;
    if (out_aid) {
        *out_aid = aid;
    }
    return 1;
}

int osfx_easy_touch_aid(osfx_easy_context* ctx, uint64_t now_ts) {
    if (!ctx || !ctx->id_alloc_ready || !ctx->aid_ready) {
        return 0;
    }
    return osfx_id_touch(&ctx->id_alloc, ctx->aid, now_ts);
}

int osfx_easy_save_ids(const osfx_easy_context* ctx, const char* path) {
    if (!ctx || !ctx->id_alloc_ready || !path) {
        return 0;
    }
    return osfx_id_save(&ctx->id_alloc, path);
}

int osfx_easy_load_ids(osfx_easy_context* ctx, const char* path) {
    if (!ctx || !path) {
        return 0;
    }
    if (!osfx_id_load(&ctx->id_alloc, path)) {
        return 0;
    }
    ctx->id_alloc_ready = 1;
    /*
     * Loaded file stores lease table, not a "current local aid" marker.
     * Force explicit allocate/set after load to avoid stale local state.
     */
    ctx->aid_ready = 0;
    ctx->aid = 0U;
    return 1;
}

int osfx_easy_is_aid_ready(const osfx_easy_context* ctx) {
    return (ctx && ctx->aid_ready) ? 1 : 0;
}

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
) {
    uint8_t cmd_tmp = 0U;
    uint8_t* cmd_ptr = out_cmd ? out_cmd : &cmd_tmp;
    if (!ctx || !ctx->aid_ready) {
        return 0;
    }
    return osfx_core_encode_sensor_packet_auto(
        &ctx->tx_state,
        ctx->aid,
        ctx->tid,
        timestamp_raw,
        sensor_id,
        input_value,
        input_unit,
        out_packet,
        out_packet_cap,
        out_packet_len,
        cmd_ptr
    );
}

int osfx_easy_encode_multi_sensor_auto(
    osfx_easy_context* ctx,
    uint64_t timestamp_raw,
    const osfx_core_sensor_input* sensors,
    size_t sensor_count,
    uint8_t* out_packet,
    size_t out_packet_cap,
    int* out_packet_len,
    uint8_t* out_cmd
) {
    uint8_t cmd_tmp = 0U;
    uint8_t* cmd_ptr = out_cmd ? out_cmd : &cmd_tmp;
    if (!ctx || !ctx->aid_ready || !sensors || sensor_count == 0U) {
        return 0;
    }
    return osfx_core_encode_multi_sensor_packet_auto(
        &ctx->tx_state,
        ctx->aid,
        ctx->tid,
        timestamp_raw,
        ctx->node_id,
        ctx->node_state,
        sensors,
        sensor_count,
        out_packet,
        out_packet_cap,
        out_packet_len,
        cmd_ptr
    );
}

const char* osfx_easy_cmd_name(uint8_t cmd) {
    if (cmd == OSFX_CMD_DATA_FULL) {
        return "FULL";
    }
    if (cmd == OSFX_CMD_DATA_DIFF) {
        return "DIFF";
    }
    if (cmd == OSFX_CMD_DATA_HEART) {
        return "HEART";
    }
    return "UNKNOWN";
}
