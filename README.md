# OSynaptic-FX

简体中文说明请见 [README.zh.md](README.zh.md)。

**Embedded-first OpenSynaptic C99 runtime for Arduino** — encodes multi-sensor readings into a compact binary packet, sends FULL or delta-DIFF frames, and integrates directly with the [OpenSynaptic](../OpenSynaptic/README.md) server over any transport (UDP / TCP / UART / LoRa / MQTT / CAN).

![C99](https://img.shields.io/badge/C-99-00599C?logo=c&logoColor=white)
![Version](https://img.shields.io/badge/version-1.0.0-2E8B57)
![Arduino](https://img.shields.io/badge/Arduino-Library-00979D?logo=arduino&logoColor=white)
![Status](https://img.shields.io/badge/status-stable-2E8B57)
![License](https://img.shields.io/badge/License-MIT-yellow)
[![Arduino Library](https://www.ardu-badge.com/badge/OSynaptic-FX.svg?)](https://www.ardu-badge.com/OSynaptic-FX)

![ESP32](https://img.shields.io/badge/ESP32-supported-E7352C?logo=espressif&logoColor=white)
![STM32](https://img.shields.io/badge/STM32-supported-03234B?logo=stmicroelectronics&logoColor=white)
![RISC-V](https://img.shields.io/badge/RISC--V-supported-283272?logo=riscv&logoColor=white)
![RP2040](https://img.shields.io/badge/RP2040-supported-B41F47?logo=raspberrypi&logoColor=white)
![AVR](https://img.shields.io/badge/AVR-supported-00599C)
![Cortex-M](https://img.shields.io/badge/Cortex--M-supported-0091BD)

## Try In 30 Seconds

```
Arduino IDE → Sketch > Include Library > Manage Libraries → search "OSynaptic-FX" → Install
File > Examples > OSynaptic-FX > EasyQuickStart → Upload
```

Arduino CLI:

```bash
arduino-cli lib install "OSynaptic-FX"
arduino-cli compile --fqbn esp32:esp32:esp32 examples/EasyQuickStart
arduino-cli upload  --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 examples/EasyQuickStart
```

Open Serial Monitor at **115200 baud** and watch binary DIFF packets stream out.

---

## Table of Contents

- [Why OSynaptic-FX](#why-OSynaptic-fx)
- [Architecture](#architecture)
- [Memory Usage](#memory-usage)
- [Quick Start (Code)](#quick-start-code)
- [Tuning for Your Target](#tuning-for-your-target)
- [Binary DIFF Protocol](#binary-diff-protocol)
- [Examples](#examples)
- [Repository Map](#repository-map)
- [CMake Build](#cmake-build)
- [Precompiled Archives](#precompiled-archives)
- [Capability Snapshot](#capability-snapshot)
- [Documentation Index](#documentation-index)
- [Contributing](#contributing)
- [FAQ](#faq)

---

## Why OSynaptic-FX

- **DRAM-safe on ESP32**: default configuration fits in ~27 KB of DRAM; all limits are `#ifndef`-overridable at compile time.
- **Binary DIFF packets**: only changed sensor slots are transmitted — saves bandwidth on noisy multi-sensor nodes.
- **Spec-driven**: implementation is derived from the same OpenSynaptic protocol spec as the Python/Rust server, so packets decode without custom glue.
- **Zero dynamic allocation option**: set `OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH 1` and everything lives in static buffers.
- **Portable C99**: compiles clean on AVR, ESP32, STM32, RP2040, RISC-V, and bare-metal Cortex-M with `-Wall -Wextra`.

---

## Architecture

```mermaid
%%{init: {'flowchart': {'rankSeparation': 60, 'nodeSeparation': 50}}}%%
graph LR
    A["Sensor Readings\n(osfx_core_sensor_input[])"] -->|standardize| B["Template Grammar\n(osfx_template_grammar)"]
    B -->|body bytes| C["Fusion Engine\n(FULL / DIFF / HEART)"]
    C -->|binary packet| D["Transport Layer\n(UDP / UART / LoRa / MQTT)"]
    D -->|transmitted| E["OpenSynaptic Server\n(Python / Rust DLL)"]

    style A fill:#e1f5ff
    style B fill:#fff3e0
    style C fill:#e8f5e9
    style D fill:#fce4ec
    style E fill:#e0f2f1
```

**Packet pipeline:**

```
osfx_core_sensor_input[]
  → osfx_template_grammar_encode()   # tag:value body bytes, UCUM normalised
  → osfx_fusion_encode()             # FULL (first) or binary bitmask DIFF (delta)
  → transport callback               # send over UDP / Serial / LoRa / …
```

The server side decodes via `OSVisualFusionEngine` + `codec.py` (OpenSynaptic project).

---

## Memory Usage

All figures are for ESP32 at default `osfx_user_config.h` settings (v1.0.0).

| Component | DRAM (BSS/DATA) | Notes |
|---|---|---|
| `osfx_easy_context` | ~14 KB | includes fusion state + template |
| ID allocator (`OSFX_ID_MAX_ENTRIES=128`) | ~5 KB | `id_entry` × 128 |
| Body scratch buffer (`BODY_CAP=512`) | 512 B | static when `STATIC_SCRATCH=1` |
| Total typical sketch | **~20 KB** | well within 160 KB DRAM limit |

> For tighter targets (AVR / Cortex-M0) reduce `OSFX_FUSION_MAX_ENTRIES` to 8–16 and `OSFX_ID_MAX_ENTRIES` to 32 via `osfx_user_config.h`.

---

## Quick Start (Code)

### Minimal single-sensor node

```c
#include <OSynapticFX.h>

static osfx_easy_context g_ctx;
static uint8_t           g_buf[256];

void setup() {
    Serial.begin(115200);
    osfx_easy_init(&g_ctx);
    osfx_easy_set_node(&g_ctx, "NODE_A", "ONLINE");
    osfx_easy_set_tid(&g_ctx, 1U);
    osfx_easy_init_id_allocator(&g_ctx, 100U, 10000U, 86400U);
}

void loop() {
    osfx_core_sensor_input s = {0};
    strncpy(s.tag,   "TEMP",  sizeof(s.tag));
    strncpy(s.value, "23.5",  sizeof(s.value));
    strncpy(s.unit,  "Cel",   sizeof(s.unit));

    int len = 0;
    uint8_t cmd = 0;
    uint64_t ts = 1710000000ULL + millis() / 1000ULL;

    int r = osfx_easy_encode_sensor_auto(&g_ctx, &s, 1, ts, g_buf, sizeof(g_buf), &len, &cmd);
    if (r == 0) {
        Serial.write(g_buf, len);   /* or WiFi.UDP.write / Serial2.write / … */
    }
    delay(1000);
}
```

### Multi-sensor node (ESP32)

See [examples/ESP32WiFiMultiSensorAuto](examples/ESP32WiFiMultiSensorAuto/ESP32WiFiMultiSensorAuto.ino) for a complete Wi-Fi UDP example with 4 sensors, ID persistence on LittleFS, and automatic FULL/DIFF switching.

---

## Tuning for Your Target

All limits are overridable before including any OSynaptic-FX header. Edit **`include/osfx_user_config.h`** (or pass `-D` flags to your build system):
All limits are overridable before including any OSynaptic-FX header. Edit **`include/osfx_user_config.h`** (or pass `-D` flags to your build system):

```c
/* include/osfx_user_config.h  — uncomment to override defaults */

/* Fusion state table size (entries = remote nodes tracked) */
#define OSFX_FUSION_MAX_ENTRIES   16   /* default 32  — use 8-16 for AVR */

/* ID allocator table size */
#define OSFX_ID_MAX_ENTRIES       64   /* default 128 — use 32 for AVR  */

/* Max sensors per template */
#define OSFX_TMPL_MAX_SENSORS      4   /* default 4   — raise to 8-16 for gateways */

/* Body scratch buffer (bytes) */
#define OSFX_CFG_MULTI_SENSOR_BODY_CAP  256  /* default 512 */

/* Use static scratch instead of stack-allocated VLA */
#define OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH  1  /* default 1 on Arduino */

/* Enable/disable optional sensor slot fields to shrink struct size */
#define OSFX_CFG_PAYLOAD_GEOHASH       0   /* +36 B/sensor when enabled  */
#define OSFX_CFG_PAYLOAD_SUPP_MSG      1   /* +128 B/sensor when enabled */
#define OSFX_CFG_PAYLOAD_RESOURCE_URL  0   /* +128 B/sensor when enabled */
```

**Recommended preset by board** (pass `-DOSFX_ARCH_PRESET=<name>` to CMake):

| Architecture | Example boards | CMake preset | `MAX_ENTRIES` | `ID_MAX` | `BODY_CAP` | DRAM (fusion+ID+body) |
|---|---|---|---|---|---|---|
| Cortex-M0+ | RP2040, STM32C0 | `cortexm0plus` | 32 | 64 | 256 B | ~6 KB |
| Cortex-M3 | STM32F1 | `cortexm3` | 32 | 64 | 256 B | ~6 KB |
| Cortex-M4 | STM32F4, SAMD51 | `cortexm4` | 64 | 128 | 512 B | ~23 KB |
| Cortex-M33 | STM32C5, LPC55 | `cortexm33` | 64 | 128 | 512 B | ~23 KB |
| Cortex-M7 | STM32H7, i.MX RT | `cortexm7` | 128 | 256 | 1 KB | ~115 KB |
| RISC-V 32 | ESP32-C3, CH32V | `riscv32` | 64 | 128 | 512 B | ~23 KB |
| Xtensa (ESP32) | ESP32, ESP32-S3 | `esp32` | 128 | 256 | 1 KB | ~115 KB |
| AVR large | ATmega2560 | `avr2560` | 16 | 16 | 64 B | ~2.0 KB |
| AVR small ⚠ | ATmega328P | `avr328p` | 8 | 8 | 32 B | ~0.6 KB |
| x86_64 (host) | Linux / Windows | `x86_64` | 256 | 512 | 4 KB | ~232 KB |

> ⚠ **ATmega328P**: Only the encode path is safe within 2 KB SRAM. Do not call `osfx_fusion_decode_apply()` or `osfx_secure_session_*` on this target.

---

## Binary DIFF Protocol

OSynaptic-FX v1.0.0 sends protocol-native **binary bitmask DIFF** packets compatible with the OpenSynaptic Rust DLL decoder (`auto_decompose_input_inner`):

```
[ mask_bytes (big-endian, ceil(N/8) bytes) ]
  for each changed slot i (in order META_0, VAL_0, META_1, VAL_1 ...):
    [ uint8 length ] [ value bytes ]
```

- **FULL packet**: sent on first transmission and every `OSFX_FUSION_FULL_INTERVAL` packets; all slots set in mask.
- **DIFF packet**: only slots with changed values are included; unchanged slots are skipped.
- **HEART packet**: keepalive with empty body, resets the server-side DIFF timer.

The server (`OpenSynaptic/src/opensynaptic/core/rscore/codec.py`) accepts FULL, DIFF, and HEART with a backward-compatible legacy text fallback.

---

## Examples

| Example | Board | Difficulty | Description |
|---|---|---|---|
| [EasyQuickStart](examples/EasyQuickStart/EasyQuickStart.ino) | Any | ★☆☆ | Minimal `osfx_easy` API walkthrough — start here |
| [BasicEncode](examples/BasicEncode/BasicEncode.ino) | Any | ★☆☆ | Single-sensor FULL packet encode |
| [MultiSensorNodePacket](examples/MultiSensorNodePacket/MultiSensorNodePacket.ino) | Any | ★★☆ | 4-sensor template packet encode + decode |
| [FusionAutoMode](examples/FusionAutoMode/FusionAutoMode.ino) | Any | ★★☆ | Automatic FULL→DIFF→HEART transitions |
| [FusionModeTest](examples/FusionModeTest/FusionModeTest.ino) | Any | ★★☆ | Deterministic FULL→DIFF→HEART test with assertions |
| [PacketMetaDecode](examples/PacketMetaDecode/PacketMetaDecode.ino) | Any | ★★☆ | Decode received packet metadata and recover sensor payload |
| [SecureSessionRoundtrip](examples/SecureSessionRoundtrip/SecureSessionRoundtrip.ino) | Any | ★★★ | Secure session key setup + encrypted packet roundtrip |
| [BootCliOrRun](examples/BootCliOrRun/BootCliOrRun.ino) | ESP32 | ★★★ | Boot into 10 s CLI window, then persistent run mode |
| [ESP32WiFiMultiSensorAuto](examples/ESP32WiFiMultiSensorAuto/ESP32WiFiMultiSensorAuto.ino) | ESP32 | ★★★ | Full Wi-Fi UDP node: 4 sensors, LittleFS ID persistence, auto DIFF |
| [QuickBench](examples/QuickBench/QuickBench.ino) | ESP32 | ★★★ | Dual-core packet encode throughput benchmark |

All examples compile with `arduino-cli compile`. ESP32-specific examples require `esp32:esp32` core v2.x or later.

---

## Repository Map

```
OSynaptic-FX-Arduino/           ← single source of truth
├── CMakeLists.txt          # CMake build (native / cross-compile)
├── cmake/
│   ├── osfx-c99Config.cmake.in
│   └── toolchains/         # arm-cortexm4, rp2040, riscv32, wasm, android, ios
├── library.properties      # Arduino Library Manager metadata
├── keywords.txt            # IDE syntax highlighting
├── src/
│   ├── OSynapticFX.h      # Umbrella include (use this in sketches)
│   ├── OSynapticFX.h      # Umbrella include (use this in sketches)
│   ├── *.c                 # C99 implementation (21 files shared with CMake)
│   ├── osfx_easy.c         # High-level easy API (Arduino + CMake)
│   ├── osfx_storage.c      # Pluggable storage backend (Arduino + CMake)
│   └── osfx_storage_littlefs.c  # LittleFS backend (Arduino IDE only)
├── include/
│   ├── osfx_user_config.h  # ← tune this for memory budget
│   ├── osfx_build_config.h # Include chain root
│   ├── osfx_easy.h         # High-level convenience API
│   ├── osfx_core.h         # Core encode/decode API
│   ├── osfx_fusion_state.h # FULL/DIFF/HEART state machine
│   ├── osfx_template_grammar.h  # Multi-sensor template
│   ├── osfx_secure_session.h    # Encrypted packet sessions
│   └── ...                 # Other module headers
├── examples/               # Arduino sketches (shown in IDE)
├── tests/                  # Native unit + integration tests
├── scripts/                # Build / test / benchmark automation
├── docs/                   # Implementation docs
└── tools/                  # CLI / bench entry programs
```

---

## CMake Build

This repository is also the CMake source for native, cross-compile, and CI builds.

### Native (Linux / macOS / Windows)

```bash
cmake -B build -DOSFX_BUILD_EASY_API=ON
cmake --build build
```

### Cross-compile with a toolchain file

```bash
# RP2040
cmake -B build-rp2040 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/rp2040.cmake \
  -DOSFX_BUILD_EASY_API=ON
cmake --build build-rp2040

# RISC-V 32-bit bare-metal
cmake -B build-riscv \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-elf.cmake
cmake --build build-riscv

# ARM Cortex-M4
cmake -B build-cortexm4 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm-cortexm4.cmake
cmake --build build-cortexm4
```

Available toolchains: `rp2040`, `arm-cortexm4`, `riscv32-elf`, `android-aarch64`, `ios-arm64`, `wasm-emscripten`.

### PowerShell automation (all targets)

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build.ps1 -Compiler auto
powershell -ExecutionPolicy Bypass -File scripts/test.ps1  -Compiler auto
```

### CMake options

| Option | Default | Description |
|---|---|---|
| `OSFX_BUILD_EASY_API` | `OFF` | Include `osfx_easy` + `osfx_storage` modules |
| `OSFX_ENABLE_FILE_IO` | `OFF` | Enable persistent storage |
| `OSFX_ENABLE_CLI` | `OFF` | Enable CLI router |
| `OSFX_ENABLE_LTO` | `OFF` | Link-time optimisation |
| `OSFX_CFG_PAYLOAD_*` | varies | Enable/disable individual payload fields |

> `osfx_storage_littlefs.c` depends on the Arduino LittleFS SDK and is compiled by Arduino IDE only — it is excluded from all CMake builds automatically.

---

## Precompiled Archives

The Arduino IDE compiles from source by default (recommended). Pre-built `.a` archives are included for toolchains that override the source path.

Run `scripts/build.ps1 -Matrix` to (re-)generate all archives; toolchains not on PATH are skipped silently.

| Architecture | CMake preset | Archive path |
|---|---|---|
| x86_64 (host) | `x86_64` | `src/host/libOSynapticFX.a` |
| ARM Cortex-M0+ | `cortexm0plus` | `src/cortex-m0plus/libOSynapticFX.a` |
| ARM Cortex-M3 | `cortexm3` | `src/cortex-m3/libOSynapticFX.a` |
| ARM Cortex-M4 | `cortexm4` | `src/cortex-m4/libOSynapticFX.a` |
| ARM Cortex-M7 | `cortexm7` | `src/cortex-m7/libOSynapticFX.a` |
| ARM Cortex-M33 | `cortexm33` | `src/cortex-m33/libOSynapticFX.a` |
| RISC-V 32 | `riscv32` | `src/riscv32/libOSynapticFX.a` |
| Xtensa (ESP32) | `esp32` | `src/esp32/libOSynapticFX.a` |
| AVR ATmega328P | `avr328p` | `src/avr/atmega328p/libOSynapticFX.a` |
| AVR ATmega2560 | `avr2560` | `src/avr/atmega2560/libOSynapticFX.a` |
| x86_64 (host) | `x86_64` | `src/host/libOSynapticFX.a` |
| ARM Cortex-M0+ | `cortexm0plus` | `src/cortex-m0plus/libOSynapticFX.a` |
| ARM Cortex-M3 | `cortexm3` | `src/cortex-m3/libOSynapticFX.a` |
| ARM Cortex-M4 | `cortexm4` | `src/cortex-m4/libOSynapticFX.a` |
| ARM Cortex-M7 | `cortexm7` | `src/cortex-m7/libOSynapticFX.a` |
| ARM Cortex-M33 | `cortexm33` | `src/cortex-m33/libOSynapticFX.a` |
| RISC-V 32 | `riscv32` | `src/riscv32/libOSynapticFX.a` |
| Xtensa (ESP32) | `esp32` | `src/esp32/libOSynapticFX.a` |
| AVR ATmega328P | `avr328p` | `src/avr/atmega328p/libOSynapticFX.a` |
| AVR ATmega2560 | `avr2560` | `src/avr/atmega2560/libOSynapticFX.a` |

Each archive is compiled with the matching `cmake/arch_presets/<preset>.cmake` memory limits so static DRAM usage is deterministic.

---

## Capability Snapshot

| Module | Feature |
|---|---|
| Codecs | Base62 i64, CRC8, CRC16/CCITT |
| Packet | FULL encode, minimal metadata decode |
| Fusion | FULL → DIFF → HEART state machine, binary bitmask DIFF |
| Security | Secure session persistence, payload AES crypto |
| Transport | Callback-based transport stack, protocol matrix |
| Template | Multi-sensor grammar up to `OSFX_TMPL_MAX_SENSORS` |
| Platform | ID allocator, plugin runtime, CLI router |
| Integration | Unified glue API (`osfx_glue`) |

---

## Documentation Index

### Start here

- [docs/README.md](docs/README.md) — documentation root and module index
- [docs/SUMMARY.md](docs/SUMMARY.md) — structured summary of all modules

### Integration guides

- [docs/17-glue-step-by-step.md](docs/17-glue-step-by-step.md) — glue API integration walkthrough
- [docs/18-data-format-specification.md](docs/18-data-format-specification.md) — normative data format
- [docs/19-input-specification.md](docs/19-input-specification.md) — sensor input field specification

### Protocol references

- [DATA_FORMATS_SPEC.md](DATA_FORMATS_SPEC.md) — strict binary packet format
- [SEND_API_INDEX.md](SEND_API_INDEX.md) — send API index
- [SEND_API_REFERENCE.md](SEND_API_REFERENCE.md) — send API reference
- [QUICK_START_SEND_EXAMPLES.md](QUICK_START_SEND_EXAMPLES.md) — send examples

### Server side (OpenSynaptic)

- [../OpenSynaptic/README.md](../OpenSynaptic/README.md) — Python / Rust server that receives OSynaptic-FX packets
- [../OpenSynaptic/README.md](../OpenSynaptic/README.md) — Python / Rust server that receives OSynaptic-FX packets

---

## Contributing

1. Read `DATA_FORMATS_SPEC.md` before touching any encode/decode path.
2. All changes to `src/` must keep both `scripts/build.ps1 -Compiler auto` (PowerShell) and `cmake -B build && cmake --build build` (CMake) green.
3. Run native tests: `powershell -ExecutionPolicy Bypass -File scripts/test.ps1 -Compiler auto`
4. This is the **single source of truth** — do not sync changes to `OSynaptic-FX/` (that repository is deprecated).
4. This is the **single source of truth** — do not sync changes to `OSynaptic-FX/` (that repository is deprecated).
5. Update docs at the closest ownership level — do not duplicate content between README and `docs/`.

---

## FAQ

**Does this library work without Wi-Fi / network?**  
Yes. The transport layer is callback-based; pass any `write(buf, len)` function — Serial, LoRa, CAN, etc. The `ESP32WiFiMultiSensorAuto` example shows Wi-Fi UDP; replace the send callback for other transports.

**Can I use it without the OpenSynaptic server?**  
Yes — the encoder/decoder is standalone. You can decode packets on a second MCU using `osfx_fusion_decode_apply()` without the server.

**How do I persist the device ID across reboots?**  
Call `osfx_easy_save_ids()` to a file path (LittleFS on ESP32, EEPROM via storage backend on AVR) and `osfx_easy_load_ids()` at boot. See `ESP32WiFiMultiSensorAuto` for a complete example.

**Where do I set per-project memory limits?**  
`include/osfx_user_config.h` — all `OSFX_FUSION_MAX_*`, `OSFX_ID_MAX_ENTRIES`, and `OSFX_CFG_*` macros are `#ifndef`-guarded and can be overridden there without editing library headers.
