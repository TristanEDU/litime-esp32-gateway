# Remote access design

The ESP32 makes the outbound connection. Friend, café, and normal home Wi-Fi permit HTTPS egress but not safe inbound access, so no port forwarding, UPnP, or public ESP32 IP is required.

Create an HTTPS `POST /api/v1/ingest` endpoint that accepts a device-scoped bearer token and a JSON array such as:

```json
[{"uptime_ms":123000,"voltage":26.42,"current":-4.8,"soc":82,"power":-126.8,"cell_temp":22.1,"mosfet_temp":24.0,"cell_delta_mv":8.0}]
```

Authenticate and rate-limit the token, validate/persist the complete batch, and return 2xx only after durable acceptance. A non-2xx leaves the local batch queued. The service should expose an authenticated user dashboard/read API; it must not proxy anonymous traffic to the ESP32.

Copy `firmware/config.example.h` to `firmware/config.h`, use a revocable token, HTTPS endpoint, and PEM root CA, then enable remote sync. The firmware does not accept invalid certificates or use an insecure TLS bypass. A later MQTT transport should reuse the same queue-and-acknowledgement contract.
