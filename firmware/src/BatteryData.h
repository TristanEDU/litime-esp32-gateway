#pragma once

#include <Arduino.h>

static const size_t MAX_BATTERY_CELLS = 16;

struct BatteryData {
	float voltage = 0.0f;
	float current = 0.0f;
	float power = 0.0f;

	uint8_t soc = 0;

	float remainingAh = 0.0f;
	float capacityAh = 0.0f;

	float cellTemp = 0.0f;
	float mosfetTemp = 0.0f;

	float cells[MAX_BATTERY_CELLS] = {0};
	size_t cellCount = 0;

	float minCell = 0.0f;
	float maxCell = 0.0f;
	float cellDeltaMv = 0.0f;

	bool connected = false;
	bool simulated = false;
  bool valid = false;

	unsigned long updatedAt = 0;
};
