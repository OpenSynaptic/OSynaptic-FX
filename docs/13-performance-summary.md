# 13 Performance Summary

> Maintainer release-QA reference. Arduino integrators can treat this as optional background.

## Benchmark Baseline

Source report: `build/bench/bench_report.md`

Latest baseline (with `MemoryLimitKB=16`):

| Sensors | Reduction vs JSON | P50 per-sensor us | P95 per-sensor us | Sensors/s |
|---|---:|---:|---:|---:|
| 1 | 70.41% | 0.600 | 0.700 | 1501773 |
| 4 | 58.33% | 0.425 | 0.450 | 2243708 |
| 8 | 54.73% | 0.425 | 0.438 | 2313790 |
| 16 | 52.69% | 0.425 | 0.438 | 2298422 |

## RAM Lock Baseline

- `RAM working set delta: 12 KB`
- `RAM memory lock limit: 16 KB`
- `RAM memory lock status: PASS`

## Release Threshold Guidance

- Compression reduction should remain above `50%` for `4/8/16` tiers.
- `P95 per-sensor us` should remain below `1.0 us` on baseline host.
- RAM lock should remain `PASS` under `MemoryLimitKB=16`.

## Re-run Command

```powershell
powershell -ExecutionPolicy Bypass -File .\\scripts\bench.ps1 -Compiler auto -MemoryLimitKB 16
```


