#include "RemoteDashboard.h"

#include "GatewayConfig.h"

#if GATEWAY_ENABLE_REMOTE_DASHBOARD
#include <WebSocketsClient.h>

namespace {
WebSocketsClient webSocket;
bool configured = false;
RemoteDashboard::StatusProvider statusProvider = nullptr;

void handleWebSocketEvent(
    WStype_t type,
    uint8_t* payload,
    size_t length
) {
  if (type != WStype_TEXT || payload == nullptr || length == 0) return;

  const String message(
      reinterpret_cast<char*>(payload),
      length
  );

  if (message == "{\"type\":\"get_status\"}") {
    if (statusProvider == nullptr) return;

    String response = "{\"type\":\"status\",\"battery\":";
    response += statusProvider();
    response += '}';

    webSocket.sendTXT(response);
  }
}
}
#endif

void RemoteDashboard::begin(StatusProvider newStatusProvider) {
#if GATEWAY_ENABLE_REMOTE_DASHBOARD
  statusProvider = newStatusProvider;

  if (String(GATEWAY_REMOTE_DASHBOARD_HOST).isEmpty()) return;
  if (String(GATEWAY_REMOTE_DASHBOARD_TOKEN).isEmpty()) return;

  webSocket.beginSSL(
      GATEWAY_REMOTE_DASHBOARD_HOST,
      443,
      GATEWAY_REMOTE_DASHBOARD_PATH
  );

  webSocket.setAuthorization(
      "Bearer",
      GATEWAY_REMOTE_DASHBOARD_TOKEN
  );

  webSocket.onEvent(handleWebSocketEvent);
  webSocket.setReconnectInterval(5000);

  configured = true;
#endif
}

void RemoteDashboard::loop() {
#if GATEWAY_ENABLE_REMOTE_DASHBOARD
  if (!configured) return;
  webSocket.loop();
#endif
}
