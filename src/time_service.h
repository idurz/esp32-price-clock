#pragma once

#include <Arduino.h>

namespace TimeService {

// Starts WiFi connection and advances time synchronization without blocking.
void begin();
void update();

bool isTimeValid();
bool isWifiConnected();
int currentHour24();
int currentMinute();

} // namespace TimeService
