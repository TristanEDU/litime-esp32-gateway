#pragma once

#include <Arduino.h>
#include "GatewayConfig.h"

// Do not pass a preformatted String to these macros: the entire expression is
// compiled out of release builds along with its format strings.
#if GATEWAY_DEBUG
#define GATEWAY_LOG(...) Serial.printf(__VA_ARGS__)
#define GATEWAY_LOGLN(message) Serial.println(message)
#else
#define GATEWAY_LOG(...) do { } while (0)
#define GATEWAY_LOGLN(message) do { } while (0)
#endif

// Small boot messages remain in production because they are the recovery path
// for the per-device setup password. No telemetry is emitted in release mode.
#define GATEWAY_BOOT(message) Serial.println(message)
