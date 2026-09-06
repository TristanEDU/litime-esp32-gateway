#pragma once

#include <Arduino.h>

class OtaService {
  public:
    void begin(const String& password);
    void loop();

  private:
    bool started = false;
};
