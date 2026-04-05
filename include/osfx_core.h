#ifndef OSFX_CORE_H
#define OSFX_CORE_H

#include <stddef.h>
#include <stdint.h>

#include "osfx_solidity.h"
#include "osfx_fusion_packet.h"
#include "osfx_standardization.h"
#include "osfx_handshake_cmd.h"
#include "osfx_fusion_state.h"
#include "osfx_template_grammar.h"
#include "osfx_library_catalog.h"
#include "osfx_storage.h"
#include "osfx_transporter_runtime.h"
#include "osfx_secure_session.h"
#include "osfx_id_allocator.h"
#include "osfx_protocol_matrix.h"
#include "osfx_payload_crypto.h"
#include "osfx_handshake_dispatch.h"
#include "osfx_service_runtime.h"
#include "osfx_plugin_transport.h"
#include "osfx_plugin_test.h"
#include "osfx_plugin_port_forwarder.h"
#include "osfx_platform_runtime.h"
#include "osfx_cli_lite.h"
#include "osfx_glue.h"

#ifdef __cplusplus
extern "C" {
#endif

int osfx_b62_encode_i64(long long value, char* out, size_t out_len);
long long osfx_b62_decode_i64(const char* s, int* ok);

uint8_t osfx_crc8(const uint8_t* data, size_t len, uint16_t poly, uint8_t init);
uint16_t osfx_crc16_ccitt(const uint8_t* data, size_t len, uint16_t poly, uint16_t init);

/*
Packet wire format (FULL):
  [0] cmd
  [1] route_count (fixed 1)
  [2..5] source_aid (big-endian u32)
  [6] tid
  [7..12] timestamp_raw (big-endian u48)
  [13..13+body_len-1] body
  [end-3] crc8(body)
  [end-2..end-1] crc16(all previous bytes)
*/
int osfx_packet_encode_full(
    uint8_t cmd,
    uint32_t source_aid,
    uint8_t tid,
    uint64_t timestamp_raw,
    const uint8_t* body,
    size_t body_len,
    uint8_t* out,
    size_t out_cap
);

/* out fields:
   out_fields[0]=cmd
   out_fields[1]=route_count
   out_fields[2]=source_aid
   out_fields[3]=tid
   out_fields[4]=timestamp_raw
   out_fields[5]=body_offset
   out_fields[6]=body_len
   out_fields[7]=crc8_ok (0/1)
   out_fields[8]=crc16_ok (0/1)
*/
int osfx_packet_decode_min(const uint8_t* packet, size_t packet_len, uint64_t out_fields[9]);

/*
Facade helpers for end-to-end sensor packet path (v0):
  - standardize value to canonical unit
  - encode numeric value using fixed scale (x10000) Base62
  - build FULL packet body format: sensor_id|unit|b62_value
*/
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
);

int osfx_core_decode_sensor_body(
    const uint8_t* body,
    size_t body_len,
    char* out_sensor_id,
    size_t out_sensor_id_cap,
    double* out_value,
    char* out_unit,
    size_t out_unit_cap
);

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
);

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
);

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
);

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
);

typedef struct osfx_core_sensor_input {
    const char* sensor_id;
    const char* sensor_state;
    double value;
    const char* unit;
    const char* geohash_id;
    const char* supplementary_message;
    const char* resource_url;
} osfx_core_sensor_input;

typedef struct osfx_core_sensor_output {
    char sensor_id[OSFX_TMPL_ID_MAX];
    char sensor_state[OSFX_TMPL_STATE_MAX];
    double value;
    char unit[OSFX_TMPL_UNIT_MAX];
    char geohash_id[OSFX_TMPL_GEOHASH_MAX];
    char supplementary_message[OSFX_TMPL_MSG_MAX];
    char resource_url[OSFX_TMPL_URL_MAX];
} osfx_core_sensor_output;

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
);

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
);

#ifdef __cplusplus
}
#endif

#endif

