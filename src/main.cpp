#include <Arduino.h>

#include "appliance_profiles.h"
#include "config.h"
#include "cost_planner.h"
#include "display.h"
#include "price_service.h"
#include "solar_service.h"
#include "time_service.h"

namespace {

int s_lastRenderedMinute = -1;
int s_lastFetchYday = -1; // day-of-year of the last successful fetch
uint32_t s_nextRetryMs = 0;
bool s_initialFetchAttempted = false;
bool s_welcomePrinted = false;
bool s_solarFetchAttempted = false;
uint32_t s_nextSolarRetryMs = 0;
uint32_t s_lastSolarFetchMs = 0;

enum class Mode { Washer, Dryer, Summary };
Mode s_mode = Mode::Washer;
bool s_displayAwake = true;
uint32_t s_lastInteractionMs = 0;
bool s_lastButtonState = HIGH;
uint32_t s_lastButtonChangeMs = 0;

struct tm currentTm() {
  static struct tm lastKnownTime{};
  struct tm current{};
  if (getLocalTime(&current, 0)) {
    lastKnownTime = current;
  }
  return lastKnownTime;
}

void tryFetchPrices() {
  if (PriceService::fetchPrices()) {
    s_lastFetchYday = currentTm().tm_yday;
    s_nextRetryMs = 0;
    s_lastRenderedMinute = -1;
  } else {
    s_nextRetryMs = millis() + PRICE_FETCH_RETRY_MS;
  }
}

void pollApplianceButton() {
  const bool pressed = digitalRead(BUTTON_PIN) == LOW;
  const uint32_t now = millis();
  if (pressed != (s_lastButtonState == LOW) &&
      now - s_lastButtonChangeMs > BUTTON_DEBOUNCE_MS) {
    s_lastButtonChangeMs = now;
    s_lastButtonState = pressed ? LOW : HIGH;
    if (pressed) {
      s_lastInteractionMs = now;
      if (!s_displayAwake) {
        s_displayAwake = true;
        Display::setScreenOn(true);
        Serial.printf("GPIO%d button woke display.\n", BUTTON_PIN);
      } else {
        s_mode = static_cast<Mode>((static_cast<int>(s_mode) + 1) % 3);
        const char *modeName = s_mode == Mode::Washer
                                   ? "Washer"
                                   : s_mode == Mode::Dryer ? "Dryer" : "Summary";
        Serial.printf("GPIO%d button pressed; view: %s\n", BUTTON_PIN,
                      modeName);
      }
      s_lastRenderedMinute = -1;
    }
  }
}

} // namespace

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Display::begin();
  s_lastInteractionMs = millis();
  TimeService::begin();
}

void loop() {
  if (!s_welcomePrinted && Serial) {
    Serial.println("Welcome to ESP32 Price Clock.");
    Serial.printf("ESP32 reset reason: %d\n", (int)esp_reset_reason());
    s_welcomePrinted = true;
  }

  TimeService::update();
  pollApplianceButton();

  const struct tm now = currentTm();
  const bool networkReady =
      TimeService::isWifiConnected() && TimeService::isTimeValid();

  if (s_displayAwake &&
      millis() - s_lastInteractionMs >= DISPLAY_IDLE_TIMEOUT_MS) {
    s_displayAwake = false;
    Display::setScreenOn(false);
    s_lastRenderedMinute = -1;
  }

  if (s_displayAwake && now.tm_min != s_lastRenderedMinute) {
    s_lastRenderedMinute = now.tm_min;

    const auto &today = PriceService::today();
    const auto &tomorrow = PriceService::tomorrow();
    const auto &solarForecast = SolarService::generationForecastKw();

    if (s_mode == Mode::Summary) {
      Display::renderSummary(today, solarForecast);
    } else {
      const Appliance::Profile &profile =
          s_mode == Mode::Dryer ? Appliance::kDroger : Appliance::kWasmachine;
      int highlightStart = -1;
      int highlightCount = 0;
      if (!today.empty()) {
        const int sliceMinutes = 1440 / (int)today.size();
        const int nowSliceIndex = (now.tm_hour * 60 + now.tm_min) / sliceMinutes;
        const CostPlanner::BestWindow best = CostPlanner::findCheapestStart(
            profile, today, tomorrow, nowSliceIndex, solarForecast);
        if (best.valid && best.startSliceIndex < (int)today.size()) {
          highlightStart = best.startSliceIndex;
          highlightCount =
              (profile.totalMinutes + sliceMinutes - 1) / sliceMinutes;
        }
      }

      Display::render(today, solarForecast, now.tm_hour, now.tm_min,
                      s_mode == Mode::Dryer, highlightStart, highlightCount);
    }
  }

  // Refetch once per day shortly after local midnight so "today" rolls over,
  // plus a bounded retry loop if the last attempt failed.
  const bool dueForDailyFetch =
      now.tm_yday != s_lastFetchYday && now.tm_hour == 0 && now.tm_min >= 5 &&
      s_nextRetryMs == 0;
  const bool dueForRetry = s_nextRetryMs != 0 && millis() >= s_nextRetryMs;
  const bool dueForInitialFetch = !s_initialFetchAttempted &&
                                  TimeService::isWifiConnected();
  if (networkReady && (dueForInitialFetch || dueForDailyFetch || dueForRetry)) {
    s_initialFetchAttempted = true;
    tryFetchPrices();
  }

  const bool solarRetryDue = s_nextSolarRetryMs != 0 &&
                             millis() >= s_nextSolarRetryMs;
  const bool solarRefreshDue =
      !s_solarFetchAttempted || solarRetryDue ||
      (s_solarFetchAttempted && s_nextSolarRetryMs == 0 &&
       millis() - s_lastSolarFetchMs >= SOLAR_FETCH_INTERVAL_MS);
  if (SOLAR_FORECAST_ENABLED && networkReady && solarRefreshDue) {
    s_solarFetchAttempted = true;
    if (SolarService::fetchForecast()) {
      s_lastSolarFetchMs = millis();
      s_nextSolarRetryMs = 0;
      s_lastRenderedMinute = -1;
      Serial.printf("Open-Meteo solar forecast ready (%u hours).\n",
                    (unsigned)SolarService::generationForecastKw().size());
    } else {
      s_nextSolarRetryMs = millis() + PRICE_FETCH_RETRY_MS;
      Serial.println("Open-Meteo solar forecast unavailable; retaining tariff-only fallback.");
    }
  }

  delay(BUTTON_POLL_MS);
}

