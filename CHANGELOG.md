# Changelog

## 2026-09-07

- Replaced the Cloudflare remote-dashboard scaffold with a responsive dark neon/electric-purple LiTime battery console. It uses the same-origin `/browser` WebSocket, requests live status, renders the complete existing battery snapshot, and handles relay reconnects plus disconnected/invalid data states without exposing device credentials.
- Re-request remote status every three seconds while the browser relay is open, preventing an ESP32 reconnect immediately after page load from leaving the dashboard stuck awaiting its one initial reply.
- Normalize relay WebSocket payloads to text before forwarding because the ESP32 remote client deliberately processes text JSON rather than binary messages.
- Routed non-relay requests through the Worker static-asset binding so the dashboard shell, JavaScript, and CSS are served alongside the authenticated `/device` and public `/browser` WebSocket routes.
- Replaced the machine-local classic `BMS_Client`/Bluedroid dependency with a vendored NimBLE-Arduino 2.5.1 LiTime scanner and client, preserving the read-only provider and battery JSON behavior.
- Restored the BLE scan, connection, update, and telemetry loop after the TLS diagnostic hold.
- Kept the remote-dashboard WebSocket handshake fix, including a persistent correctly formatted bearer header and no default `arduino` subprotocol; heap polling is now development-only.
- Corrected LittleFS build/flash geometry to match the committed 4 MB partition table.
