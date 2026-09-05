# LiTime ESP32 Gateway

Turn an inexpensive ESP32 into a local and remote monitoring gateway for Bluetooth-enabled LiTime batteries.

The project is designed to provide:

- Direct BLE communication with compatible LiTime battery management systems
- Pack voltage, current, power, and state of charge
- Remaining and full battery capacity
- Individual cell voltages
- Cell imbalance monitoring
- Battery and MOSFET temperature monitoring
- A local web dashboard hosted directly by the ESP32
- A JSON API for local integrations
- Optional remote telemetry over the internet
- A fallback ESP32 Wi-Fi access point for local access
- Automatic BLE and Wi-Fi reconnect behavior
- Mock/simulation mode for development without a battery attached
- An optional self-hosted remote dashboard/backend

## Project Goals

This repository should be usable by someone who has:

1. A compatible Bluetooth-enabled LiTime battery
2. An ESP32
3. A computer capable of flashing the ESP32

The project should not require users to understand or modify internal firmware code for normal setup.

Long term, configuration should be possible through a browser-based setup flow.

## Architecture

```text
LiTime Battery
      |
      | BLE
      v
    ESP32
      |
      +---- Local Web Dashboard
      |
      +---- Local JSON API
      |
      +---- Wi-Fi / Internet
                    |
                    v
          Optional Remote Server
                    |
                    v
           Internet Dashboard
```

## Current Development Hardware

The initial reference hardware is:

- ESP32 NodeMCU-32S
- Classic ESP32 dual-core module
- LiTime 24V 100Ah Bluetooth battery

The reference battery has been successfully discovered over BLE and live telemetry has been read from its BMS.

## Planned Development

Major upcoming work includes:

- Clean firmware architecture
- Battery provider abstraction
- Mock battery provider
- LiTime BLE provider
- Wi-Fi station mode
- Fallback setup access point
- Local web dashboard
- JSON telemetry API
- Persistent configuration
- Browser-based setup
- Remote telemetry client
- Optional Docker-based remote server
- Authentication and device tokens
- OTA firmware updates
- Automated firmware builds with GitHub Actions
- Documentation and troubleshooting guides

## Repository Structure

```text
litime-esp32-gateway/
├── firmware/
│   ├── config.example.h
│   └── src/
├── server/
├── docs/
├── scripts/
├── .github/
│   └── workflows/
├── README.md
├── CHANGELOG.md
├── LICENSE
└── .gitignore
```

## Security

Local credentials and secrets must never be committed.

The following are intentionally ignored by Git:

- Wi-Fi credentials
- Device-specific configuration
- API tokens
- Server environment files

Use the provided example configuration files as templates.

## License

License information will be added before the first public release.
