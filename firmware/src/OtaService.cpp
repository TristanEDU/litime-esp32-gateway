#include "OtaService.h"

#include "GatewayConfig.h"

#if GATEWAY_ENABLE_ARDUINO_OTA
#include <ArduinoOTA.h>
#include <WiFi.h>

#include "GatewayLog.h"
#endif

void OtaService::begin() {
#if GATEWAY_ENABLE_ARDUINO_OTA
  ArduinoOTA.setHostname("litime-gateway");
  ArduinoOTA.onStart([]() { GATEWAY_LOGLN("OTA update started"); });
  ArduinoOTA.onEnd([]() { GATEWAY_LOGLN("OTA update complete"); });
#endif
}

void OtaService::loop() {
#if GATEWAY_ENABLE_ARDUINO_OTA
  if (WiFi.status() != WL_CONNECTED) return;
  if (!started) {
    ArduinoOTA.begin();
    started = true;
  }
  ArduinoOTA.handle();
#endif
}
