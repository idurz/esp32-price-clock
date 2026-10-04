#include "time_service.h"

#include <WiFi.h>
#include <time.h>

#include "config.h"
#include "secrets.h"

namespace {
bool s_timeValid = false;
bool s_ntpStarted = false;
}

namespace TimeService {

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void update() {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  if (!s_ntpStarted) {
    configTzTime(TZ_INFO, NTP_SERVER_1, NTP_SERVER_2);
    s_ntpStarted = true;
  }

  if (!s_timeValid) {
    struct tm timeinfo;
    s_timeValid = getLocalTime(&timeinfo, 0);
  }
}

bool isTimeValid() { return s_timeValid; }
bool isWifiConnected() { return WiFi.status() == WL_CONNECTED; }

int currentHour24() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) {
    return 0;
  }
  return timeinfo.tm_hour;
}

int currentMinute() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0)) {
    return 0;
  }
  return timeinfo.tm_min;
}

} // namespace TimeService
