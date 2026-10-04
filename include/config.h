#pragma once

// ---- Time / locale ----
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"
// POSIX TZ string, e.g. Central European Time with DST rules.
#define TZ_INFO "CET-1CEST,M3.5.0,M10.5.0/3"

// ---- Tibber price colors (as R,G,B triplets, converted with color565() at use site) ----
// "Deliver back" always wins when a slice's price is <= 0, regardless of level.
#define COLOR_CHEAP_R 24, 255, 10
#define COLOR_NORMAL_R 171, 171, 181
#define COLOR_PRICY_R 247, 255, 15
#define COLOR_EXPENSIVE_R 251, 19, 42
#define COLOR_DELIVER_BACK_R 114, 42, 248
#define COLOR_SOLAR_R 0, 150, 255

// ---- Refresh cadence ----
#define PRICE_FETCH_RETRY_MS (60UL * 1000UL)      // retry cadence on failure
#define PRICE_FETCH_DAILY_HOUR 14                  // local hour to fetch tomorrow's prices
#define BUTTON_POLL_MS (10UL)                      // responsive button sampling
#define DISPLAY_IDLE_TIMEOUT_MS (5UL * 60UL * 1000UL)

// ---- Clock minute hand ----
#define MINUTE_HAND_ENABLED 1
#define MINUTE_HAND_LENGTH 72
#define MINUTE_HAND_WIDTH 3
#define MINUTE_HAND_COLOR_R 90, 220, 255

// ---- Solar forecast (Open-Meteo) ----
#define SOLAR_FORECAST_ENABLED 1
#define SOLAR_LATITUDE 51.342f                         // decimal degrees, north positive
#define SOLAR_LONGITUDE 5.431f                        // decimal degrees, east positive
#define SOLAR_ARRAY_CAPACITY_KWP 8.2f               // installed panel capacity
#define SOLAR_PANEL_TILT_DEG 15.0f                   // 0 horizontal, 90 vertical
#define SOLAR_PANEL_AZIMUTH_DEG 90.0f                // Open-Meteo: 0 south, -90 east, +90 west
#define SOLAR_BASE_LOAD_KW 1.0f                     // optional typical household load
#define SOLAR_SYSTEM_EFFICIENCY 0.80f                // approximate inverter/system losses
#define SOLAR_DISPLAY_SURPLUS_KW 0.2f                // blue ring means surplus above this
#define SOLAR_FETCH_INTERVAL_MS (6UL * 60UL * 60UL * 1000UL)

// ---- Display SPI pins (ESP32-S2 <-> Waveshare 1.28" round GC9A01 module) ----
// Adjust to match your actual wiring.
#define BOARD_PIN_SCLK 12
#define BOARD_PIN_MOSI 11
#define BOARD_PIN_DC 13
#define BOARD_PIN_CS 10
#define BOARD_PIN_RST 14

// ---- Backlight ----
#define BOARD_PIN_BL 9

// ---- External mode button ----
// Connect a momentary button between GPIO4 and GND (active LOW).
#define BUTTON_PIN 4
#define BUTTON_DEBOUNCE_MS 50
