#pragma once

#include <Arduino.h>
#include "BatteryData.h"

class StorageManager {
 public:
  bool begin();
  bool available() const;
  void recordTelemetry(const BatteryData& data);
  void recordEvent(const char* event, const BatteryData* data = nullptr);
  String historyJson(size_t limit = 48) const;
  String eventsJson(size_t limit = 32) const;

  String nextRemoteBatch(size_t maxItems, size_t& itemCount) const;
  bool acknowledgeRemoteBatch(size_t itemCount);

 private:
  bool mounted = false;
  uint32_t lastSampleAt = 0;

  void appendLine(const char* path, const String& line, size_t maximumBytes);
  void rotate(const char* path, size_t maximumBytes);
  String tailLinesJson(const char* path, size_t limit, bool telemetry) const;
};
