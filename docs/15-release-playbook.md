# 15 Release Playbook

## Goal

Produce an Arduino-library-first release package with validated example compile checks, quality gate evidence, and complete docs.

## Step 0: Arduino Package Integrity

- Verify `library.properties` version and metadata completeness.
- Verify `src/OSynapticFX.h` remains the public Arduino include entry.
- Ensure `examples/` includes at least 5 practical sketches.

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\examples\BasicEncode
arduino-cli compile --fqbn arduino:avr:uno .\examples\MultiSensorNodePacket
```

## Step 1: Build + Tests

```powershell
powershell -ExecutionPolicy Bypass -File .\\scripts\build.ps1 -Compiler auto
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Matrix
```

## Step 2: Benchmark Gate

```powershell
powershell -ExecutionPolicy Bypass -File .\\scripts\bench.ps1 -Compiler auto -MemoryLimitKB 16
```

## Step 3: Verify Reports

- `build/quality_gate_report.md`
- `build/bench/bench_report.md`
- `build/bench/bench_report.csv`

## Step 4: Update Release Docs

- `docs/08-release-notes.md`
- `docs/09-mirror-coverage-report.md`
- `docs/13-performance-summary.md`
- `docs/CHANGELOG.md`

## Step 5: Final Acceptance

Use checklist in `docs/10-acceptance-checklist.md` and mark all items complete.

## Deliverable Set

- Arduino metadata (`library.properties`)
- Arduino include entry (`src/OSynapticFX.h`)
- Practical sketches (`examples/`)
- Static library output (`libosfx_core.a` / `osfx_core.lib`, maintainer artifact)
- Quality gate report
- Benchmark reports
- Release notes + mirror coverage + changelog


