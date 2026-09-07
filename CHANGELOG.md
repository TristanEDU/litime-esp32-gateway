# Changelog

## 2026-09-07

- Replaced the machine-local classic `BMS_Client`/Bluedroid dependency with a vendored NimBLE-Arduino 2.5.1 LiTime scanner and client, preserving the read-only provider and battery JSON behavior.
- Restored the BLE scan, connection, update, and telemetry loop after the TLS diagnostic hold.
- Kept the remote-dashboard WebSocket handshake fix, including a persistent correctly formatted bearer header and no default `arduino` subprotocol; heap polling is now development-only.
- Corrected LittleFS build/flash geometry to match the committed 4 MB partition table.
