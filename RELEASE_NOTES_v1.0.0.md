# OSynaptic-FX v1.0.0 — First Stable Release

We are excited to announce the **first stable release** of OSynaptic-FX — an embedded-first C99 protocol and runtime library for Arduino that encodes multi-sensor readings into compact binary packets and streams them to the [OpenSynaptic](https://github.com/opensynaptic) server over any transport (UDP / UART / LoRa / MQTT / CAN).

---

## What Is OSynaptic-FX?

OSynaptic-FX is the MCU-side half of the OpenSynaptic sensor telemetry stack. A single `#include <OSynapticFX.h>` gives your sketch:

- **Compact binary encoding** — UCUM-normalised tag:value bodies with per-field type tags, optimised for low-bandwidth transports.
- **Automatic FULL → DIFF → HEART transitions** — the library decides what to send; you just call `osfx_easy_encode_sensor_auto()`.
- **Spec-compatible wire format** — packets decoded directly by the OpenSynaptic Rust DLL (`auto_decompose_input_inner`) and Python codec without custom glue.
- **Zero dynamic allocation option** — set `OSFX_CFG_MULTI_SENSOR_STATIC_SCRATCH 1` and every buffer lives in static storage.
- **Portable C99** — compiles clean on AVR, ESP32, STM32, RP2040, RISC-V, Cortex-M, and host x86_64 with `-Wall -Wextra -Werror`.

---

## Install in 30 Seconds

**Arduino IDE:** `Sketch > Include Library > Manage Libraries` → search **OSynaptic-FX** → Install

**Arduino CLI:**
```bash
arduino-cli lib install "OSynaptic-FX"
```

**Manual ZIP install:** download `OSynaptic-FX-1.0.0.zip` from the assets below and use `Sketch > Include Library > Add .ZIP Library…`

---

## Quick Start

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

    int len = 0; uint8_t cmd = 0;
    uint64_t ts = 1710000000ULL + millis() / 1000ULL;

    if (osfx_easy_encode_sensor_auto(&g_ctx, &s, 1, ts, g_buf, sizeof(g_buf), &len, &cmd) == 0)
        Serial.write(g_buf, len);   // or UDP.write / Serial2.write / ...

    delay(1000);
}
```

Open Serial Monitor at **115200 baud** and watch binary DIFF packets stream out.

---

## What's New in v1.0.0

### Binary DIFF protocol — production-ready wire format

The biggest change in this release. The library now emits **protocol-native binary bitmask DIFF packets** that are directly compatible with the OpenSynaptic server:

```
[ mask_bytes (big-endian, ceil(N/8) bytes) ]
  for each changed slot i (META_0, VAL_0, META_1, VAL_1 ...):
    [ uint8 length ] [ value bytes ]
```

The previous plain-text body format caused `"Malformed DIFF payload"` errors server-side. Both FULL and DIFF paths are now validated against the OpenSynaptic Python/Rust codec.

### ESP32 DRAM safety

Eliminated a `.dram0.bss` overflow of **16,568 bytes** on ESP32 by tuning all default table sizes:

| Config key | Old default | New default |
|---|---|---|
| `OSFX_FUSION_MAX_ENTRIES` | 64 | 32 |
| `OSFX_FUSION_MAX_VALS` | 16 | 8 |
| `OSFX_ID_MAX_ENTRIES` | 1024 | 128 |

Total typical sketch DRAM: **~20 KB** (was ~90 KB). All limits remain `#ifndef`-overridable.

### Struct-level memory optimisation

- Integer fields (`sensor_count`, `val_count`, `used`) narrowed from `size_t`/`int` to `uint8_t` — saves ~16 B per fusion entry.
- Optional payload fields (`geohash_id`, `supplementary_message`, `resource_url`) wrapped in `#if OSFX_CFG_PAYLOAD_*` guards — saves up to 288 B/sensor when disabled.
- `sig_base_len` redundant field removed.

### Single-file compile-time tuning

All configuration is now concentrated in `include/osfx_user_config.h`. No library header edits needed — just override the constants you need before including any OSynaptic-FX header.

---

## Supported Platforms

Precompiled static libraries are included in the Arduino ZIP for each `build.mcu` value:

