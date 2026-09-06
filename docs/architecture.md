# Gateway architecture

```text
LiTime BMS --BLE--> LiTimeBLEProvider --> live API / local UI
                                         |             |
                                         v             v
                                   StorageManager   GatewayNetwork
                                   60 s samples     AP + station Wi-Fi
                                   events           setup scan/save
                                   remote queue          |
                                                         v
                                             optional HTTPS POST sync
```

The existing BLE scanner/provider retains discovery and telemetry. The cooperative loop handles HTTP, Wi-Fi reconnect, OTA, BLE updates, bounded storage, and optional sync without the old three-second diagnostic delay.

`firmware/partitions.csv` reserves NVS, `otadata`, equal OTA app slots, LittleFS, and coredump. Two OTA slots are mandatory. The 128 KiB filesystem is deliberately small: it holds a compact UI plus short-term aggregate history, not raw BLE traffic.

Wi-Fi SSID/password and the generated setup password use `Preferences`/NVS. The setup page uses WPA2 on the AP plus HTTP Basic auth for configuration writes. For physical-device threat protection, enable ESP32 flash encryption and secure boot during production provisioning.

Storage uses one compact sample per minute, bounded event records, and a separate acknowledged NDJSON remote queue. If capacity fills, the oldest bounded file rotates; live monitoring is never blocked by logging.
