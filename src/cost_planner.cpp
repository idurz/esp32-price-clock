#include "cost_planner.h"

#include <algorithm>

#include "config.h"

namespace CostPlanner {

namespace {

constexpr int kPowerSampleMinutes = 5;

float powerAt(const Appliance::Profile &profile, int offsetMinutes) {
  float kW = profile.segments.front().kW;
  for (const auto &seg : profile.segments) {
    if (seg.offsetMinutes > offsetMinutes) {
      break;
    }
    kW = seg.kW;
  }
  return kW;
}

} // namespace

BestWindow findCheapestStart(const Appliance::Profile &profile,
                              const std::vector<PriceService::Slice> &today,
                              const std::vector<PriceService::Slice> &tomorrow,
                              int nowSliceIndex,
                              const std::vector<float> &solarGenerationKw) {
  BestWindow best;
  if (today.empty()) {
    return best;
  }

  std::vector<PriceService::Slice> combined = today;
  combined.insert(combined.end(), tomorrow.begin(), tomorrow.end());

  const int sliceMinutes = 1440 / (int)today.size();
  const int slicesNeeded = (profile.totalMinutes + sliceMinutes - 1) / sliceMinutes;

  for (int start = nowSliceIndex; start + slicesNeeded <= (int)combined.size(); start++) {
    float cost = 0.0f;
    // Fine-grained sampling preserves short high-power stages even with hourly prices.
    for (int offsetMinutes = 0; offsetMinutes < profile.totalMinutes;
         offsetMinutes += kPowerSampleMinutes) {
      const int sampleMinutes =
          std::min(kPowerSampleMinutes, profile.totalMinutes - offsetMinutes);
      const int priceOffset = offsetMinutes / sliceMinutes;
      const int priceIndex = start + priceOffset;
      if (priceIndex >= (int)combined.size()) {
        break;
      }
      const float kW = powerAt(profile, offsetMinutes);
      float gridKw = kW;
      const int forecastHour =
          (priceIndex * sliceMinutes + offsetMinutes % sliceMinutes) / 60;
      if (forecastHour >= 0 &&
          forecastHour < (int)solarGenerationKw.size()) {
        const float solarSurplus = std::max(
            0.0f, solarGenerationKw[forecastHour] - SOLAR_BASE_LOAD_KW);
        gridKw = std::max(0.0f, kW - solarSurplus);
      }
      const float kWh = gridKw * (sampleMinutes / 60.0f);
      cost += kWh * combined[priceIndex].total;
    }
    if (!best.valid || cost < best.estimatedCostFactor) {
      best.valid = true;
      best.startSliceIndex = start;
      best.estimatedCostFactor = cost;
    }
  }

  return best;
}

} // namespace CostPlanner
