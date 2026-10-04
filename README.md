# ESP32 Price Clock

A round-display clock that shows today's Tibber electricity prices as a 24-hour
colored ring, with a 24-hour hour hand and a minute hand. It also
highlights the cheapest window to run an appliance (washing machine or dryer),
based on digitized power profiles of each appliance's cycle.

## Hardware

- **Board:** Wemos S2 Mini (ESP32-S2FN4R2)
- **Display:** Waveshare 1.28" round LCD module (GC9A01 driver, 240x240, SPI)
- **Input:** external momentary push button between GPIO4 and GND

## Pin layout

| Signal              | S2 Mini GPIO | Notes                                |
| -------------------- | ------------ | ------------------------------------- |
| Display SCLK         | 12           | SPI clock                             |
| Display MOSI (DIN)   | 11           | SPI data out                          |
| Display DC           | 13           | Data/command select                   |
| Display CS           | 10           | Chip select                           |
| Display RST          | 14           | Panel reset                           |
| Display BL           | 9            | Backlight (PWM)                       |
| Mode button           | 4            | Active LOW, button connects to GND    |

Display MISO is unused (panel is write-only). Pin assignments live in
[include/config.h](include/config.h); the SPI/panel setup itself is in
[include/LGFX_Config.h](include/LGFX_Config.h).

## Features

- Fetches hourly/quarter-hourly prices from the Tibber API and renders them as
  colored wedges around the ring (cheap / normal / pricy / expensive / deliver-back).
- Synchronizes local time via NTP.
- Finds the lowest estimated grid-cost contiguous start window for the selected
  appliance, accounting for forecast solar generation when available.
- Re-fetches prices once per day after local midnight, with retry-on-failure.

## Solar-aware planning

- The clock requests hourly `global_tilted_irradiance` from the Open-Meteo
  Forecast API for the configured location and panel orientation. It estimates
  PV output as irradiance × array capacity × system efficiency.
- For each candidate washer/dryer cycle, estimated generation minus
  `SOLAR_BASE_LOAD_KW` offsets appliance demand. Only the remaining estimated
  grid import is scored against the Tibber price. Without a forecast, planning
  falls back to tariff-only scoring.
- On the dial, blue marks price slices where forecast solar generation exceeds
  the household baseline by at least `SOLAR_DISPLAY_SURPLUS_KW`. Green appliance
  windows are overlaid by solar blue, and purple zero/negative-price slices
  remain purple.
- Configure location, array capacity, panel tilt/azimuth, and typical household
  load in [include/config.h](include/config.h). The current base-load estimate
  is `1.0 kW`. This is an estimate, not live inverter telemetry.
- Forecast refreshes every six hours. Open-Meteo data attribution: Open-Meteo,
  CC BY 4.0. The free API is limited to non-commercial use under its terms.

## Configuration

Review [include/config.h](include/config.h) before flashing. It contains the
timezone, display and button GPIO assignments, button timing, price-ring colors,
minute-hand length, width and color, and solar forecast settings (enable flag,
coordinates, panel capacity, orientation, system efficiency, and household
base load. Set `MINUTE_HAND_ENABLED` to `0` to hide the cyan minute hand while
keeping the 24-hour hour hand. `DISPLAY_IDLE_TIMEOUT_MS` controls how long the
display stays on without a button press. Panel azimuth uses Open-Meteo's
convention: `0` is south, `-90` east, and `+90` west. Keep WiFi credentials and
the Tibber token in `include/secrets.h`; do not put credentials in `config.h`.

## Display and controls

- The price ring covers the full day: `00` is at the top and hours increase
  clockwise through `23`. The white marker on the ring shows the current time's
  position among the day's prices.
- The white hour hand makes one revolution every 24 hours. The cyan minute hand
  makes one revolution each hour.
- Price colors follow Tibber's price level: green is cheap or very cheap, gray
  is normal or unclassified, yellow is expensive, and red is very expensive.
  Purple indicates a zero or negative price (electricity delivered back to the
  grid). Blue marks solar-surplus hours and overrides the green appliance
  highlight; green overrides other positive Tibber prices.
- The selected washer or dryer start window is highlighted green across the
  price ring, replacing the Tibber colors for those hours.
- Press the external momentary button between GPIO4 and GND to cycle **Washer →
  Dryer → Summary → Washer**. The Summary view shows average Tibber price and
  estimated solar kWh for blue solar-surplus, green cheap, yellow expensive, and
  red very-expensive hours. After five minutes without a press, the display
  blanks; the next press wakes the last-selected view. The input uses the
  internal pull-up, so no external resistor is needed.

## Project layout

- `src/main.cpp` — setup/loop, mode button handling, daily price refresh
- `src/display.*` — LovyanGFX rendering (price ring, clock hands, time marker, highlight arc)
- `src/price_service.*` — Tibber API fetch + price level classification
- `src/solar_service.*` — Open-Meteo forecast fetch + estimated PV generation
- `src/cost_planner.*` — cheapest-start-window search over the price timeline
- `src/appliance_profiles.*` — digitized washer/dryer power-draw profiles
- `src/time_service.*` — WiFi + NTP time sync
- `include/config.h` — pins, colors, timing constants
- `include/LGFX_Config.h` — LovyanGFX panel/bus configuration
- `include/secrets.h` — WiFi + Tibber credentials (gitignored; copy from `secrets.h.example`)

## Setup

1. Copy [include/secrets.h.example](include/secrets.h.example) to `include/secrets.h`
   and fill in your WiFi credentials and a
   [Tibber personal access token](https://developer.tibber.com/settings/access-token)
   (needs the `HOME` scope).
2. Wire the Waveshare 1.28" display to the S2 Mini per the pin table above.
3. Build and flash:

   ```
   pio run -t upload
   ```
