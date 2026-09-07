#pragma once

#include <Arduino.h>

struct DiscoveredBattery {
  String name;
  String macAddress;
  uint8_t addressType = BLE_ADDR_PUBLIC;
  int rssi = 0;
};
