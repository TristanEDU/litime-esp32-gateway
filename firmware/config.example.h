#pragma once

// Copy this file to config.h. The build loads config.h when present; never
// commit it. Remote sync is disabled until all values below are supplied.

#define GATEWAY_ENABLE_REMOTE_SYNC 0
#define GATEWAY_REMOTE_ENDPOINT "https://example.invalid/api/v1/ingest"
#define GATEWAY_REMOTE_TOKEN "replace-with-a-device-scoped-token"

// PEM root certificate for the HTTPS endpoint. Keep the quoted multi-line
// value in config.h (or use a raw C++ string literal) and leave it empty when
// remote sync is disabled.
#define GATEWAY_REMOTE_CA_CERT ""
