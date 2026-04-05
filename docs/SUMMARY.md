# Documentation Summary

Primary consumer path is Arduino library usage via `library.properties` + `examples/`; native script and CLI content is maintainer-oriented.

## 1. Core

- `01-overview.md`: Project positioning, phase status, capability boundaries.
- `02-architecture.md`: Module layering, data flow, runtime relationships.
- `03-api-index.md`: Core header files and main API navigation.

## 2. Platform Scope

- `04-plugin-scope-and-commands.md`: Plugin inclusion/exclusion strategy, command matrix.
- `05-port-forwarder.md`: Rules model, commands, persistence, semantic statistics.
- `06-cli.md`: Lightweight CLI commands, usage, and output conventions.

## 3. Delivery

- `07-quality-gate-and-compiler-matrix.md`: Build/test/matrix execution and reporting.
- `08-release-notes.md`: Release notes (current version).
- `09-mirror-coverage-report.md`: Mirror coverage conclusions with OpenSynaptic.
- `10-acceptance-checklist.md`: Pre-release acceptance checklist.
- `11-benchmark-method.md`: Release-level performance quantification method and calibration.
- `12-config-quick-reference.md`: Script and parameter configuration quick reference.
- `13-performance-summary.md`: Release performance results summary and threshold recommendations.
- `14-troubleshooting.md`: Common failures and remediation paths.
- `15-release-playbook.md`: Release execution manual.
- `16-examples-cookbook.md`: Detailed end-to-end examples and failure branches.
- `17-glue-step-by-step.md`: Glue layer step-by-step code explanation.
- `18-standardized-units.md`: User input units to standardized output mapping table.
- `19-input-specification.md`: Input specification (send rules and field boundaries).
- `20-arduino-easy-api.md`: Simplified Arduino-facing API and usage path.

## 4. Change Management

- `CHANGELOG.md`: Version changelog.
- `templates/changelog-template.md`: Changelog template.
