#include "LiTimeNimBLEClient.h"

namespace {
constexpr char kServiceUuid[] = "0000ffe0-0000-1000-8000-00805f9b34fb";
constexpr char kWriteUuid[] = "0000ffe2-0000-1000-8000-00805f9b34fb";
constexpr char kNotifyUuid[] = "0000ffe1-0000-1000-8000-00805f9b34fb";
constexpr uint8_t kQueryCommand[] = {0x00, 0x00, 0x04, 0x01, 0x13, 0x55, 0xAA, 0x17};

uint32_t littleEndianU32(const uint8_t* data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}
}  // namespace

LiTimeNimBLEClient::LiTimeNimBLEClient() : clientCallbacks(*this) {}

LiTimeNimBLEClient::~LiTimeNimBLEClient() {
  disconnect();
}

bool LiTimeNimBLEClient::begin() {
  if (NimBLEDevice::isInitialized()) return true;
  return NimBLEDevice::init("LiTime-ESP32-Gateway");
}

bool LiTimeNimBLEClient::connect(const char* address, uint8_t addressType) {
  if (address == nullptr || address[0] == '\0') return false;
  if (!begin()) return false;
  if (isConnected() && deviceAddress.toString() == address) return true;

  disconnect();
  deviceAddress = NimBLEAddress(std::string(address), addressType);
  client = NimBLEDevice::createClient(deviceAddress);
  if (client == nullptr) return false;

  client->setClientCallbacks(&clientCallbacks, false);
  client->setConnectionParams(12, 12, 0, 150);
  client->setConnectTimeout(5000);

  if (!client->connect(deviceAddress)) {
    clearConnection();
    return false;
  }

  NimBLERemoteService* service = client->getService(kServiceUuid);
  if (service == nullptr) {
    clearConnection();
    return false;
  }

  writeCharacteristic = service->getCharacteristic(kWriteUuid);
  notifyCharacteristic = service->getCharacteristic(kNotifyUuid);
  if (writeCharacteristic == nullptr || notifyCharacteristic == nullptr ||
      !notifyCharacteristic->canNotify()) {
    clearConnection();
    return false;
  }

  if (!notifyCharacteristic->subscribe(
          true,
          [this](NimBLERemoteCharacteristic*, uint8_t* data, size_t length, bool) {
            parseNotificationData(data, length);
          })) {
    clearConnection();
    return false;
  }

  connected = true;
  lastQuery = 0;
  return true;
}

void LiTimeNimBLEClient::disconnect() {
  if (client != nullptr && client->isConnected()) client->disconnect();
  clearConnection();
}

bool LiTimeNimBLEClient::isConnected() const {
  return connected && client != nullptr && client->isConnected();
}

void LiTimeNimBLEClient::update() {
  if (!isConnected()) {
    connected = false;
    return;
  }

  if (millis() - lastQuery > 1000) {
    if (!writeCharacteristic->writeValue(kQueryCommand, sizeof(kQueryCommand), true)) {
      connected = false;
      return;
    }
    lastQuery = millis();
  }
}

const LiTimeNimBLEClient::BMSData& LiTimeNimBLEClient::getData() const {
  return currentData;
}

float LiTimeNimBLEClient::getTotalVoltage() const { return currentData.totalVoltage; }
float LiTimeNimBLEClient::getCurrent() const { return currentData.current; }
int16_t LiTimeNimBLEClient::getMosfetTemp() const { return currentData.mosfetTemp; }
int16_t LiTimeNimBLEClient::getCellTemp() const { return currentData.cellTemp; }
uint8_t LiTimeNimBLEClient::getSOC() const { return currentData.soc; }
float LiTimeNimBLEClient::getRemainingAh() const { return currentData.remainingAh; }
float LiTimeNimBLEClient::getFullCapacityAh() const { return currentData.fullCapacityAh; }
std::vector<float> LiTimeNimBLEClient::getCellVoltages() const { return currentData.cellVoltages; }

void LiTimeNimBLEClient::clearConnection() {
  connected = false;
  writeCharacteristic = nullptr;
  notifyCharacteristic = nullptr;
  if (client != nullptr) {
    NimBLEDevice::deleteClient(client);
    client = nullptr;
  }
}

void LiTimeNimBLEClient::parseNotificationData(uint8_t* data, size_t length) {
  if (data == nullptr || length < 104) return;

  currentData.totalVoltage = littleEndianU32(data + 8) / 1000.0f;
  currentData.cellVoltageSum = littleEndianU32(data + 12) / 1000.0f;
  currentData.current = static_cast<int32_t>(littleEndianU32(data + 48)) / 1000.0f;
  currentData.mosfetTemp = static_cast<int16_t>((data[55] << 8) | data[54]);
  currentData.cellTemp = static_cast<int16_t>((data[53] << 8) | data[52]);
  currentData.remainingAh = static_cast<uint16_t>((data[63] << 8) | data[62]) / 100.0f;
  currentData.fullCapacityAh = static_cast<uint16_t>((data[65] << 8) | data[64]) / 100.0f;
  currentData.protectionState = bytesToHexString(data, 76, 4);
  currentData.heatState = bytesToHexString(data, 68, 4);
  currentData.balanceMemory = bytesToHexString(data, 72, 4);
  currentData.failureState = bytesToHexString(data, 80, 3);
  currentData.balancingState = bytesToBinaryString(data, 84, 4);
  currentData.batteryState = bytesToHexString(data, 88, 2);
  currentData.soc = static_cast<uint8_t>((data[91] << 8) | data[90]);
  currentData.soh = String(littleEndianU32(data + 92)) + "%";
  currentData.dischargesCount = littleEndianU32(data + 96);
  currentData.dischargesAhCount = littleEndianU32(data + 100) / 1000.0f;

  currentData.cellVoltages.clear();
  for (int index = 16; index < 48; index += 2) {
    if (data[index] == 0 && data[index + 1] == 0) continue;
    currentData.cellVoltages.push_back(
        static_cast<uint16_t>((data[index + 1] << 8) | data[index]) / 1000.0f);
  }
}

String LiTimeNimBLEClient::bytesToHexString(const uint8_t* data, int start, int length, bool reverse) {
  String result;
  result.reserve(length * 2);
  for (int index = 0; index < length; ++index) {
    const int offset = reverse ? start + length - 1 - index : start + index;
    char buffer[3];
    snprintf(buffer, sizeof(buffer), "%02x", data[offset]);
    result += buffer;
  }
  return result;
}

String LiTimeNimBLEClient::bytesToBinaryString(const uint8_t* data, int start, int length) {
  String result;
  result.reserve(length * 8);
  for (int index = length - 1; index >= 0; --index) {
    for (int bit = 7; bit >= 0; --bit) result += (data[start + index] & (1 << bit)) ? '1' : '0';
  }
  return result;
}

void LiTimeNimBLEClient::ClientCallbacks::onConnect(NimBLEClient*) {
  owner.connected = true;
}

void LiTimeNimBLEClient::ClientCallbacks::onDisconnect(NimBLEClient*, int) {
  owner.connected = false;
}
