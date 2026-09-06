#pragma once

#include <Arduino.h>

struct DiscoveredBattery {
  String name;
  String macAddress;
  int rssi = 0;
};
