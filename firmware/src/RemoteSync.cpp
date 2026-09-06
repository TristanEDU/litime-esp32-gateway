#include "RemoteSync.h"

#include <WiFi.h>

#include "GatewayConfig.h"
#include "GatewayLog.h"
#include "StorageManager.h"

#if GATEWAY_ENABLE_REMOTE_SYNC
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#endif

void RemoteSync::begin(StorageManager& newStorage) { storage = &newStorage; }

bool RemoteSync::enabled() const {
#if GATEWAY_ENABLE_REMOTE_SYNC
  return String(GATEWAY_REMOTE_ENDPOINT).startsWith("https://") &&
         String(GATEWAY_REMOTE_TOKEN).length() > 0 &&
         String(GATEWAY_REMOTE_CA_CERT).length() > 0;
#else
  return false;
#endif
}

void RemoteSync::loop() {
  if (!enabled() || storage == nullptr || WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastAttemptAt < REMOTE_SYNC_INTERVAL_MS) return;
  lastAttemptAt = millis();
  publishNextBatch();
}

bool RemoteSync::publishNextBatch() {
#if GATEWAY_ENABLE_REMOTE_SYNC
  size_t itemCount = 0;
  const String batch = storage->nextRemoteBatch(8, itemCount);
  if (itemCount == 0) return true;

  WiFiClientSecure client;
  client.setCACert(GATEWAY_REMOTE_CA_CERT);
  HTTPClient http;
  if (!http.begin(client, GATEWAY_REMOTE_ENDPOINT)) return false;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + GATEWAY_REMOTE_TOKEN);
  const int status = http.POST(batch);
  http.end();
  if (status >= 200 && status < 300) {
    return storage->acknowledgeRemoteBatch(itemCount);
  }
  GATEWAY_LOG("Remote sync rejected batch: HTTP %d\\n", status);
  return false;
#else
  return false;
#endif
}
