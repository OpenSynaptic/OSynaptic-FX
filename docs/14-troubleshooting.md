# 14 Troubleshooting

> Troubleshooting here is maintainers-first (native scripts/CLI). For Arduino usage issues, start from `examples/` and board core setup.

## Build/Test Failures

### Symptom

- `No C compiler found (clang/gcc/cl)`

### Action

- Install one toolchain and re-run:

```powershell
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Compiler auto
```

## Matrix Gate Failure

### Symptom

- `quality gate failed; see ...quality_gate_report.md`

### Action

- Re-run failing compiler only:

```powershell
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Compiler clang
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Compiler gcc
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Compiler cl
```

## CLI Smoke Failure

### Symptom

- `CLI smoke failed: ...`

### Action

- Verify standalone CLI build and command routing:

```powershell
.\\build\osfx_cli_cl.exe plugin-list
.\\build\osfx_cli_cl.exe transport-status
```

## Benchmark Memory Lock Failure

### Symptom

- `bench_failed=1 reason=mem_limit_exceeded`

### Action

- Inspect memory lines in `bench_report.md`.
- Relax threshold for diagnosis:

```powershell
powershell -ExecutionPolicy Bypass -File .\\scripts\bench.ps1 -Compiler auto -MemoryLimitKB 64
```

- If needed, disable lock temporarily:

```powershell
powershell -ExecutionPolicy Bypass -File .\\scripts\bench.ps1 -Compiler auto -MemoryLimitKB 0
```


