# Remote access design

The ESP32 makes the outbound connection. Friend, café, and normal home Wi-Fi permit HTTPS egress but not safe inbound access, so no port forwarding, UPnP, or public ESP32 IP is required.

Create an HTTPS `POST /api/v1/ingest` endpoint that accepts a device-scoped bearer token and a JSON array such as:

```json
[{"uptime_ms":123000,"voltage":26.42,"current":-4.8,"soc":82,"power":-126.8,"cell_temp":22.1,"mosfet_temp":24.0,"cell_delta_mv":8.0}]
```

Authenticate and rate-limit the token, validate/persist the complete batch, and return 2xx only after durable acceptance. A non-2xx leaves the local batch queued. The service should expose an authenticated user dashboard/read API; it must not proxy anonymous traffic to the ESP32.

Copy `firmware/config.example.h` to `firmware/config.h`, use a revocable token, HTTPS endpoint, and PEM root CA, then enable remote sync. The firmware does not accept invalid certificates or use an insecure TLS bypass. A later MQTT transport should reuse the same queue-and-acknowledgement contract.

The optional remote-dashboard relay uses the same ignored configuration file. When enabled, its WebSocket handshake sends `Authorization: Bearer <device-token>` as an HTTP header and intentionally supplies no `arduino` subprotocol. It shares the ESP32 with the NimBLE LiTime client; use a release build for normal operation and complete a physical BLE-plus-TLS soak test before relying on remote monitoring.

## Remote browser dashboard

`cloudflare/remote-dashboard/` hosts the read-only dashboard and the relay. The `/device` WebSocket accepts only the configured bearer token; `/browser` is intentionally token-free so the page can establish its same-origin `wss://<dashboard-host>/browser` connection without embedding a device credential in client JavaScript. Once connected, the browser sends `{"type":"get_status"}`. The ESP32 recognizes that exact request and returns `{"type":"status","battery":...}` using the existing firmware battery serializer.

The dashboard renders the complete live snapshot: connection/validity state, voltage, current, state of charge, power, remaining/capacity Ah, cell and MOSFET temperatures, count, minimum/maximum/delta, and the reported per-cell voltages. It re-requests the read-only status every three seconds while its relay socket is open, reconnects with a capped backoff, and makes relay loss, device disconnection, and a connected-but-not-valid reading visibly distinct. It retains no history and exposes no BMS control.

Deploy from `cloudflare/remote-dashboard/` after creating the Worker `DEVICE_TOKEN` secret outside source control:

```sh
npm ci
npx wrangler secret put DEVICE_TOKEN
npm run deploy
```
