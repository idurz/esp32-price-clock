#pragma once

#include "price_service.h"

namespace Display {

void begin();

// Turns the backlight and panel on/off (used for the screen-off appliance mode).
void setScreenOn(bool on);

// Redraws the full face: colored price ring + hour hand for the given time,
// plus an optional highlighted arc marking the recommended appliance start
// window (pass highlightCount <= 0 to skip the highlight).
void render(const std::vector<PriceService::Slice> &slices,
            const std::vector<float> &solarGenerationKw, int hour24, int minute,
            bool dryerMode, int highlightStart,
            int highlightCount);

void renderSummary(const std::vector<PriceService::Slice> &slices,
                   const std::vector<float> &solarGenerationKw);

} // namespace Display
