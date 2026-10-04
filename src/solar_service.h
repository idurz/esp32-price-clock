#pragma once

#include <Arduino.h>
#include <vector>

namespace SolarService {

// Fetches a two-day Open-Meteo irradiance forecast and estimates PV output in kW.
bool fetchForecast();

// Hourly estimated PV output, starting at local midnight today.
const std::vector<float> &generationForecastKw();

} // namespace SolarService