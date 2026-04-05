#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "osfx_core.h"

int main(void) {
    uint8_t packet[256];
    int packet_len = 0;
    osfx_packet_meta meta;
    char sensor_id[32];
    char unit[16];
    double value = 0.0;
    int ok;
    uint8_t cmd = 0;
    uint8_t ctrl[16];
    osfx_fusion_state tx_state;
    osfx_fusion_state rx_state;
    osfx_core_sensor_input sensors[2];
    osfx_core_sensor_output out_sensors[2];
    size_t out_sensor_count = 0;
    char node_id[32];
    char node_state[16];

    osfx_fusion_state_init(&tx_state);
    osfx_fusion_state_init(&rx_state);

    ok = osfx_core_encode_sensor_packet_auto(
        &tx_state, 42U, 1U, 1710000000ULL, "TEMP", 1.0, "kPa",
        packet, sizeof(packet), &packet_len, &cmd
    );
    if (!ok || packet_len <= 0 || cmd != OSFX_CMD_DATA_FULL) {
        printf("[FAIL] auto encode FULL failed cmd=%u\n", (unsigned)cmd);
        return 1;
    }
    ok = osfx_core_decode_sensor_packet_auto(
        &rx_state, packet, (size_t)packet_len,
        sensor_id, sizeof(sensor_id), &value, unit, sizeof(unit), &meta
    );
    if (!ok || meta.cmd != OSFX_CMD_DATA_FULL) {
        printf("[FAIL] auto decode FULL failed\n");
        return 1;
    }
    if (strcmp(sensor_id, "TEMP") != 0 || strcmp(unit, "Pa") != 0 || value != 1000.0) {
        printf("[FAIL] FULL payload mismatch sensor=%s unit=%s value=%f\n", sensor_id, unit, value);
        return 1;
    }

    ok = osfx_core_encode_sensor_packet_auto(
        &tx_state, 42U, 1U, 1710000001ULL, "TEMP", 1.2, "kPa",
        packet, sizeof(packet), &packet_len, &cmd
    );
    if (!ok || cmd != OSFX_CMD_DATA_DIFF) {
        printf("[FAIL] auto encode DIFF failed cmd=%u\n", (unsigned)cmd);
        return 1;
    }
    ok = osfx_core_decode_sensor_packet_auto(
        &rx_state, packet, (size_t)packet_len,
        sensor_id, sizeof(sensor_id), &value, unit, sizeof(unit), &meta
    );
    if (!ok || meta.cmd != OSFX_CMD_DATA_DIFF || value != 1200.0) {
        printf("[FAIL] auto decode DIFF failed value=%f\n", value);
        return 1;
    }

    ok = osfx_core_encode_sensor_packet_auto(
        &tx_state, 42U, 1U, 1710000002ULL, "TEMP", 1.2, "kPa",
        packet, sizeof(packet), &packet_len, &cmd
    );
    if (!ok || cmd != OSFX_CMD_DATA_HEART) {
        printf("[FAIL] auto encode HEART failed cmd=%u\n", (unsigned)cmd);
        return 1;
    }
    ok = osfx_core_decode_sensor_packet_auto(
        &rx_state, packet, (size_t)packet_len,
        sensor_id, sizeof(sensor_id), &value, unit, sizeof(unit), &meta
    );
    if (!ok || meta.cmd != OSFX_CMD_DATA_HEART || value != 1200.0) {
        printf("[FAIL] auto decode HEART failed value=%f\n", value);
        return 1;
    }

    memset(sensors, 0, sizeof(sensors));
    sensors[0].sensor_id = "TEMP";
    sensors[0].sensor_state = "OK";
    sensors[0].value = 1.0;
    sensors[0].unit = "kPa";
    sensors[1].sensor_id = "HUM";
    sensors[1].sensor_state = "OK";
    sensors[1].value = 55.0;
    sensors[1].unit = "%";

    ok = osfx_core_encode_multi_sensor_packet_auto(
        &tx_state, 42U, 2U, 1710000010ULL, "NODE_A", "ONLINE", sensors, 2U,
        packet, sizeof(packet), &packet_len, &cmd
    );
    if (!ok || packet_len <= 0) {
        printf("[FAIL] multi-sensor encode failed\n");
        return 1;
    }

    ok = osfx_core_decode_multi_sensor_packet_auto(
        &rx_state, packet, (size_t)packet_len,
        node_id, sizeof(node_id), node_state, sizeof(node_state),
        out_sensors, 2U, &out_sensor_count, &meta
    );
    if (!ok || out_sensor_count != 2U) {
        printf("[FAIL] multi-sensor decode failed count=%llu\n", (unsigned long long)out_sensor_count);
        return 1;
    }
    if (strcmp(node_id, "NODE_A") != 0 || strcmp(node_state, "ONLINE") != 0) {
        printf("[FAIL] multi node header mismatch\n");
        return 1;
    }
    if (strcmp(out_sensors[0].sensor_id, "TEMP") != 0 || out_sensors[0].value != 1000.0) {
        printf("[FAIL] multi sensor0 mismatch id=%s value=%f\n", out_sensors[0].sensor_id, out_sensors[0].value);
        return 1;
    }
    if (strcmp(out_sensors[1].sensor_id, "HUM") != 0 || out_sensors[1].value != 55.0) {
        printf("[FAIL] multi sensor1 mismatch id=%s value=%f\n", out_sensors[1].sensor_id, out_sensors[1].value);
        return 1;
    }

    sensors[1].value = 56.0;
    ok = osfx_core_encode_multi_sensor_packet_auto(
        &tx_state, 42U, 2U, 1710000011ULL, "NODE_A", "ONLINE", sensors, 2U,
        packet, sizeof(packet), &packet_len, &cmd
    );
    if (!ok || cmd != OSFX_CMD_DATA_DIFF) {
        printf("[FAIL] multi-sensor DIFF encode failed cmd=%u\n", (unsigned)cmd);
        return 1;
    }

    ok = osfx_core_decode_multi_sensor_packet_auto(
        &rx_state, packet, (size_t)packet_len,
        node_id, sizeof(node_id), node_state, sizeof(node_state),
        out_sensors, 2U, &out_sensor_count, &meta
    );
    if (!ok || meta.cmd != OSFX_CMD_DATA_DIFF || out_sensor_count != 2U) {
        printf("[FAIL] multi-sensor DIFF decode failed cmd=%u count=%llu\n", (unsigned)meta.cmd, (unsigned long long)out_sensor_count);
        return 1;
    }
    if (out_sensors[1].value != 56.0) {
        printf("[FAIL] multi-sensor DIFF value mismatch value=%f\n", out_sensors[1].value);
        return 1;
    }

    ok = osfx_core_encode_multi_sensor_packet_auto(
        &tx_state, 42U, 2U, 1710000012ULL, "NODE_A", "ONLINE", sensors, 2U,
        packet, sizeof(packet), &packet_len, &cmd
    );
    if (!ok || cmd != OSFX_CMD_DATA_HEART) {
        printf("[FAIL] multi-sensor HEART encode failed cmd=%u\n", (unsigned)cmd);
        return 1;
    }

    ok = osfx_core_decode_multi_sensor_packet_auto(
        &rx_state, packet, (size_t)packet_len,
        node_id, sizeof(node_id), node_state, sizeof(node_state),
        out_sensors, 2U, &out_sensor_count, &meta
    );
    if (!ok || meta.cmd != OSFX_CMD_DATA_HEART || out_sensor_count != 2U) {
        printf("[FAIL] multi-sensor HEART decode failed cmd=%u count=%llu\n", (unsigned)meta.cmd, (unsigned long long)out_sensor_count);
        return 1;
    }
    if (out_sensors[1].value != 56.0) {
        printf("[FAIL] multi-sensor HEART value mismatch value=%f\n", out_sensors[1].value);
        return 1;
    }

    if (osfx_build_secure_dict_ready(42U, 1710000010ULL, ctrl, sizeof(ctrl)) != 13) {
        printf("[FAIL] secure dict builder failed\n");
        return 1;
    }
    if (ctrl[0] != OSFX_CMD_SECURE_DICT_READY) {
        printf("[FAIL] secure dict cmd mismatch\n");
        return 1;
    }
    if (osfx_build_secure_channel_ack(42U, ctrl, sizeof(ctrl)) != 5 || ctrl[0] != OSFX_CMD_SECURE_CHANNEL_ACK) {
        printf("[FAIL] secure channel ack builder failed\n");
        return 1;
    }

    printf("[PASS] osfx-c99 integration test passed\n");
    return 0;
}

