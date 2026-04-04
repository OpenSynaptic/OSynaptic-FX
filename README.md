# OSynapptic-FX

![C99](https://img.shields.io/badge/C-99-00599C?logo=c&logoColor=white)
![Focus](https://img.shields.io/badge/focus-embedded%20runtime-0A7EA4)
![Architecture](https://img.shields.io/badge/architecture-spec%20driven-6A5ACD)
![Status](https://img.shields.io/badge/status-active%20development-2E8B57)
![Docs](https://img.shields.io/badge/docs-structured-4169E1)
![License](https://img.shields.io/badge/license-see%20project%20files-696969)
![CI](https://github.com/ChrisLee0721/OSynapptic-FX/actions/workflows/build.yml/badge.svg)

![ESP32](https://img.shields.io/badge/ESP32-supported-E7352C?logo=espressif&logoColor=white)
![STM32](https://img.shields.io/badge/STM32-supported-03234B?logo=stmicroelectronics&logoColor=white)
![RISC--V](https://img.shields.io/badge/RISC--V-supported-283272?logo=riscv&logoColor=white)
![RP2040](https://img.shields.io/badge/RP2040-supported-B41F47?logo=raspberrypi&logoColor=white)
![Android NDK](https://img.shields.io/badge/Android%20NDK-supported-3DDC84?logo=android&logoColor=white)
![iOS](https://img.shields.io/badge/iOS-supported-000000?logo=apple&logoColor=white)
![WebAssembly](https://img.shields.io/badge/WebAssembly-supported-654FF0?logo=webassembly&logoColor=white)

![Linux](https://img.shields.io/badge/Linux-build-222222?logo=linux&logoColor=white)
![Windows](https://img.shields.io/badge/Windows-build-0078D6?logo=windows&logoColor=white)
![macOS](https://img.shields.io/badge/macOS-build-111111?logo=apple&logoColor=white)

Embedded-first OpenSynaptic C99 workspace.

> 当前仓库已经整合到根目录：实现代码、脚本、测试和文档均以根路径为准。

## Why OSynapptic-FX

- **Embedded-first delivery**: 面向嵌入式交付的 C99 主实现，路径清晰、依赖可控。
- **Spec-driven engineering**: 规范先行，代码与测试围绕协议/数据格式契约实现。
- **Operational runtime stack**: 包含 transport/runtime matrix、glue 编排与插件能力。
- **Repeatable quality workflow**: build/test/matrix/benchmark 脚本统一，产物可复验。
- **Single-root structure**: 单根目录结构，减少路径切换和文档漂移。

## Who This Repo Is For

- 平台与固件工程师（集成 C99 协议栈）
- 维护者（验证协议/运行时行为）
- 贡献者（从规范到实现到测试）

## Repository Map

### Core implementation

- `include/` - public headers (`osfx_core.h` and module APIs)
- `src/` - C99 module implementations
- `CMakeLists.txt` - CMake static-library entry
- `tests/` - native unit and integration tests
- `tools/` - CLI / benchmark entry programs
- `scripts/` - build/test/benchmark automation
- `docs/` - implementation-focused documentation

### Reference context

- `OpenSynaptic/` - reference project context (non-primary delivery path)

### Root-level specs and guides

- `DATA_FORMATS_SPEC.md` - strict data format specification
- `SEND_API_INDEX.md` - send API index
- `SEND_API_REFERENCE.md` - send API reference
- `QUICK_START_SEND_EXAMPLES.md` - send quick-start examples

## 3-Minute Quick Start

From repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Compiler auto
powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1 -Compiler auto
```

If you want explicit config path:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Compiler auto -ConfigPath .\Config.json
powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1 -Compiler auto -ConfigPath .\Config.json
```

Optional quality/benchmark runs:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1 -Matrix
powershell -ExecutionPolicy Bypass -File .\scripts\bench.ps1 -Compiler auto
```

## CMake Quick Start

```powershell
cmake -S . -B build_cmake
cmake --build build_cmake --config Release
```

Example with feature overrides:

```powershell
cmake -S . -B build_cmake -DOSFX_CFG_PLUGIN_TEST_PLUGIN=OFF -DOSFX_CFG_PAYLOAD_GEOHASH_ID=ON
cmake --build build_cmake --config Release
```

## Output Artifacts

- Static library and binaries: `build/`
- Matrix quality report: `build/quality_gate_report.md`
- Benchmark reports:
  - `build/bench/bench_report.csv`
  - `build/bench/bench_report.md`

## Capability Snapshot (C99)

- Base codecs: Base62 i64, CRC8, CRC16/CCITT
- Packet path: FULL encode + minimal metadata decode
- State path: FULL/DIFF/HEART fusion transitions
- Security: secure session persistence + payload crypto
- Runtime: callback transport stack + protocol matrix
- Data model: multi-sensor template grammar path
- Platform: ID allocator persistence + plugin runtime + CLI router
- Integration: unified glue API (`osfx_glue`)

## Documentation Index

### Start here

- Root docs index: `docs/README.md`
- Structured summary: `docs/SUMMARY.md`

### Integration and normative docs

- Glue integration walkthrough: `docs/17-glue-step-by-step.md`
- Normative data format (split-out section A): `docs/18-data-format-specification.md`
- Input specification: `docs/19-input-specification.md`

### Root protocol/spec references

- Strict data formats: `DATA_FORMATS_SPEC.md`
- Send API index: `SEND_API_INDEX.md`
- Send API reference: `SEND_API_REFERENCE.md`
- Send examples: `QUICK_START_SEND_EXAMPLES.md`

## Project Boundaries

To keep docs maintainable and avoid duplicate sources of truth:
- Root `README.md` is the project-level portal.
- `docs/` owns implementation workflows and module-level details.
- Root spec files own protocol/API normative references.
- `OpenSynaptic/` is retained as reference context.

## Suggested Contributor Workflow

1. Read required spec first (`DATA_FORMATS_SPEC.md` or send API docs).
2. Implement/update C99 code in `src/` and headers in `include/`.
3. Run native tests via `scripts/test.ps1`.
4. Run matrix/benchmark when changing runtime/protocol-sensitive paths.
5. Update docs at the closest ownership level (root portal vs module docs).

## FAQ

### Is `OpenSynaptic/` the implementation target in this repository?

No. Active implementation work is in root-level C99 paths (`src/`, `include/`, `scripts/`, `tests/`).

### Where should I start for API-level send usage?

Start with `SEND_API_INDEX.md`, then `SEND_API_REFERENCE.md`, then `QUICK_START_SEND_EXAMPLES.md`.

### Where is the split normative data-format section from glue doc?

Use `docs/18-data-format-specification.md`.
