#pragma once

#include <Arduino.h>

class StorageManager;

class RemoteSync {
 public:
  void begin(StorageManager& storage);
  void loop();
  bool enabled() const;

 private:
  StorageManager* storage = nullptr;
  uint32_t lastAttemptAt = 0;
  bool publishNextBatch();
};
