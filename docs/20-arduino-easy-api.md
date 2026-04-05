# 20 Arduino Easy API

This document introduces a simplified Arduino-facing API layer built on top of the existing C99 core APIs.

## Goals

- Keep the original low-level APIs intact.
- Provide a small set of high-frequency calls for sketch authors.
- Reduce boilerplate in setup/loop paths.

## Header and Entry

- Arduino include entry: `src/OSynapticFX.h`
- Easy API header: `include/osfx_easy.h`
- Easy API implementation: `src/osfx_easy.c`
- Reference sketch: `examples/EasyQuickStart/EasyQuickStart.ino`

## Example Matrix (Current)

- Easy-first entry:
  - `examples/EasyQuickStart/EasyQuickStart.ino`
- Core API examples:
  - `examples/BasicEncode/BasicEncode.ino`
  - `examples/PacketMetaDecode/PacketMetaDecode.ino`
  - `examples/FusionAutoMode/FusionAutoMode.ino`
  - `examples/MultiSensorNodePacket/MultiSensorNodePacket.ino`
  - `examples/SecureSessionRoundtrip/SecureSessionRoundtrip.ino`
- Advanced runtime / diagnostics examples:
  - `examples/FusionModeTest/FusionModeTest.ino`
  - `examples/BootCliOrRun/BootCliOrRun.ino`
  - `examples/QuickBench/QuickBench.ino`

## Easy API Surface

Context type:

- `osfx_easy_context`

Init and configuration:

- `osfx_easy_init`
- `osfx_easy_set_node`
- `osfx_easy_set_tid`
- `osfx_easy_set_aid`

AID allocator helpers:

- `osfx_easy_init_id_allocator`
- `osfx_easy_allocate_aid`
- `osfx_easy_touch_aid`
- `osfx_easy_save_ids`
- `osfx_easy_load_ids`
- `osfx_easy_is_aid_ready`

Encode helpers:

- `osfx_easy_encode_sensor_auto`
- `osfx_easy_encode_multi_sensor_auto`
- `osfx_easy_cmd_name`

## Minimal Single-Sensor Flow

1. Call `osfx_easy_init` once.
2. Call `osfx_easy_init_id_allocator` and `osfx_easy_allocate_aid` in `setup`.
3. Call `osfx_easy_encode_sensor_auto` in `loop`.
4. Print command via `osfx_easy_cmd_name` for debugging.

Reference: `examples/EasyQuickStart/EasyQuickStart.ino`

## Minimal Multi-Sensor Flow

1. Init context as above.
2. Set node info through `osfx_easy_set_node`.
3. Build `osfx_core_sensor_input[]`.
4. Call `osfx_easy_encode_multi_sensor_auto`.

## Notes

- Easy API still uses the same wire format and fusion behavior as core APIs.
- `osfx_easy_load_ids` restores lease table state, but does not auto-select a local current AID.
  After load, call `osfx_easy_allocate_aid` or `osfx_easy_set_aid` explicitly.
- For advanced behavior (secure session, protocol matrix dispatch, CLI bridges), continue using core/runtime APIs directly.
