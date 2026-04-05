# OSynaptic-FX Arduino Examples

This folder contains practical Arduino use cases for OSynaptic-FX.

## Example List

- `EasyQuickStart`: recommended first sketch using simplified `osfx_easy` API — **start here**
- `BasicEncode`: single-sensor FULL packet encode baseline
- `PacketMetaDecode`: decode packet metadata and recover sensor payload
- `FusionAutoMode`: auto FULL/DIFF/HEART fusion-mode encode/decode
- `FusionModeTest`: deterministic FULL → DIFF → HEART transition validation
- `SecureSessionRoundtrip`: secure-session key setup and encrypted packet roundtrip
- `MultiSensorNodePacket`: aggregate and decode multi-sensor node packet
- `BootCliOrRun`: boot 10 s serial-triggered CLI mode, otherwise persistent run mode
- `ESP32WiFiMultiSensorAuto`: full Wi-Fi UDP node with LittleFS ID persistence and 4-sensor auto DIFF
- `QuickBench`: ESP32-focused dual-core packet encode throughput benchmark

## Quick Compile (Arduino CLI)

```powershell
arduino-cli compile --fqbn arduino:avr:uno    .\examples\EasyQuickStart
arduino-cli compile --fqbn arduino:avr:uno    .\examples\BasicEncode
arduino-cli compile --fqbn arduino:avr:uno    .\examples\PacketMetaDecode
arduino-cli compile --fqbn arduino:avr:uno    .\examples\FusionAutoMode
arduino-cli compile --fqbn esp32:esp32:esp32  .\examples\FusionModeTest
arduino-cli compile --fqbn arduino:avr:uno    .\examples\SecureSessionRoundtrip
arduino-cli compile --fqbn arduino:avr:uno    .\examples\MultiSensorNodePacket
arduino-cli compile --fqbn esp32:esp32:esp32  .\examples\BootCliOrRun
arduino-cli compile --fqbn esp32:esp32:esp32  .\examples\ESP32WiFiMultiSensorAuto
arduino-cli compile --fqbn esp32:esp32:esp32  .\examples\QuickBench
```
