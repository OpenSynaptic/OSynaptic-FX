# 10 Acceptance Checklist

## A. Arduino Package Baseline

- [ ] `library.properties` 字段完整（name/version/includes/architectures）。
- [ ] `src/OSynapticFX.h` 可被 Arduino Sketch 直接 include。
- [ ] `examples/` 至少包含 5 个可编译实战案例。
- [ ] 至少在一个 FQBN 上通过示例编译（建议 `arduino:avr:uno`）。

## B. Build & Test (Maintainer)

- [ ] `build.ps1 -Compiler auto` 通过。
- [ ] `test.ps1 -Compiler auto` 通过。
- [ ] `test.ps1 -Matrix` 通过。
- [ ] `build/quality_gate_report.md` 存在且三编译器为 `PASS`。

## C. Plugin Scope

- [ ] `plugin-list` 仅包含 `transport` / `test_plugin` / `port_forwarder`。
- [ ] 未纳入 `web/sql/dependency_manager/env_guard`。
- [ ] `port_forwarder` 规则添加、转发、统计、持久化可用。

## D. CLI

- [ ] `plugin-load` 可加载三类 scoped 插件。
- [ ] `plugin-cmd` 可成功路由到目标插件。
- [ ] `transport-status` / `test-plugin` / `port-forwarder` 子命令可用。

## E. Docs & Release

- [ ] `docs/` 文档库完整可读。
- [ ] `docs/08-release-notes.md` 已更新版本与日期。
- [ ] `docs/09-mirror-coverage-report.md` 与当前策略一致。
- [ ] `docs/12-config-quick-reference.md` 与当前脚本参数一致。
- [ ] `docs/13-performance-summary.md` 已同步最新基准结果。
- [ ] `docs/14-troubleshooting.md` 包含当前主要故障路径。
- [ ] `docs/15-release-playbook.md` 可直接执行发布流程。
- [ ] `docs/16-examples-cookbook.md` 场景示例可复制并通过抽检。
- [ ] `docs/17-glue-step-by-step.md` 可用于独立完成 glue 集成。
- [ ] 发布包包含 README、质量报告、Release Notes、镜像覆盖报告。

## F. 验收命令示例

```powershell
# 0) Arduino 示例编译抽检
arduino-cli compile --fqbn arduino:avr:uno .\examples\BasicEncode
arduino-cli compile --fqbn arduino:avr:uno .\examples\SecureSessionRoundtrip

# 1) 基础构建与测试
powershell -ExecutionPolicy Bypass -File .\\scripts\build.ps1 -Compiler auto
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Compiler auto

# 2) 编译器矩阵
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Matrix

# 3) 检查质量报告
Get-Content .\\build\quality_gate_report.md

# 4) 抽检 CLI
.\\build\osfx_cli_cl.exe plugin-list
.\\build\osfx_cli_cl.exe transport-status
.\\build\osfx_cli_cl.exe test-plugin run component
```

## G. 验收结果判定

- Arduino 示例编译抽检必须成功，且示例数量不低于 5。
- 质量报告中 `clang/gcc/cl` 必须全部 `PASS`。
- CLI 抽检命令返回不应出现 `error=`。
- 若任一项失败，先修复后重新执行 A~F 全流程。


