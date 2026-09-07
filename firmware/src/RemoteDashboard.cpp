#include "RemoteDashboard.h"

#include "GatewayConfig.h"
#include "GatewayLog.h"

#if GATEWAY_ENABLE_REMOTE_DASHBOARD
#include <WebSocketsClient.h>

namespace {
WebSocketsClient webSocket;
bool configured = false;
String authorizationHeader;
RemoteDashboard::StatusProvider statusProvider = nullptr;

void handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    GATEWAY_LOGLN("Remote dashboard WebSocket connected.");
    return;
  }

  if (type == WStype_DISCONNECTED) {
    GATEWAY_LOG(
        "Remote dashboard disconnected: length=%u payload=%.*s\n",
        static_cast<unsigned>(length),
        static_cast<int>(length),
        payload != nullptr ? reinterpret_cast<char*>(payload) : ""
    );
    return;
  }
  if (type == WStype_ERROR) {
    GATEWAY_LOG(
        "Remote dashboard error: %.*s\n",
        static_cast<int>(length),
        payload != nullptr ? reinterpret_cast<char*>(payload) : ""
    );
    return;
  }
  if (type != WStype_TEXT || payload == nullptr || length == 0) return;

  const String message(reinterpret_cast<char*>(payload), length);
  if (message != "{\"type\":\"get_status\"}" || statusProvider == nullptr) return;

  String response = "{\"type\":\"status\",\"battery\":";
  response += statusProvider();
  response += '}';
  webSocket.sendTXT(response);
}
}  // namespace
#endif

void RemoteDashboard::begin(StatusProvider newStatusProvider) {
#if GATEWAY_ENABLE_REMOTE_DASHBOARD
  statusProvider = newStatusProvider;

  if (String(GATEWAY_REMOTE_DASHBOARD_HOST).isEmpty()) return;
  if (String(GATEWAY_REMOTE_DASHBOARD_TOKEN).isEmpty()) return;

  // Pass an empty subprotocol so the relay does not receive the library's
  // default "arduino" value.
  webSocket.beginSSL(
      GATEWAY_REMOTE_DASHBOARD_HOST,
      443,
      GATEWAY_REMOTE_DASHBOARD_PATH,
      nullptr,
      ""
  );

  // WebSocketsClient stores this pointer, so the String must outlive begin().
  authorizationHeader = String("Authorization: Bearer ") + GATEWAY_REMOTE_DASHBOARD_TOKEN;
  webSocket.setExtraHeaders(authorizationHeader.c_str());
  webSocket.onEvent(handleWebSocketEvent);
  webSocket.setReconnectInterval(5000);

  configured = true;
#endif
}

void RemoteDashboard::loop() {
#if GATEWAY_ENABLE_REMOTE_DASHBOARD
  if (!configured) return;
#if GATEWAY_DEBUG
  static uint32_t lastHeapLog = 0;
  if (millis() - lastHeapLog >= 5000) {
    lastHeapLog = millis();
    GATEWAY_LOG("Remote dashboard heap: free=%u\n", ESP.getFreeHeap());
  }
#endif
  webSocket.loop();
#endif
}
