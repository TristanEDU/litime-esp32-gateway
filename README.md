# LiTime ESP32 Gateway

An open-source, read-only gateway for Bluetooth-enabled LiTime batteries. It keeps a browser dashboard available through its own Wi-Fi access point, can join a selected Wi-Fi network, buffers short-term telemetry locally, and can optionally push queued telemetry outward to a user-owned HTTPS endpoint. It never requires inbound port forwarding.

## This phase

- Preserves LiTime BLE discovery, live telemetry, and `/api/battery` JSON.
- Serves a real LittleFS web UI from `firmware/data/` (`index.html`, `style.css`, `app.js`).
- Uses a 4 MB custom table: NVS, OTA metadata, two 1.875 MiB OTA slots, 128 KiB LittleFS, and coredump space.
- Has release/development profiles; release compiles verbose telemetry/debug serial strings out.
- Provides a unique WPA2 setup AP, password-protected network scan/save, AP+station reconnect, 60-second history, bounded events, and a bounded remote queue.
- Keeps Arduino OTA enabled when a station network is connected.

## Build and flash

Install Arduino CLI, `esp32:esp32`, and the original **BMS Client** library. Then:

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

## Firmware-space profile

With ESP32 Arduino core 3.3.11 and BMS Client 1.0.0, the original prototype used **1,682,363 bytes** in a 1.94 MiB OTA slot. The release gateway build uses **1,805,855 bytes** in its **1,966,080-byte** slot, leaving about **160 KiB**. BLE/BMS support is the major practical consumer; Wi-Fi/AP, LittleFS, web server, OTA, and buffering form the next tier. TLS sync is opt-in to retain growth margin: the tested enabled build is 1,927,167 bytes (about 39 KiB remaining), so future remote features require a size check.

## Safety

Monitoring only: this firmware exposes no LiTime charge/discharge control and must not be used as a safety system or automatic charging disconnect.
