#include "LiTimeScanner.h"

static BLEUUID LITIME_SERVICE_UUID(
    "0000ffe0-0000-1000-8000-00805f9b34fb"
    );

LiTimeScanner::LiTimeScanner() {
}

bool LiTimeScanner::begin() {
  if (!BLEDevice::getInitialized()) {
    BLEDevice::init("LiTime-ESP32-Gateway");
  }

  scanner = BLEDevice::getScan();

  if (scanner == nullptr) {
    return false;
  }

  scanner->setActiveScan(true);
  scanner->setInterval(100);
  scanner->setWindow(99);

  return true;
}

std::vector<DiscoveredBattery>
LiTimeScanner::scan(uint32_t durationSeconds) {
  std::vector<DiscoveredBattery> batteries;

  if (scanner == nullptr) {
    return batteries;
  }

  BLEScanResults* results = scanner->start(durationSeconds, false);

  for (int i = 0; i < results->getCount(); i++) {
    BLEAdvertisedDevice device = results->getDevice(i);

    bool serviceMatch =
      device.haveServiceUUID() &&
      device.isAdvertisingService(LITIME_SERVICE_UUID);

    String name = "";

    if (device.haveName()) {
      name = device.getName().c_str();
    }

    bool nameMatch =
      name.startsWith("L-") ||
      name.startsWith("LT-") ||
      name.indexOf("LiTime") >= 0;

    if (!serviceMatch && !nameMatch) {
      continue;
    }

    DiscoveredBattery battery;

    battery.name = name;
    battery.macAddress = 
      device.getAddress().toString().c_str();
    battery.rssi = device.getRSSI();

    batteries.push_back(battery);
  }

  scanner->clearResults();

  return batteries;
}
