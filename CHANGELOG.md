# Changelog

## 2026-09-12

- Added ESP32 remote-dashboard WebSocket heartbeats: the gateway sends a protocol ping every 15 seconds, treats two missed five-second pong windows as a broken connection, and uses the existing reconnect loop to recover when no browser is polling.
- Installed the heartbeat-enabled firmware over authenticated Wi-Fi OTA and verified that the local API and remote relay returned valid eight-cell telemetry after reboot. A ten-minute test with all dashboards closed passed without restarting the gateway.
- Added remote-dashboard time-until-empty and time-until-full estimates. The page uses reported remaining or missing amp-hours and a page-local, time-weighted rolling two-minute current average; it warms up for 15 seconds, handles idle/stale/duplicate data, resets on direction changes and reconnects, and explains that charging can taper near full.
- Added nine deterministic estimator tests covering discharge, charging, fluctuating loads, idle behavior, stale data, duplicate frames, direction changes, device restarts, and display rounding. Verified desktop and 390 px layouts, no horizontal overflow, Worker dry-run, and live eight-cell browser rendering.

## 2026-09-07

- Replaced the Cloudflare remote-dashboard scaffold with a responsive dark neon/electric-purple LiTime battery console. It uses the same-origin `/browser` WebSocket, requests live status, renders the complete existing battery snapshot, and handles relay reconnects plus disconnected/invalid data states without exposing device credentials.
- Re-request remote status every three seconds while the browser relay is open, preventing an ESP32 reconnect immediately after page load from leaving the dashboard stuck awaiting its one initial reply.
- Normalize relay WebSocket payloads to text before forwarding because the ESP32 remote client deliberately processes text JSON rather than binary messages.
- Routed non-relay requests through the Worker static-asset binding so the dashboard shell, JavaScript, and CSS are served alongside the authenticated `/device` and public `/browser` WebSocket routes.
- Replaced the machine-local classic `BMS_Client`/Bluedroid dependency with a vendored NimBLE-Arduino 2.5.1 LiTime scanner and client, preserving the read-only provider and battery JSON behavior.
- Restored the BLE scan, connection, update, and telemetry loop after the TLS diagnostic hold.
- Kept the remote-dashboard WebSocket handshake fix, including a persistent correctly formatted bearer header and no default `arduino` subprotocol; heap polling is now development-only.
- Corrected LittleFS build/flash geometry to match the committed 4 MB partition table.
