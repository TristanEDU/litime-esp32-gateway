#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

class GatewayNetwork {
 public:
  bool begin();
  void loop();

  bool startScan();
  String scanJson() const;
  bool saveWiFi(const String& ssid, const String& password, String& error);
  void forgetWiFi();

  bool stationConnected() const;
  const String& stationSsid() const;
  const String& apSsid() const;
  const String& setupPassword() const;
  String statusJson() const;

 private:
  Preferences preferences;
  String apName;
  String setupKey;
  String savedSsid;
  uint32_t lastReconnectAt = 0;
  String createSetupKey();
  void connectSavedNetwork();
};
