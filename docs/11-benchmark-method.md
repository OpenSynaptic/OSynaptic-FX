# 11 Benchmark Method

> Maintainer performance-calibration document. Arduino users can skip this unless they are validating release-level performance baselines.

## 目标指标

- 压缩率：相对紧凑 JSON 基线的占比与缩减百分比。
- 编码时延：`osfx_core_encode_multi_sensor_packet_auto` 的 `P50/P95`（us）。
- 编码速度：平均每传感器微秒与 `sensors/s` 吞吐。
- RAM：工作集前后差值 + 关键结构体静态字节数。

## 分档

- 传感器分档：`1 / 4 / 8 / 16`
- 每档预热：`500` 次
- 每档采样：`30000` 次

## 输出

- CSV：`build/bench/bench_report.csv`
- Markdown：`build/bench/bench_report.md`

## 运行命令

```powershell
# 默认带 16KB 内存上限锁
powershell -ExecutionPolicy Bypass -File .\\scripts\bench.ps1 -Compiler auto

# 覆盖默认上限（单位 KB）
powershell -ExecutionPolicy Bypass -File .\\scripts\bench.ps1 -Compiler auto -MemoryLimitKB 64

# 显式关闭内存上限锁
powershell -ExecutionPolicy Bypass -File .\\scripts\bench.ps1 -Compiler auto -MemoryLimitKB 0
```

## 口径说明

- JSON 基线使用紧凑格式（无空格）并包含 node/state/sensors 列表。
- 延迟为单次编码调用墙钟时间（QPC）。
- `per-sensor us` = `per-packet us / sensor_count`。
- RAM 为同进程工作集对比，不等同于 MCU 裸机静态内存预算。
- 默认内存上限锁为 `16KB`（可通过 `-MemoryLimitKB` 覆盖）。
- `-MemoryLimitKB` 大于 0 时启用内存上限锁：`working_set_delta_kb > limit` 则基准返回失败。
- `-MemoryLimitKB 0` 时关闭内存上限锁。
- 即使超限，仍会生成 `bench_report.csv` 与 `bench_report.md`，便于发布复盘。


