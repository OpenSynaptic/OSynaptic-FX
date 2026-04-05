# 05 Port Forwarder (Full)

## Capability Boundary

`port_forwarder` is a full implementation within the current scope, covering rule creation/deletion/modification/query, hit statistics, forwarding execution, rule persistence, and loading.

## Rules Model

- Rule fields: `name`, `from_proto`, `from_port|*`, `to_proto`, `to_port`, `enabled`, `hit_count`.
- Matching logic: Scan in registration order, execute forwarding on first match.
- Port policy: `*` represents wildcard for source port.

## Runtime Statistics

- Plugin-level: `total_forwarded`, `total_dropped`
- Rule-level: `hit_count`

## Command Description

- `status`: Returns initialization status and total forwarded/dropped statistics.
- `stats`: Returns statistics summary.
- `list`: Returns rules list.
- `add-rule`: Add or overwrite rule with same name.
- `remove-rule`: Remove rule.
- `enable-rule`: Enable/disable rule.
- `forward`: Input source protocol+port+hex data, execute match-based forwarding.
- `save/load`: Rule persistence.

## Persistence Format

- File is line-by-line text record with comma-separated fields.
- Recommended to keep path fixed in release directory and include in backup strategy.

## Typical Workflow

1. `plugin-load port_forwarder`
2. `port-forwarder add-rule r1 udp 8080 tcp 9000`
3. `port-forwarder forward udp 8080 A1B2C3`
4. `port-forwarder stats`
5. `port-forwarder save <path>`

## End-to-End Example (Copy-paste ready)

```powershell
# Initialize and load
.\\build\osfx_cli_cl.exe plugin-load port_forwarder

# Add rule: udp:8080 -> tcp:9000
.\\build\osfx_cli_cl.exe port-forwarder add-rule r1 udp 8080 tcp 9000

# View rules
.\\build\osfx_cli_cl.exe port-forwarder list

# Trigger forwarding (hex payload)
.\\build\osfx_cli_cl.exe port-forwarder forward udp 8080 A1B2C3D4

# View statistics
.\\build\osfx_cli_cl.exe port-forwarder stats
```

## Wildcard Port Example

```powershell
# Forward UDP data from any source port to tcp:9100
.\\build\osfx_cli_cl.exe port-forwarder add-rule any_udp udp * tcp 9100
.\\build\osfx_cli_cl.exe port-forwarder forward udp 5001 AABB
```

## Persistence Example

```powershell
# Save rules
.\\build\osfx_cli_cl.exe port-forwarder save E:\\OSynaptic-FX\\build\\pf_rules.txt

# Reload rules
.\\build\osfx_cli_cl.exe port-forwarder load E:\\OSynaptic-FX\\build\\pf_rules.txt
```

## Common Failures and Remedies

- Symptom: `error=usage add-rule ...`
- Cause: Insufficient parameters or incorrect order.
- Remedy: Retry with `add-rule <name> <from_proto> <from_port|*> <to_proto> <to_port>`.

- Symptom: `error=no_match_or_emit_failed`
- Cause: No matching rule, or emit callback send failed.
- Remedy: First `list` to verify rules, then check if `from_proto/from_port` matches input parameters.


