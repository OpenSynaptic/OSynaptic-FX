# 16 Examples Cookbook

本文档提供发布级“可复制端到端示例”。

当前以 Arduino 示例为主路径，每个场景都对应 `examples/` 下可直接编译的 Sketch；CLI 场景保留为维护者回归与诊断路径。

## Arduino 实战示例（主路径）

### 统一编译命令模板

```powershell
arduino-cli compile --fqbn arduino:avr:uno .\examples\<ExampleFolder>
```

### 场景 A：单传感器编码上报

- 示例：`examples/BasicEncode/BasicEncode.ino`
- 适用：温度/压力等单传感器定时上报
- 关键 API：`osfx_core_encode_sensor_packet`
- 预期：串口输出 `osfx encode ok=1` 与有效 packet 长度

### 场景 B：包元数据解包与业务字段还原

- 示例：`examples/PacketMetaDecode/PacketMetaDecode.ino`
- 适用：网关侧对上行包做审计与追踪
- 关键 API：`osfx_packet_decode_meta` + `osfx_packet_extract_body`
- 预期：可打印 `source_aid/tid`，并还原 `sensor_id/value/unit`

### 场景 C：融合自动模式（FULL/DIFF）

- 示例：`examples/FusionAutoMode/FusionAutoMode.ino`
- 适用：连续采样场景下减少重复传输开销
- 关键 API：`osfx_core_encode_sensor_packet_auto` + `osfx_core_decode_sensor_packet_auto`
- 预期：两次发送的命令字可能不同（取决于状态机），均可正确解码

### 场景 D：安全会话加密回环

- 示例：`examples/SecureSessionRoundtrip/SecureSessionRoundtrip.ino`
- 适用：边缘节点与网关建立受控加密数据链路
- 关键 API：`osfx_secure_note_plaintext_sent` / `osfx_secure_confirm_dict` / `osfx_core_encode_sensor_packet_secure`
- 预期：`should_encrypt=1`，解密后业务值与明文一致

### 场景 E：多传感器节点聚合包

- 示例：`examples/MultiSensorNodePacket/MultiSensorNodePacket.ino`
- 适用：一个节点聚合多个采样点后批量发送
- 关键 API：`osfx_core_encode_multi_sensor_packet_auto` + `osfx_core_decode_multi_sensor_packet_auto`
- 预期：可打印节点信息与多条传感器解析结果

### 场景 F：Easy API 最小上手路径

- 示例：`examples/EasyQuickStart/EasyQuickStart.ino`
- 适用：希望以最少步骤完成 `setup/loop` 编码流程的 Arduino 用户
- 关键 API：`osfx_easy_init` + `osfx_easy_allocate_aid` + `osfx_easy_encode_sensor_auto`
- 预期：串口周期输出 `cmd=<FULL/DIFF/HEART> len=<N> aid=<id>`

### 场景 G：融合状态序列回归（FULL/DIFF/HEART）

- 示例：`examples/FusionModeTest/FusionModeTest.ino`
- 适用：验证融合状态机在固定输入下的命令序列稳定性
- 关键 API：`osfx_core_encode_multi_sensor_packet_auto` + `osfx_packet_decode_meta`
- 预期：`CASE-1/2/3` 分别命中 `FULL/DIFF/HEART`

### 场景 H：启动窗口 CLI + 持久运行模式

- 示例：`examples/BootCliOrRun/BootCliOrRun.ino`
- 适用：需要运行时插件控制、ID 持久化和诊断命令的节点
- 关键 API：`osfx_platform_runtime_*` + `osfx_id_save/load` + `osfx_storage_use_littlefs`
- 预期：10 秒内输入 `cli` 进入命令模式，否则进入持续发送模式

### 场景 I：双核编码吞吐压测（ESP32）

- 示例：`examples/QuickBench/QuickBench.ino`
- 适用：维护者做性能基线对比、改动前后吞吐回归检查
- 关键 API：`osfx_packet_encode_full`（双核并行任务）
- 预期：输出双核吞吐、错误数与堆使用统计

---

## 维护者原生回归场景（辅助路径）

## 通用前置

```powershell
# 1) 先构建
powershell -ExecutionPolicy Bypass -File .\\scripts\build.ps1 -Compiler auto

# 2) 再跑测试（确保当前环境可用）
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Compiler auto
```

可执行文件示例（按编译器不同可能是 `osfx_cli_cl.exe` / `osfx_cli_clang.exe` / `osfx_cli_gcc.exe`）：

`build/osfx_cli_cl.exe`

---

## 场景 1：插件范围校验（纳入与排除）

### 目标

验证当前构建仅包含 `transport`、`test_plugin`、`port_forwarder`。

### 命令

```powershell
.\\build\osfx_cli_cl.exe plugin-list
.\\build\osfx_cli_cl.exe plugin-load web
```

### 预期输出

- `plugin-list` 包含：`transport,test_plugin,port_forwarder`
- 不应包含：`web/sql/dependency_manager/env_guard`
- `plugin-load web` 返回失败（例如 `error=load_failed name=web`）

### 失败分支

- 若 `plugin-list` 缺少 `port_forwarder`：说明运行时注册路径异常。
- 若 `web` 可加载：说明范围收敛策略被破坏。

### 排障

