#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <vector>

#include "DiscoveredBattery.h"

class LiTimeScanner {
  public:
    LiTimeScanner();

    bool begin();
    std::vector<DiscoveredBattery> scan(uint32_t durationSeconds = 10);

  private:
    BLEScan* scanner = nullptr;
};
