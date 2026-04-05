# 03 API Index

## Facade

- Header file: `include/osfx_core.h`
- Main capabilities:
  - `osfx_core_encode_sensor_packet*`
  - `osfx_core_decode_sensor_packet*`
  - `osfx_core_encode_multi_sensor_packet_auto`
  - `osfx_core_decode_multi_sensor_packet_auto`

## Easy API

- Header file: `include/osfx_easy.h`
- Main capabilities:
  - `osfx_easy_init`
  - `osfx_easy_init_id_allocator`
  - `osfx_easy_allocate_aid`
  - `osfx_easy_encode_sensor_auto`
  - `osfx_easy_encode_multi_sensor_auto`

## Packet & Fusion

- `include/osfx_fusion_packet.h`
- `include/osfx_fusion_state.h`
- FULL/DIFF/HEART encoding, decoding, and state transitions.

## Security

- `include/osfx_secure_session.h`
- `include/osfx_payload_crypto.h`
- `include/osfx_handshake_dispatch.h`
- Key semantics:
  - Session state: `INIT/PLAINTEXT_SENT/DICT_READY/SECURE`
  - Timestamp checking: `ACCEPT/REPLAY/OUT_OF_ORDER`

## Runtime

- `include/osfx_transporter_runtime.h`
- `include/osfx_protocol_matrix.h`
- `include/osfx_service_runtime.h`
- Key control plane APIs:
  - `osfx_service_register_ex`
  - `osfx_service_count`
  - `osfx_service_name_at`
  - `osfx_service_load`
  - `osfx_service_command`

## Scoped Plugin APIs

- `include/osfx_plugin_transport.h`
- `include/osfx_plugin_test.h`
- `include/osfx_plugin_port_forwarder.h`
- `include/osfx_platform_runtime.h`
- `include/osfx_cli_lite.h`

## Usage Recommendations

- External integration should prioritize `osfx_core.h`.
- Use `osfx_platform_runtime + osfx_cli_lite` for platform command routing scenarios.
- Manage `port_forwarder` rules through plugin commands, not direct internal structure modifications.

