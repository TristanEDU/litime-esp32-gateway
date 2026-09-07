#pragma once

#include "BatteryProvider.h"
#include "LiTimeNimBLEClient.h"

class LiTimeBLEProvider : public BatteryProvider {
  public:
    LiTimeBLEProvider();

    bool begin() override;
    void update() override;
    bool isConnected() const override;
    const BatteryData& getData() const override;
    bool connect(const char* macAddress, uint8_t addressType = BLE_ADDR_PUBLIC);

  private:
    String macAddress;
    LiTimeNimBLEClient bms;
    BatteryData data;

    void syncData();
};
