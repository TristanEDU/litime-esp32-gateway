#include "LiTimeBLEProvider.h"

LiTimeBLEProvider::LiTimeBLEProvider() {
}

bool LiTimeBLEProvider::begin() {
  data.connected = false;
  data.simulated = false;
  data.valid = false;
  return bms.begin();
}

bool LiTimeBLEProvider::connect(const char* macAddress, uint8_t addressType) {
  this->macAddress = macAddress;

  if (!bms.connect(this->macAddress.c_str(), addressType)) {
    data.connected = false;
    return false;
  }

  data.connected = true;
  data.simulated = false;
  data.valid = false;

  bms.update();
  syncData();

  return true;
}

void LiTimeBLEProvider::update() {
  if (!bms.isConnected()) {
    data.connected = false;
    data.valid = false;
    return;
  }

  bms.update();
  syncData();
}

bool LiTimeBLEProvider::isConnected() const {
  return data.connected;
}

const BatteryData& LiTimeBLEProvider::getData() const {
  return data;
}

void LiTimeBLEProvider::syncData() {
  data.connected = bms.isConnected();
  data.simulated = false;

  data.voltage = bms.getTotalVoltage();
  data.current = bms.getCurrent();
  data.power = data.voltage * data.current;

  data.soc = bms.getSOC(); 

  data.remainingAh = bms.getRemainingAh();
  data.capacityAh = bms.getFullCapacityAh();

  data.cellTemp = bms.getCellTemp();
  data.mosfetTemp = bms.getMosfetTemp();

  std::vector<float> cells = bms.getCellVoltages();

  data.cellCount = cells.size();

  if (data.cellCount > MAX_BATTERY_CELLS) {
    data.cellCount = MAX_BATTERY_CELLS;
  }

  for (size_t i = 0; i < data.cellCount; i++) {
    data.cells[i] = cells[i];
  }

  for (size_t i = data.cellCount; i < MAX_BATTERY_CELLS; i++) {
    data.cells[i] = 0.0f;
  }

  if (data.cellCount > 0) {
    data.minCell = data.cells[0];
    data.maxCell = data.cells[0];

    for (size_t i = 1; i < data.cellCount; i++) {
      if (data.cells[i] < data.minCell) {
        data.minCell = data.cells[i];
      }
      if (data.cells[i] > data.maxCell) {
        data.maxCell = data.cells[i];
      }
    }

    data.cellDeltaMv = 
      (data.maxCell - data.minCell) * 1000.0f;
  } else {
    data.minCell = 0.0f;
    data.maxCell = 0.0f;
    data.cellDeltaMv = 0.0f;
  }

  data.valid =
    data.voltage > 0.0f &&
    data.cellCount > 0;
  data.updatedAt = millis();
}
