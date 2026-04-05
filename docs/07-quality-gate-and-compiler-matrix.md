# 07 Quality Gate And Compiler Matrix

> Maintainer QA document. Arduino end users should prioritize `examples/` compile and upload workflows.

## 质量门目标

在发布前验证以下链路均通过：

- Native tests
- Integration tests
- CLI smoke tests
- 多编译器一致性（`clang/gcc/cl`）

## 执行命令

```powershell
# 单编译器（自动选择）
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Compiler auto

# 编译器矩阵
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Matrix
```

## 报告产物

- `build/quality_gate_report.md`

报告字段：

- `Compiler`
- `Native`
- `Integration`
- `CLI Smoke`
- `Status`

## 报告查看示例

```powershell
Get-Content .\\build\quality_gate_report.md
```

示例判断：

- 若 `clang/gcc/cl` 三行均为 `PASS`，可进入发布验收。
- 若某行为 `FAIL`，先按该编译器做单点复现。

## 当前基线结果

基于最新报告：`clang/gcc/cl` 三编译器全部 `PASS`。

## 失败处理建议

1. 先定位失败阶段（build/native/integration/cli）。
2. 复跑单编译器验证，缩小问题域。
3. 修复后再执行 `-Matrix` 完整门禁。

## 分层排障示例

```powershell
# 仅复跑 clang
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Compiler clang

# 仅复跑 cl
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Compiler cl
```

常见场景：

- `native` 失败：优先检查新增 C 文件是否进入库构建。
- `integration` 失败：优先检查协议行为回归（packet/meta）。
- `CLI Smoke` 失败：优先检查 `tools/osfx_cli_main.c` 与 `osfx_cli_lite` 路由命令。


