#pragma once

#include <Arduino.h>

class RemoteDashboard {
 public:
  using StatusProvider = String (*)();

  void begin(StatusProvider statusProvider);
  void loop();
};
