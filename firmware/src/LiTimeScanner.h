#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <vector>

#include "DiscoveredBattery.h"

class LiTimeScanner {
  public:
    LiTimeScanner();

    bool begin();
    std::vector<DiscoveredBattery> scan(uint32_t durationSeconds = 10);

  private:
    NimBLEScan* scanner = nullptr;
};
