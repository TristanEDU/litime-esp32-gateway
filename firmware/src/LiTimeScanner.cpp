#include "LiTimeScanner.h"

static NimBLEUUID LITIME_SERVICE_UUID(
    "0000ffe0-0000-1000-8000-00805f9b34fb"
    );

LiTimeScanner::LiTimeScanner() {
}

bool LiTimeScanner::begin() {
  if (!NimBLEDevice::isInitialized()) {
    if (!NimBLEDevice::init("LiTime-ESP32-Gateway")) return false;
  }

  scanner = NimBLEDevice::getScan();

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

  NimBLEScanResults results = scanner->getResults(durationSeconds * 1000UL);

  for (int i = 0; i < results.getCount(); i++) {
    const NimBLEAdvertisedDevice* device = results.getDevice(i);
    if (device == nullptr) continue;

    bool serviceMatch =
      device->haveServiceUUID() &&
      device->isAdvertisingService(LITIME_SERVICE_UUID);

    String name = "";

    if (device->haveName()) {
      name = device->getName().c_str();
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
    battery.macAddress = device->getAddress().toString().c_str();
    battery.addressType = device->getAddress().getType();
    battery.rssi = device->getRSSI();

    batteries.push_back(battery);
  }

  scanner->clearResults();

  return batteries;
}
