# Gateway architecture

```text
LiTime BMS --BLE--> NimBLE scanner/client --> LiTimeBLEProvider --> live API / local UI
                                         |             |
                                         v             v
                                   StorageManager   GatewayNetwork
                                   60 s samples     AP + station Wi-Fi
                                   events           setup scan/save
                                   remote queue          |
                                                         v
                                             optional HTTPS POST sync
```

The BLE scanner and LiTime client share one NimBLE central/observer host. The client preserves the prior `FFE0` service, `FFE2` query, `FFE1` notification flow and frame parsing, but avoids the higher-memory classic ESP32 BLE stack. A disconnect clears the client connection only; it does not deinitialize the shared BLE host, so later scans/reconnects can coexist with TLS/WebSocket activity. The cooperative loop handles HTTP, Wi-Fi reconnect, OTA, BLE updates, bounded storage, and optional sync without the old three-second diagnostic delay.

`firmware/partitions.csv` reserves NVS, `otadata`, two equal 0x1F0000 OTA app slots, and a 0x10000 LittleFS partition at `0x3F0000`. Two OTA slots are mandatory. The 64 KiB filesystem is deliberately small: it holds a compact UI plus short-term aggregate history, not raw BLE traffic.

Wi-Fi SSID/password and the generated setup password use `Preferences`/NVS. The setup page uses WPA2 on the AP plus HTTP Basic auth for configuration writes. For physical-device threat protection, enable ESP32 flash encryption and secure boot during production provisioning.

Storage uses one compact sample per minute, bounded event records, and a separate acknowledged NDJSON remote queue. If capacity fills, the oldest bounded file rotates; live monitoring is never blocked by logging.
