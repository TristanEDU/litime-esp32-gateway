# LiTime ESP32 Gateway

An open-source, read-only gateway for Bluetooth-enabled LiTime batteries. It keeps a browser dashboard available through its own Wi-Fi access point, can join a selected Wi-Fi network, buffers short-term telemetry locally, and can optionally push queued telemetry outward to a user-owned HTTPS endpoint. It never requires inbound port forwarding.

## This phase

- Preserves LiTime BLE discovery, live telemetry, and `/api/battery` JSON through a NimBLE central/observer transport.
- Serves a real LittleFS web UI from `firmware/data/` (`index.html`, `style.css`, `app.js`).
- Uses a 4 MB custom table: NVS, OTA metadata, two 1.9375 MiB OTA slots, and 64 KiB LittleFS.
- Has release/development profiles; release compiles verbose telemetry/debug serial strings out.
- Provides a unique WPA2 setup AP, password-protected network scan/save, AP+station reconnect, 60-second history, bounded events, and a bounded remote queue.
- Keeps Arduino OTA enabled when a station network is connected.
- Includes an optional Cloudflare-hosted, read-only remote dashboard at a user-owned custom domain. Its browser uses the same-origin WebSocket relay and shows the live firmware battery snapshot without exposing device credentials.

## Build and flash

Install Arduino CLI and `esp32:esp32`. The required NimBLE-Arduino 2.5.1 source is vendored under `firmware/libraries/`, so no machine-local BLE library is required. Then:

```sh
./scripts/build.sh release
./scripts/flash.sh /dev/cu.usbserial-0001 release
```

`flash.sh` writes both the application/partition image and the LittleFS UI image. A partition-layout change reformats the prior filesystem, so export logs first. First boot prints a generated setup SSID, password, and `192.168.4.1` address. Join that WPA2 AP and open the address. Use `./scripts/build.sh development` for bench diagnostics; normal operation should use release.

## Wi-Fi and security

The gateway always retains a per-device setup AP. The setup page accepts the printed password, scans networks, saves a selected network, and operates AP+station so direct access continues during an internet outage. Read-only APIs remain local-network accessible; setup-changing endpoints require HTTP Basic authentication with the generated setup password. Do not expose the station IP to the public internet.

Wi-Fi credentials and the setup password are saved in ESP32 NVS, not in source control. NVS is reasonable embedded persistence but is not protection against physical flash extraction; production devices needing that protection should enable secure boot and flash encryption.

## Local storage and remote access

LittleFS is a buffer, not a database. Valid BLE data is sampled at most once per minute. History, events, and unsent remote data have independent strict caps so a full queue cannot consume the UI filesystem. A valid remote batch is removed only after a 2xx response; intermittent connectivity does not stop BLE monitoring or local history.

Remote sync is compiled off by default. Copy `firmware/config.example.h` to ignored `firmware/config.h`, supply a device-scoped HTTPS endpoint/token and PEM root CA, and set `GATEWAY_ENABLE_REMOTE_SYNC` to `1`. The device posts small JSON arrays every five minutes while online. See [remote access design](docs/remote-access.md).

The separately optional remote dashboard uses `cloudflare/remote-dashboard/` and the ignored dashboard values in `firmware/config.h`. It relays only live status requests and replies: the browser sends `{"type":"get_status"}` to `/browser`, and the authenticated ESP32 returns `{"type":"status","battery":...}` through `/device`. The static page has no device token and is intentionally read-only. Deploy it with `npm ci && npm run deploy` from that directory after setting the Cloudflare Worker `DEVICE_TOKEN` secret.

## BLE and firmware-space profile

The gateway uses the NimBLE-Arduino 2.5.1 transport (Apache-2.0, vendored at `firmware/libraries/NimBLE-Arduino`) instead of the classic ESP32 `BMS_Client`/Bluedroid stack. Scanner and client share the same NimBLE host and preserve the LiTime `FFE0`/`FFE1`/`FFE2` read path and battery JSON fields. The stack is not deinitialized on a disconnect, allowing later scans and reconnects without colliding with the outbound TLS/WebSocket client.

With ESP32 Arduino core 3.3.11, a configured remote-dashboard WebSocket build uses **1,501,853 bytes** for release and **1,508,105 bytes** for development in the **2,031,616-byte** OTA slot, leaving roughly **530 KiB** of code-space margin. The NimBLE migration also leaves **263,956 bytes** of reported dynamic-memory headroom at link time for the release build. The 64 KiB LittleFS image is built and flashed at `0x3F0000`; it is intentionally compact, so treat it as a bounded UI/history buffer rather than durable storage. Hardware validation is still required to establish run-time heap margin with a real LiTime BMS and TLS WebSocket connected.

## Safety

Monitoring only: this firmware exposes no LiTime charge/discharge control and must not be used as a safety system or automatic charging disconnect.
