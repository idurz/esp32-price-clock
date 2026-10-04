#pragma once

#include "appliance_profiles.h"
#include "price_service.h"

#include <vector>

namespace CostPlanner {

struct BestWindow {
  bool valid = false;
  int startSliceIndex = -1; // index into the combined today+tomorrow timeline
  float estimatedCostFactor = 0.0f; // sum(kWh * price); currency unit matches Tibber's `total`
};

// Finds the lowest estimated grid-import cost for `profile` within the combined
// today+tomorrow price timeline, starting no earlier than `nowSliceIndex`.
// Solar generation offsets appliance demand after the configured household load.
// Returns valid=false if there isn't enough future price data to fit the profile.
BestWindow findCheapestStart(const Appliance::Profile &profile,
                              const std::vector<PriceService::Slice> &today,
                              const std::vector<PriceService::Slice> &tomorrow,
                              int nowSliceIndex,
                              const std::vector<float> &solarGenerationKw);

} // namespace CostPlanner
