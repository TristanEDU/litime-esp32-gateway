#pragma once

#include "BatteryData.h"

class BatteryProvider {
  public:
    virtual ~BatteryProvider() = default;

    virtual bool begin() = 0;
    virtual void update() = 0;
    virtual bool isConnected() const = 0;
    virtual const BatteryData& getData() const = 0;
};
