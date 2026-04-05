# 08 Release Notes

## Release

- Version: `v1.0.0-p5` (建议标签)
- Date: `2026-04-04`
- Package: `OSynaptic-FX` Arduino library package (`library.properties`) + scoped runtime plugins + maintainer CLI lite

## Highlights

- 完成 `P0~P4` 核心里程碑。
- Arduino Libraries Manager 交付基线已对齐：`library.properties`、`src/OSynapticFX.h`、`examples/`。
- 示例覆盖扩展至 5 个实战场景：单包编码、元数据解包、融合自动模式、安全会话回环、多传感器节点包。
- 插件策略收敛为：`transport` lite、`test_plugin` lite、`port_forwarder` full。
- 新增独立 CLI 入口（`tools/osfx_cli_main.c`）与轻量命令路由。
- 质量闸门支持 `clang/gcc/cl` 编译器矩阵。

## Quality Gate

- 报告文件：`build/quality_gate_report.md`
- 当前结果：三编译器 Native/Integration/CLI Smoke 全 `PASS`。

## Compatibility Notes

- Arduino 使用路径以 `examples/` 为主，CLI 场景用于维护者回归与诊断。
- 目标定位为协议核心与受控平台面，不覆盖 OpenSynaptic 全服务生态。
- 显式排除 `web/sql/dependency_manager/env_guard`。

## Known Non-Blocking Items

- 驱动行为语义仍可继续深度优化（非发布阻塞）。
- 部分平台级运营能力留待后续迭代。


