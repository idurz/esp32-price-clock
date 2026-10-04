#pragma once

#include <Arduino.h>
#include <vector>

namespace PriceService {

enum class Level { DeliverBack, Cheap, Normal, Pricy, Expensive };

struct Slice {
  float total = 0.0f;
  Level level = Level::Normal;
  String startsAt;
};

// Fetches today's (+tomorrow's, if published) hourly/quarter-hourly prices
// from the Tibber API and replaces the cached slice lists on success.
// Returns true on success; on failure the previous cache (if any) is kept.
bool fetchPrices();

// Slices for the current calendar day only — this is what the ring renders,
// one full revolution == one day, matching the 24h hour hand.
const std::vector<Slice> &today();
const std::vector<Slice> &tomorrow();

} // namespace PriceService
