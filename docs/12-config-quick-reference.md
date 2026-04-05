# 12 Config Quick Reference

## Arduino Consumer Quick Reference

### Install and Include

- Install from Arduino IDE Libraries Manager: search `OSynaptic-FX`.
- Arduino include entry: `#include <OSynapticFX.h>`.

### Arduino CLI Compile Examples

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\examples\BasicEncode
arduino-cli compile --fqbn arduino:avr:uno .\examples\PacketMetaDecode
arduino-cli compile --fqbn arduino:avr:uno .\examples\FusionAutoMode
arduino-cli compile --fqbn arduino:avr:uno .\examples\SecureSessionRoundtrip
arduino-cli compile --fqbn arduino:avr:uno .\examples\MultiSensorNodePacket
```

### Precompiled Binary Layout

- `library.properties` uses `precompiled=full`.
- Arduino-recognized binary path pattern: `src/{build.mcu}/libOSynapticFX.a`.
- Current mapped folders in this repo:
  - `src/atmega328p/`
  - `src/avr/`
  - `src/esp32/`
  - `src/rp2040/`
  - `src/cortex-m0plus/`
  - `src/stm32/`
  - `src/riscv32/`
- Non-Arduino precompiled archives are kept in `extras/precompiled-non-arduino/` and are ignored by Arduino build tools.

## Maintainer Script Parameters

### `scripts/build.ps1`

- `-Compiler auto|clang|gcc|cl`
- Default: `auto`

### `scripts/test.ps1`

- `-Compiler auto|clang|gcc|cl`
- `-Matrix` (run `clang/gcc/cl` gate)
- Default: `-Compiler auto`

### `scripts/bench.ps1`

- `-Compiler auto|clang|gcc|cl`
- `-MemoryLimitKB <int>`
- Default: `-MemoryLimitKB 16`
- Set `-MemoryLimitKB 0` to disable memory lock.

## Common Config Profiles

```powershell
# Local fast validation
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Compiler auto

# Release matrix gate
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Matrix

# Release benchmark with default 16KB lock
powershell -ExecutionPolicy Bypass -File .\\scripts\bench.ps1 -Compiler auto

# Benchmark with strict lock
powershell -ExecutionPolicy Bypass -File .\\scripts\bench.ps1 -Compiler auto -MemoryLimitKB 8
```

## CLI Runtime Config Entry

- `plugin-load <name> [config]` can pass plugin-specific config strings.
- Current scoped plugins:
  - `transport`
  - `test_plugin`
  - `port_forwarder`

## Report Artifacts

- Quality gate: `build/quality_gate_report.md`
- Benchmark: `build/bench/bench_report.md`
- Benchmark CSV: `build/bench/bench_report.csv`

