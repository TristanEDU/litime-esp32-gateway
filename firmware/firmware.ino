#include <Arduino.h>
#include <LittleFS.h>
#include <WebServer.h>

#include "src/GatewayConfig.h"
#include "src/GatewayLog.h"
#include "src/LiTimeBLEProvider.h"
#include "src/LiTimeScanner.h"
#include "src/NetworkManager.h"
#include "src/OtaService.h"
#include "src/RemoteSync.h"
#include "src/StorageManager.h"

LiTimeBLEProvider battery;
LiTimeScanner scanner;
GatewayNetwork network;
StorageManager storage;
RemoteSync remoteSync;
OtaService ota;
WebServer server(80);

namespace {
bool previousBatteryConnected = false;
uint32_t lastBatteryScanAt = 0;

String jsonString(const String& body, const char* key) {
  const String marker = String('"') + key + "\":";
  const int markerIndex = body.indexOf(marker);
  if (markerIndex < 0) return "";
  int index = markerIndex + marker.length();
  while (index < body.length() && body[index] == ' ') ++index;
  if (index >= body.length() || body[index] != '"') return "";
  ++index;
  String value;
  bool escaped = false;
  for (; index < body.length(); ++index) {
    const char character = body[index];
    if (escaped) {
      switch (character) {
        case 'n': value += '\n'; break;
        case 'r': value += '\r'; break;
        case 't': value += '\t'; break;
        default: value += character; break;
      }
      escaped = false;
    } else if (character == '\\') {
      escaped = true;
    } else if (character == '"') {
      return value;
    } else {
      value += character;
    }
  }
  return "";
}

bool requireSetupAuth() {
  if (server.authenticate("admin", network.setupPassword().c_str())) return true;
  server.requestAuthentication(BASIC_AUTH, "LiTime gateway setup");
  return false;
}

String batteryJson() {
  const BatteryData& data = battery.getData();
  String json;
  json.reserve(512);
  json = "{\"connected\":" + String(data.connected ? "true" : "false");
  json += ",\"valid\":" + String(data.valid ? "true" : "false");
  json += ",\"voltage\":" + String(data.voltage, 2);
  json += ",\"current\":" + String(data.current, 2);
  json += ",\"soc\":" + String(data.soc);
  json += ",\"power\":" + String(data.power, 2);
  json += ",\"remainingAh\":" + String(data.remainingAh, 2);
  json += ",\"capacityAh\":" + String(data.capacityAh, 2);
  json += ",\"cellTemp\":" + String(data.cellTemp, 1);
  json += ",\"mosfetTemp\":" + String(data.mosfetTemp, 1);
  json += ",\"cellCount\":" + String(data.cellCount);
  json += ",\"minCell\":" + String(data.minCell, 3);
  json += ",\"maxCell\":" + String(data.maxCell, 3);
  json += ",\"cellDeltaMv\":" + String(data.cellDeltaMv, 1);
  json += ",\"updatedAt\":" + String(data.updatedAt);
  json += ",\"cells\":[";
  for (size_t i = 0; i < data.cellCount; ++i) {
    if (i > 0) json += ',';
    json += String(data.cells[i], 3);
  }
  return json + "]}";
}

void sendJson(int status, const String& body) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(status, "application/json", body);
}

void attemptBatteryConnection() {
  lastBatteryScanAt = millis();
  std::vector<DiscoveredBattery> batteries = scanner.scan(10);
  GATEWAY_LOG("Found %u compatible battery advertisements\\n", batteries.size());
  if (!batteries.empty()) battery.connect(batteries[0].macAddress.c_str());
}

void setupRoutes() {
  server.on("/api/battery", HTTP_GET, []() { sendJson(200, batteryJson()); });
  server.on("/api/status", HTTP_GET, []() {
    String json = network.statusJson();
    json.remove(json.length() - 1);
    json += ",\"storage\":{\"available\":";
    json += storage.available() ? "true" : "false";
    json += ",\"total\":" + String(LittleFS.totalBytes());
    json += ",\"used\":" + String(LittleFS.usedBytes());
    json += "},\"remoteSyncEnabled\":";
    json += remoteSync.enabled() ? "true" : "false";
    sendJson(200, json + '}');
  });
  server.on("/api/history", HTTP_GET, []() { sendJson(200, storage.historyJson()); });
  server.on("/api/events", HTTP_GET, []() { sendJson(200, storage.eventsJson()); });
  server.on("/api/wifi/scan", HTTP_POST, []() {
    if (!requireSetupAuth()) return;
    if (!network.startScan()) { sendJson(503, "{\"error\":\"Unable to start scan\"}"); return; }
    sendJson(202, "{\"state\":\"scanning\"}");
  });
  server.on("/api/wifi/scan", HTTP_GET, []() { sendJson(200, network.scanJson()); });
  server.on("/api/wifi/configure", HTTP_POST, []() {
    if (!requireSetupAuth()) return;
    String error;
    if (!network.saveWiFi(jsonString(server.arg("plain"), "ssid"), jsonString(server.arg("plain"), "password"), error)) {
      sendJson(400, "{\"error\":\"" + error + "\"}");
      return;
    }
    storage.recordEvent("wifi_credentials_saved");
    sendJson(202, "{\"accepted\":true}");
  });
  server.on("/api/wifi/forget", HTTP_POST, []() {
    if (!requireSetupAuth()) return;
    network.forgetWiFi();
    storage.recordEvent("wifi_credentials_cleared");
    sendJson(200, "{\"ok\":true}");
  });
  server.on("/", HTTP_GET, []() {
    File file = LittleFS.open("/index.html", FILE_READ);
    if (!file) { server.send(503, "text/plain", "Web files are not installed."); return; }
    server.streamFile(file, "text/html");
    file.close();
  });
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/app.js", LittleFS, "/app.js");
  server.onNotFound([]() { server.send(404, "application/json", "{\"error\":\"Not found\"}"); });
}
}

void setup() {
  Serial.begin(115200);
  delay(100);
  GATEWAY_BOOT("LiTime ESP32 Gateway");
  if (!storage.begin()) GATEWAY_BOOT("LittleFS mount failed; local history is unavailable.");
  if (!network.begin()) GATEWAY_BOOT("Setup access point failed to start.");
  GATEWAY_BOOT(String("Setup Wi-Fi: ") + network.apSsid());
  GATEWAY_BOOT(String("Setup password: ") + network.setupPassword());
  GATEWAY_BOOT(String("Setup address: http://") + WiFi.softAPIP().toString());
  setupRoutes();
  server.begin();
  ota.begin();
  remoteSync.begin(storage);
  battery.begin();
  if (!scanner.begin()) {
    storage.recordEvent("ble_scanner_init_failed");
    GATEWAY_BOOT("BLE scanner failed to initialize.");
    return;
  }
  storage.recordEvent("gateway_started");
  attemptBatteryConnection();
}

void loop() {
  server.handleClient();
  network.loop();
  ota.loop();
  battery.update();
  const bool connected = battery.isConnected();
  if (connected != previousBatteryConnected) {
    storage.recordEvent(connected ? "battery_connected" : "battery_disconnected", &battery.getData());
    previousBatteryConnected = connected;
  }
  if (!connected && millis() - lastBatteryScanAt >= BATTERY_RESCAN_INTERVAL_MS) attemptBatteryConnection();
  storage.recordTelemetry(battery.getData());
  remoteSync.loop();
  delay(10);
}
