#pragma once

#include <BMSClient.h>
#include "BatteryProvider.h"

class LiTimeBLEProvider : public BatteryProvider {
  public:
    LiTimeBLEProvider();

    bool begin() override;
    void update() override;
    bool isConnected() const override;
    const BatteryData& getData() const override;
    bool connect(const char* macAddress);

  private:
    String macAddress;
    BMSClient bms;
    BatteryData data;

    void syncData();
};
