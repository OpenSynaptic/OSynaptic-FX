#ifndef OSFX_TEMPLATE_GRAMMAR_H
#define OSFX_TEMPLATE_GRAMMAR_H

#include <stddef.h>
#include <stdint.h>
#include "osfx_build_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum sensors per template message.
 * Override via osfx_user_config.h before this header is included. */
#ifndef OSFX_TMPL_MAX_SENSORS
#define OSFX_TMPL_MAX_SENSORS 16
#endif
#define OSFX_TMPL_ID_MAX 32
#define OSFX_TMPL_STATE_MAX 16
#define OSFX_TMPL_UNIT_MAX 24
#define OSFX_TMPL_VALUE_MAX 48
#define OSFX_TMPL_TS_MAX 32
#define OSFX_TMPL_GEOHASH_MAX 32
#define OSFX_TMPL_MSG_MAX 128
#define OSFX_TMPL_URL_MAX 128

typedef struct osfx_sensor_slot {
    char sensor_id[OSFX_TMPL_ID_MAX];
    char sensor_state[OSFX_TMPL_STATE_MAX];
    char sensor_unit[OSFX_TMPL_UNIT_MAX];
    char sensor_value[OSFX_TMPL_VALUE_MAX];
#if OSFX_CFG_PAYLOAD_GEOHASH_ID
    char geohash_id[OSFX_TMPL_GEOHASH_MAX];
#endif
#if OSFX_CFG_PAYLOAD_SUPPLEMENTARY_MESSAGE
    char supplementary_message[OSFX_TMPL_MSG_MAX];
#endif
#if OSFX_CFG_PAYLOAD_RESOURCE_URL
    char resource_url[OSFX_TMPL_URL_MAX];
#endif
} osfx_sensor_slot;

typedef struct osfx_template_msg {
    char node_id[OSFX_TMPL_ID_MAX];
    char node_state[OSFX_TMPL_STATE_MAX];
    char ts_token[OSFX_TMPL_TS_MAX];
    uint8_t sensor_count;
    osfx_sensor_slot sensors[OSFX_TMPL_MAX_SENSORS];
} osfx_template_msg;

int osfx_template_encode(
    const char* node_id,
    const char* node_state,
    const char* ts_token,
    const osfx_sensor_slot* sensors,
    size_t sensor_count,
    char* out,
    size_t out_cap
);

int osfx_template_decode(const char* text, osfx_template_msg* out_msg);

#ifdef __cplusplus
}
#endif

#endif

