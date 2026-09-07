#include "StorageManager.h"

#include <LittleFS.h>
#include <time.h>

#include "GatewayConfig.h"

namespace {
  constexpr size_t kTelemetryMaximumBytes = 71680;
  constexpr size_t kEventsMaximumBytes = 4096;
  constexpr size_t kRemoteQueueMaximumBytes = 8192;

  String csvValue(float value, uint8_t decimals) { return String(value, static_cast<unsigned int>(decimals)); }

  String telemetryCsv(const BatteryData& data) {
    String line;
    line.reserve(96);
    line += String(millis());
    line += ',' + csvValue(data.voltage, 2);
    line += ',' + csvValue(data.current, 2);
    line += ',' + String(data.soc);
    line += ',' + csvValue(data.power, 1);
    line += ',' + csvValue(data.cellTemp, 1);
    line += ',' + csvValue(data.mosfetTemp, 1);
    line += ',' + csvValue(data.cellDeltaMv, 1);
    return line;
  }

  String telemetryJson(const BatteryData& data) {
    String json = "{\"uptime_ms\":" + String(millis());
    const time_t now = time(nullptr);
    json += ",\"timestamp\":";
    json += now > 1700000000 ? String(static_cast<unsigned long>(now)) : "null";
    json += ",\"voltage\":" + csvValue(data.voltage, 2);
    json += ",\"current\":" + csvValue(data.current, 2);
    json += ",\"soc\":" + String(data.soc);
    json += ",\"power\":" + csvValue(data.power, 1);
    json += ",\"cell_temp\":" + csvValue(data.cellTemp, 1);
    json += ",\"mosfet_temp\":" + csvValue(data.mosfetTemp, 1);
    json += ",\"cell_delta_mv\":" + csvValue(data.cellDeltaMv, 1);
    return json + '}';
  }

  String jsonEscape(const String& value) {
    String escaped;
    for (size_t i = 0; i < value.length(); ++i) {
      if (value[i] == '\\' || value[i] == '"') escaped += '\\';
      escaped += value[i];
    }
    return escaped;
  }
}

bool StorageManager::begin() {
  mounted = LittleFS.begin(
      true,
      "/littlefs",
      10,
      "littlefs"
      );
  if (!mounted) return false;
  if (!LittleFS.exists("/telemetry.csv")) {
    File file = LittleFS.open("/telemetry.csv", FILE_WRITE);
    if (file) file.println("uptime_ms,voltage,current,soc,power,cell_temp,mosfet_temp,cell_delta_mv");
    file.close();
  }
  return true;
}

bool StorageManager::available() const { return mounted; }

void StorageManager::recordTelemetry(const BatteryData& data) {
  if (!mounted || !data.valid || millis() - lastSampleAt < TELEMETRY_SAMPLE_INTERVAL_MS) return;
  lastSampleAt = millis();
  appendLine("/telemetry.csv", telemetryCsv(data), kTelemetryMaximumBytes);
  appendLine("/remote.ndjson", telemetryJson(data), kRemoteQueueMaximumBytes);
}

void StorageManager::recordEvent(const char* event, const BatteryData* data) {
  if (!mounted) return;
  String line = "{\"uptime_ms\":" + String(millis()) + ",\"event\":\"";
  line += jsonEscape(event);
  line += "\"";
  if (data != nullptr && data->valid) {
    line += ",\"voltage\":" + String(data->voltage, 2);
    line += ",\"soc\":" + String(data->soc);
  }
  line += '}';
  appendLine("/events.ndjson", line, kEventsMaximumBytes);
}

String StorageManager::historyJson(size_t limit) const {
  return tailLinesJson("/telemetry.csv", limit, true);
}

String StorageManager::eventsJson(size_t limit) const {
  return tailLinesJson("/events.ndjson", limit, false);
}

String StorageManager::nextRemoteBatch(size_t maxItems, size_t& itemCount) const {
  itemCount = 0;
  if (!mounted || !LittleFS.exists("/remote.ndjson")) return "[]";
  File file = LittleFS.open("/remote.ndjson", FILE_READ);
  if (!file) return "[]";
  String json = "[";
  while (file.available() && itemCount < maxItems) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.isEmpty()) continue;
    if (itemCount++ > 0) json += ',';
    json += line;
  }
  file.close();
  return json + ']';
}

bool StorageManager::acknowledgeRemoteBatch(size_t itemCount) {
  if (!mounted || itemCount == 0) return true;
  File input = LittleFS.open("/remote.ndjson", FILE_READ);
  File output = LittleFS.open("/remote.tmp", FILE_WRITE);
  if (!input || !output) {
    input.close(); output.close();
    return false;
  }
  size_t consumed = 0;
  while (input.available()) {
    const String line = input.readStringUntil('\n');
    if (!line.isEmpty() && consumed < itemCount) {
      ++consumed;
      continue;
    }
    if (!line.isEmpty()) output.println(line);
  }
  input.close(); output.close();
  LittleFS.remove("/remote.ndjson");
  return LittleFS.rename("/remote.tmp", "/remote.ndjson");
}

void StorageManager::appendLine(const char* path, const String& line, size_t maximumBytes) {
  rotate(path, maximumBytes);
  File file = LittleFS.open(path, FILE_APPEND);
  if (file) file.println(line);
  file.close();
}

void StorageManager::rotate(const char* path, size_t maximumBytes) {
  File file = LittleFS.open(path, FILE_READ);
  const size_t size = file ? file.size() : 0;
  file.close();
  if (size < maximumBytes) return;
  String previous = String(path) + ".1";
  LittleFS.remove(previous);
  LittleFS.rename(path, previous);
}

String StorageManager::tailLinesJson(const char* path, size_t limit, bool telemetry) const {
  if (!mounted || !LittleFS.exists(path)) return "[]";
  File file = LittleFS.open(path, FILE_READ);
  if (!file) return "[]";
  size_t total = 0;
  while (file.available()) {
    file.readStringUntil('\n');
    ++total;
  }
  const size_t skip = total > limit ? total - limit : 0;
  file.seek(0);
  String json = "[";
  size_t index = 0;
  size_t emitted = 0;
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (index++ < skip || line.isEmpty()) continue;
    if (telemetry && line.startsWith("uptime_ms,")) continue;
    if (emitted++ > 0) json += ',';
    if (telemetry) {
      int separators[7]; int found = 0;
      for (size_t i = 0; i < line.length() && found < 7; ++i) if (line[i] == ',') separators[found++] = i;
      if (found != 7) { --emitted; continue; }
      String values[8]; int start = 0;
      for (int i = 0; i < 7; ++i) { values[i] = line.substring(start, separators[i]); start = separators[i] + 1; }
      values[7] = line.substring(start);
      json += "{\"uptime_ms\":" + values[0] + ",\"voltage\":" + values[1] + ",\"current\":" + values[2] + ",\"soc\":" + values[3] + ",\"power\":" + values[4] + ",\"cell_temp\":" + values[5] + ",\"mosfet_temp\":" + values[6] + ",\"cell_delta_mv\":" + values[7] + '}';
    } else {
      json += line;
    }
  }
  file.close();
  return json + ']';
}
