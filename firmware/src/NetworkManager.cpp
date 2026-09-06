#include "NetworkManager.h"

#include <esp_system.h>

#include "GatewayLog.h"

namespace {
String jsonEscape(const String& value) {
  String escaped;
  escaped.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value[i];
    if (c == '\\' || c == '"') {
      escaped += '\\';
    }
    escaped += c;
  }
  return escaped;
}
}

bool GatewayNetwork::begin() {
  preferences.begin("gateway", false);
  savedSsid = preferences.getString("wifi_ssid", "");
  setupKey = preferences.getString("setup_key", "");
  if (setupKey.length() < 10) {
    setupKey = createSetupKey();
    preferences.putString("setup_key", setupKey);
  }

  const uint64_t chipId = ESP.getEfuseMac();
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%06llX",
           static_cast<unsigned long long>(chipId & 0xFFFFFF));
  apName = String("LiTime-Setup-") + suffix;

  WiFi.mode(WIFI_AP_STA);
  const bool apStarted = WiFi.softAP(apName.c_str(), setupKey.c_str());
  if (!apStarted) {
    return false;
  }
  connectSavedNetwork();
  return true;
}

void GatewayNetwork::loop() {
  if (savedSsid.isEmpty() || stationConnected()) {
    return;
  }
  if (millis() - lastReconnectAt >= 30000UL) {
    connectSavedNetwork();
  }
}

bool GatewayNetwork::startScan() {
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
    return true;
  }
  WiFi.scanDelete();
  const int result = WiFi.scanNetworks(true, true);
  return result == WIFI_SCAN_RUNNING;
}

String GatewayNetwork::scanJson() const {
  const int count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    return "{\"state\":\"scanning\",\"networks\":[]}";
  }
  if (count < 0) {
    return "{\"state\":\"idle\",\"networks\":[]}";
  }

  String json = "{\"state\":\"complete\",\"networks\":[";
  for (int i = 0; i < count; ++i) {
    if (i > 0) json += ',';
    json += "{\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",\"rssi\":";
    json += String(WiFi.RSSI(i));
    json += ",\"secure\":";
    json += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false" : "true";
    json += '}';
  }
  json += "]}";
  return json;
}

bool GatewayNetwork::saveWiFi(const String& ssid, const String& password, String& error) {
  if (ssid.isEmpty() || ssid.length() > 32 || password.length() > 63) {
    error = "SSID or password length is invalid.";
    return false;
  }
  preferences.putString("wifi_ssid", ssid);
  preferences.putString("wifi_pass", password);
  savedSsid = ssid;
  connectSavedNetwork();
  return true;
}

void GatewayNetwork::forgetWiFi() {
  preferences.remove("wifi_ssid");
  preferences.remove("wifi_pass");
  savedSsid = "";
  WiFi.disconnect(false, false);
}

bool GatewayNetwork::stationConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

const String& GatewayNetwork::stationSsid() const { return savedSsid; }
const String& GatewayNetwork::apSsid() const { return apName; }
const String& GatewayNetwork::setupPassword() const { return setupKey; }

String GatewayNetwork::statusJson() const {
  String json = "{\"ap\":{\"ssid\":\"" + jsonEscape(apName) + "\",\"ip\":\"";
  json += WiFi.softAPIP().toString();
  json += "\"},\"station\":{\"connected\":";
  json += stationConnected() ? "true" : "false";
  json += ",\"ssid\":\"" + jsonEscape(savedSsid) + "\",\"ip\":\"";
  json += stationConnected() ? WiFi.localIP().toString() : "";
  json += "\"}}";
  return json;
}

String GatewayNetwork::createSetupKey() {
  static const char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
  String key;
  key.reserve(12);
  for (uint8_t i = 0; i < 12; ++i) {
    key += alphabet[esp_random() % (sizeof(alphabet) - 1)];
  }
  return key;
}

void GatewayNetwork::connectSavedNetwork() {
  lastReconnectAt = millis();
  if (savedSsid.isEmpty()) return;
  const String password = preferences.getString("wifi_pass", "");
  GATEWAY_LOG("Connecting to saved Wi-Fi network %s\\n", savedSsid.c_str());
  WiFi.begin(savedSsid.c_str(), password.c_str());
}
