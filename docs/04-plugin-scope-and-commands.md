# 04 Plugin Scope And Commands

## 范围策略

### Included Plugins

- `transport` (lite)
- `test_plugin` (lite)
- `port_forwarder` (full)

### Excluded Plugins

- `web`
- `sql`
- `dependency_manager`
- `env_guard`
- Other non-goal service plugins

## Command Matrix

| Command | Description |
|---|---|
| `plugin-list` | List currently registered plugins |
| `plugin-load <name> [config]` | Load plugin |
| `plugin-cmd <name> <cmd> [args...]` | Issue command to specified plugin |
| `transport-status` | Quick view of transport status |
| `test-plugin ...` | Execute test_plugin command |
| `port-forwarder ...` | Execute port_forwarder command |

## 插件级命令

### transport

- `status`
- `dispatch <proto|auto> <hex_payload>`

### test_plugin

- `status`
- `run [suite]`

### port_forwarder

- `status`
- `stats`
- `list`
- `add-rule <name> <from_proto> <from_port|*> <to_proto> <to_port>`
- `remove-rule <name>`
- `enable-rule <name> <0|1>`
- `forward <from_proto> <from_port> <hex_payload>`
- `save [path]`
- `load [path]`

## 插件应用映射（具体用途）

| Plugin | Typical Use | Example |
|---|---|---|
| `transport` | 设备侧发包与链路 fallback | `plugin-cmd transport dispatch auto A1B2C3` |
| `test_plugin` | 现场健康检查、OTA 后快速验证 | `plugin-cmd test_plugin run component` |
| `port_forwarder` | 网关协议/端口转发桥接 | `port-forwarder add-rule r1 udp 8080 tcp 9000` |

说明：

- `transport` 更偏“发包执行面”。
- `test_plugin` 更偏“运行时自检面”。
- `port_forwarder` 更偏“网关转发策略面”。

## Quick Examples

```powershell
# 1) View current included plugins
.\\build\osfx_cli_cl.exe plugin-list

# 2) Load scoped plugins
.\\build\osfx_cli_cl.exe plugin-load transport
.\\build\osfx_cli_cl.exe plugin-load test_plugin
.\\build\osfx_cli_cl.exe plugin-load port_forwarder

# 3) Access transport via plugin-cmd
.\\build\osfx_cli_cl.exe plugin-cmd transport status
.\\build\osfx_cli_cl.exe plugin-cmd transport dispatch auto A1B2

# 4) Access test_plugin via plugin-cmd
.\\build\osfx_cli_cl.exe plugin-cmd test_plugin run component

# 5) Access port_forwarder via plugin-cmd
.\\build\osfx_cli_cl.exe plugin-cmd port_forwarder add-rule r1 udp 8080 tcp 9000
.\\build\osfx_cli_cl.exe plugin-cmd port_forwarder forward udp 8080 A1B2C3
```

## Expected Output Examples

- `plugin-list` output contains `transport,test_plugin,port_forwarder`.
- Successful load typically returns `ok=1 loaded=<name>`.
- Command failure typically returns `error=...` (e.g., insufficient parameters or unknown command).
- If calling excluded plugins (e.g., `web`), should return `error=load_failed name=web` or equivalent failure output.