| Library file | Target | Toolchain |
|---|---|---|
| `src/cortex-m0plus/libOSynapticFX.a` | Cortex-M0/M0+ (RP2040, STM32C0) | `arm-none-eabi-gcc -mcpu=cortex-m0plus` |
| `src/cortex-m3/libOSynapticFX.a` | Cortex-M3 (STM32F1) | `arm-none-eabi-gcc -mcpu=cortex-m3` |
| `src/cortex-m4/libOSynapticFX.a` | Cortex-M4 (STM32F4, SAMD51) | `arm-none-eabi-gcc -mcpu=cortex-m4 -mfpu=fpv4-sp-d16` |
| `src/cortex-m7/libOSynapticFX.a` | Cortex-M7 (STM32H7, i.MX RT) | `arm-none-eabi-gcc -mcpu=cortex-m7 -mfpu=fpv5-d16` |
| `src/cortex-m33/libOSynapticFX.a` | Cortex-M33 (STM32C5, LPC55) | `arm-none-eabi-gcc -mcpu=cortex-m33 -mfpu=fpv5-sp-d16` |
| `src/riscv32/libOSynapticFX.a` | RISC-V rv32imc (ESP32-C3, CH32V) | `riscv64-unknown-elf-gcc -march=rv32imc` |
| `src/esp32/libOSynapticFX.a` | Xtensa LX6/LX7 (ESP32, ESP32-S3) | `xtensa-esp32-elf-gcc` via IDF |
| `src/atmega328p/libOSynapticFX.a` | AVR ≤64 KB Flash (ATmega328P) | `avr-gcc -mmcu=atmega328p` |
| `src/atmega2560/libOSynapticFX.a` | AVR ≥128 KB Flash (ATmega2560) | `avr-gcc -mmcu=atmega2560` |

Standalone libraries for host development are also published as separate release assets:

| Asset | Platform |
|---|---|
| `libosfx_x86_64_linux.a` | Linux x86\_64 |
| `libosfx_x86_64_windows.lib` | Windows x86\_64 (MinGW) |
| `libosfx_x86_64_macos.a` | macOS x86\_64 |

---

## Examples

| Example | Board | Difficulty |
|---|---|---|
| [EasyQuickStart](examples/EasyQuickStart/) | Any | ★☆☆ |
| [BasicEncode](examples/BasicEncode/) | Any | ★☆☆ |
| [MultiSensorNodePacket](examples/MultiSensorNodePacket/) | Any | ★★☆ |
| [FusionAutoMode](examples/FusionAutoMode/) | Any | ★★☆ |
| [FusionModeTest](examples/FusionModeTest/) | Any | ★★☆ |
| [PacketMetaDecode](examples/PacketMetaDecode/) | Any | ★★☆ |
| [SecureSessionRoundtrip](examples/SecureSessionRoundtrip/) | Any | ★★★ |
| [BootCliOrRun](examples/BootCliOrRun/) | ESP32 | ★★★ |
| [ESP32WiFiMultiSensorAuto](examples/ESP32WiFiMultiSensorAuto/) | ESP32 | ★★★ |
| [QuickBench](examples/QuickBench/) | ESP32 | ★★★ |

---

## Breaking Changes from v0.2.0

| Area | Change |
|---|---|
| Binary DIFF wire format | Native bitmask DIFF; server must be OpenSynaptic ≥ codec.py fix |
| `osfx_fusion_entry` layout | `sig_base_len` removed; integer fields now `uint8_t` |
| `osfx_sensor_slot` layout | Optional fields conditional on `OSFX_CFG_PAYLOAD_*` |
| `OSFX_ID_MAX_ENTRIES` default | 1024 → 128 |
| `OSFX_FUSION_MAX_ENTRIES` default | 64 → 32 |
| `OSFX_FUSION_MAX_VALS` default | 16 → 8 |

---

## Quality Gate

All code in this release passes the three-compiler quality gate (**GCC · Clang · MSVC**) with `-Wall -Wextra -Werror -std=c99` and the full native unit + integration test suite. Benchmark reports are attached below.

---

## Full Documentation

- [Architecture overview](docs/01-overview.md)
- [API reference](docs/03-api-index.md)
- [Tuning guide & memory budgets](docs/12-config-quick-reference.md)
- [Binary DIFF protocol specification](docs/18-data-format-specification.md)
- [Troubleshooting](docs/14-troubleshooting.md)
- [Examples cookbook](docs/16-examples-cookbook.md)

---

*MIT License — contributions welcome, see [CONTRIBUTING.md](CONTRIBUTING.md)*
