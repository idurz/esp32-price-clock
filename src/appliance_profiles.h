#pragma once

#include <vector>

namespace Appliance {

// A power draw step: from `offsetMinutes` (since cycle start) until the next
// segment's offset (or `Profile::totalMinutes` for the last one), power is `kW`.
struct PowerSegment {
  int offsetMinutes;
  float kW;
};

struct Profile {
  const char *name;
  std::vector<PowerSegment> segments; // sorted by offsetMinutes, starting at 0
  int totalMinutes;
};

// Digitized from a measured smart-plug power graph (~15min resolution, approximate):
// fill+heat peak ~1.35kW, long low-power wash/rinse baseline, mid-cycle bump,
// final heat/spin peak ~1.2kW, ~4h40m total.
extern const Profile kWasmachine;

// Digitized from a measured smart-plug power graph (~5min resolution, approximate):
// short ramp, ~1.1kW plateau for ~30min, short tail-off, ~40min total.
extern const Profile kDroger;

} // namespace Appliance
