# Changelog

All notable changes to `OSynaptic-FX` are documented in this file.

## [v1.0.0-p5] - 2026-04-04

### Added

- Scoped plugin modules:
  - `transport` (lite)
  - `test_plugin` (lite)
  - `port_forwarder` (full)
- Platform runtime adapter and CLI lite routing.
- Standalone CLI entry: `tools/osfx_cli_main.c`.
- Compiler matrix quality gate mode (`test.ps1 -Matrix`).
- Quality report artifact: `build/quality_gate_report.md`.
- Full documentation library under `docs/`.

### Changed

- Service runtime control-plane API extended for list/load/cmd patterns.
- README updated with scoped plugin policy and quality-gate usage.

### Removed / Excluded

- Out-of-scope plugin surfaces for this release:
  - `web`
  - `sql`
  - `dependency_manager`
  - `env_guard`