- 检查 `src/osfx_platform_runtime.c` 注册列表。
- 检查 `docs/04-plugin-scope-and-commands.md` 与实现是否一致。

---

## 场景 2：`plugin-load` + `transport-status` 基础链路

### 目标

确认插件加载与状态查询可用。

### 命令

```powershell
.\\build\osfx_cli_cl.exe plugin-load transport
.\\build\osfx_cli_cl.exe transport-status
```

### 预期输出

- 第一条返回：`ok=1 loaded=transport`
- 第二条包含：`transport initialized=1`

### 失败分支

- 返回 `error=load_failed`：插件加载失败。
- 返回 `error=...` 且无 `initialized=1`：状态命令路由失败。

### 排障

- 检查 `src/osfx_plugin_transport.c` 的 `init` / `status` 实现。
- 检查 `src/osfx_cli_lite.c` 的 `transport-status` 路由。

---

## 场景 3：`transport dispatch` 数据投递

### 目标

验证 `transport` 插件可将 hex 负载经协议矩阵发送。

### 命令

```powershell
.\\build\osfx_cli_cl.exe plugin-load transport
.\\build\osfx_cli_cl.exe plugin-cmd transport dispatch auto A1B2C3
```

### 输入说明

- `A1B2C3`：十六进制 payload
- `auto`：由 runtime 自动选择协议路径

### 预期输出

- 返回包含：`ok=1 used=<proto> bytes=3`

### 失败分支

- `error=invalid_hex_payload`：hex 非法。
- `error=dispatch_failed`：无可用 driver 或发送失败。

### 排障

- 检查 `src/osfx_protocol_matrix.c` 默认 driver 是否注册。
- 检查 `osfx_transporter_runtime` 的启用状态。

---

## 场景 4：`test_plugin` 轻量回归

### 目标

验证测试插件命令和状态统计可用。

### 命令

```powershell
.\\build\osfx_cli_cl.exe plugin-load test_plugin
.\\build\osfx_cli_cl.exe test-plugin run component
.\\build\osfx_cli_cl.exe test-plugin status
```

### 预期输出

- `run component` 返回：`ok=1 suite=component`
- `status` 包含：`initialized=1` 且 `total_runs/pass_runs` 递增

### 失败分支

- `error=unknown_cmd`：子命令写错。
- `initialized=0`：插件未成功加载。

### 排障

- 检查 `src/osfx_plugin_test.c` 的 `run/status` 分支。

---

## 场景 5：`port_forwarder` 规则闭环

### 目标

验证规则添加、转发命中、统计更新。

### 命令

```powershell
.\\build\osfx_cli_cl.exe plugin-load port_forwarder
.\\build\osfx_cli_cl.exe port-forwarder add-rule r1 udp 8080 tcp 9000
.\\build\osfx_cli_cl.exe port-forwarder list
.\\build\osfx_cli_cl.exe port-forwarder forward udp 8080 A1B2
.\\build\osfx_cli_cl.exe port-forwarder stats
```

### 预期输出

- `add-rule` 返回：`ok=1 added=r1`
- `list` 包含规则 `r1`
- `forward` 返回：`ok=1 route=tcp:9000 bytes=2`
- `stats` 的 `forwarded` 增加

### 失败分支

- `error=usage add-rule ...`：参数顺序错误。
- `error=no_match_or_emit_failed`：规则不匹配或 emit 回调失败。

### 排障

- 检查 `from_proto/from_port` 是否与 `forward` 命令一致。
- 先执行 `list` 确认规则生效。

---

## 场景 6：`port_forwarder` 持久化闭环

### 目标

验证规则可保存并重新加载。

### 命令

```powershell
.\\build\osfx_cli_cl.exe port-forwarder add-rule r2 udp * tcp 9100
.\\build\osfx_cli_cl.exe port-forwarder save E:\\OSynaptic-FX\\build\\pf_rules.txt
.\\build\osfx_cli_cl.exe port-forwarder load E:\\OSynaptic-FX\\build\\pf_rules.txt
.\\build\osfx_cli_cl.exe port-forwarder list
```

### 预期输出

- `save` 返回：`ok=1 saved=...`
- `load` 返回：`ok=1 loaded=...`
- `list` 仍可见规则 `r2`

### 失败分支

- `error=save_failed`：路径不可写。
- `error=load_failed`：路径不存在或格式异常。

### 排障

- 检查路径权限、文件是否被占用。
- 先用绝对路径验证，再回收为相对路径。

---

## 场景 7：质量门与性能门联动验收

### 目标

在发布流程中一次性校验构建、测试、性能与内存锁。

### 命令

```powershell
powershell -ExecutionPolicy Bypass -File .\\scripts\test.ps1 -Matrix
powershell -ExecutionPolicy Bypass -File .\\scripts\bench.ps1 -Compiler auto -MemoryLimitKB 16
Get-Content .\\build\quality_gate_report.md
Get-Content .\\build\bench\bench_report.md
```

### 预期输出

- `quality_gate_report.md` 中 `clang/gcc/cl` 全部 `PASS`
- `bench_report.md` 中 `RAM memory lock status: PASS`

### 失败分支

- 矩阵失败：优先单编译器复跑定位。
- 内存锁失败：先放宽阈值做诊断（如 `-MemoryLimitKB 64`），再回归 16KB。


