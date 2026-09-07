#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>

#include <vector>

// Minimal, read-only LiTime protocol transport. The parsing intentionally
// mirrors the prior BMS_Client fields, while the transport uses NimBLE rather
// than the classic ESP32 BLE stack.
class LiTimeNimBLEClient {
 public:
  struct BMSData {
    float totalVoltage = 0.0f;
    float cellVoltageSum = 0.0f;
    float current = 0.0f;
    int16_t mosfetTemp = 0;
    int16_t cellTemp = 0;
    uint8_t soc = 0;
    String soh;
    float remainingAh = 0.0f;
    float fullCapacityAh = 0.0f;
    String protectionState;
    String heatState;
    String balanceMemory;
    String failureState;
    String balancingState;
    String batteryState;
    uint32_t dischargesCount = 0;
    float dischargesAhCount = 0.0f;
    std::vector<float> cellVoltages;
  };

  LiTimeNimBLEClient();
  ~LiTimeNimBLEClient();

  bool begin();
  bool connect(const char* deviceAddress, uint8_t addressType = BLE_ADDR_PUBLIC);
  void disconnect();
  bool isConnected() const;
  void update();
  const BMSData& getData() const;

  float getTotalVoltage() const;
  float getCurrent() const;
  int16_t getMosfetTemp() const;
  int16_t getCellTemp() const;
  uint8_t getSOC() const;
  float getRemainingAh() const;
  float getFullCapacityAh() const;
  std::vector<float> getCellVoltages() const;

 private:
  class ClientCallbacks : public NimBLEClientCallbacks {
   public:
    explicit ClientCallbacks(LiTimeNimBLEClient& owner) : owner(owner) {}
    void onConnect(NimBLEClient* client) override;
    void onDisconnect(NimBLEClient* client, int reason) override;

   private:
    LiTimeNimBLEClient& owner;
  };

  NimBLEAddress deviceAddress;
  NimBLEClient* client = nullptr;
  NimBLERemoteCharacteristic* writeCharacteristic = nullptr;
  NimBLERemoteCharacteristic* notifyCharacteristic = nullptr;
  ClientCallbacks clientCallbacks;
  BMSData currentData;
  bool connected = false;
  unsigned long lastQuery = 0;

  void clearConnection();
  void parseNotificationData(uint8_t* data, size_t length);
  static String bytesToHexString(const uint8_t* data, int start, int length, bool reverse = true);
  static String bytesToBinaryString(const uint8_t* data, int start, int length);
};
