#include <Arduino.h>

#include "src/LiTimeBLEProvider.h"
#include "src/LiTimeScanner.h"

LiTimeBLEProvider battery;
LiTimeScanner scanner;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("LiTime ESP32 Gateway");
  Serial.println("====================");

  battery.begin();

  if (!scanner.begin()) {
    Serial.println("BLE scanner failed to initialize.");
    return;
  }

  Serial.println("Scanning for LiTime batteries...");
  Serial.println();

  std::vector<DiscoveredBattery> batteries = 
    scanner.scan(10);

  Serial.print("Found ");
  Serial.print(batteries.size());
  Serial.println(" compatible batteries.");
  Serial.println();

  for (size_t i = 0; i < batteries.size(); i++) {
    Serial.print("[");
    Serial.print(i + 1);
    Serial.println("]");

    Serial.print("Name: ");
    Serial.println(batteries[i].name);

    Serial.print("MAC:  ");
    Serial.println(batteries[i].macAddress);

    Serial.print("RSSI: ");
    Serial.println(batteries[i].rssi);

    Serial.println();
  }

  if (!batteries.empty()) {
    Serial.println("Connecting to first discovered battery...");

    if (battery.connect(batteries[0].macAddress.c_str())) {
      Serial.println("Battery connected successfully.");
    } else {
      Serial.println("Battery connection failed.");
    }
  }

}

void loop() {
  battery.update();

  if (battery.isConnected()) {
  const BatteryData& data = battery.getData();

  if (!data.valid) {
    Serial.println("Waiting for first valid telemetry...");
  } else {
    Serial.print("Voltage: ");
    Serial.print(data.voltage, 2);
    Serial.println(" V");

    Serial.print("Current: ");
    Serial.print(data.current, 2);
    Serial.println(" A");

    Serial.print("SOC: ");
    Serial.print(data.soc);
    Serial.println(" %");

    Serial.println();
  }
}

  delay(3000);
}
