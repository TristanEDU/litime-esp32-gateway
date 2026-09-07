#pragma once

#include <cstdint>

// config.h is intentionally ignored by Git. Keeping this include here rather
// than only in the sketch makes the same build options visible to every .cpp
// translation unit (especially the optional remote transport).
#if __has_include("../config.h")
#include "../config.h"
#endif

// A release build omits verbose serial strings and diagnostics.  The build
// scripts set this to 1 for the development profile.
#ifndef GATEWAY_DEBUG
#define GATEWAY_DEBUG 0
#endif

#ifndef GATEWAY_ENABLE_ARDUINO_OTA
#define GATEWAY_ENABLE_ARDUINO_OTA 1
#endif

// Remote sync is opt-in because it adds TLS code and requires a user-owned
// HTTPS endpoint and CA certificate. Copy config.example.h to config.h to set
// these values; config.h is ignored by Git.
#ifndef GATEWAY_ENABLE_REMOTE_SYNC
#define GATEWAY_ENABLE_REMOTE_SYNC 0
#endif

#ifndef GATEWAY_ENABLE_REMOTE_DASHBOARD
#define GATEWAY_ENABLE_REMOTE_DASHBOARD 0
#endif

#ifndef GATEWAY_REMOTE_DASHBOARD_HOST
#define GATEWAY_REMOTE_DASHBOARD_HOST ""
#endif

#ifndef GATEWAY_REMOTE_DASHBOARD_PATH
#define GATEWAY_REMOTE_DASHBOARD_PATH "/device"
#endif

#ifndef GATEWAY_REMOTE_DASHBOARD_TOKEN
#define GATEWAY_REMOTE_DASHBOARD_TOKEN ""
#endif

#ifndef GATEWAY_REMOTE_ENDPOINT
#define GATEWAY_REMOTE_ENDPOINT ""
#endif

#ifndef GATEWAY_REMOTE_TOKEN
#define GATEWAY_REMOTE_TOKEN ""
#endif

#ifndef GATEWAY_REMOTE_CA_CERT
#define GATEWAY_REMOTE_CA_CERT ""
#endif

static constexpr uint32_t TELEMETRY_SAMPLE_INTERVAL_MS = 60000UL;
static constexpr uint32_t BATTERY_RESCAN_INTERVAL_MS = 60000UL;
static constexpr uint32_t REMOTE_SYNC_INTERVAL_MS = 300000UL;
