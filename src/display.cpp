#include "display.h"

#include "LGFX_Config.h"
#include "config.h"

namespace {

LGFX s_lcd;
lgfx::LGFX_Sprite s_frame(&s_lcd);

constexpr int kCenter = 120;
constexpr int kRingOuter = 116;
constexpr int kRingInner = 99;
constexpr int kHourHandLength = 54;

uint16_t colorFor(PriceService::Level level) {
  switch (level) {
  case PriceService::Level::DeliverBack:
    return s_frame.color565(COLOR_DELIVER_BACK_R);
  case PriceService::Level::Cheap:
    return s_frame.color565(COLOR_CHEAP_R);
  case PriceService::Level::Pricy:
    return s_frame.color565(COLOR_PRICY_R);
  case PriceService::Level::Expensive:
    return s_frame.color565(COLOR_EXPENSIVE_R);
  case PriceService::Level::Normal:
  default:
    return s_frame.color565(COLOR_NORMAL_R);
  }
}

// The price ring uses a full revolution for the 24-hour day.
void drawRing(const std::vector<PriceService::Slice> &slices) {
  const size_t n = slices.size();
  if (n == 0) {
    return;
  }
  const float degPerSlice = 360.0f / n;
  for (size_t i = 0; i < n; i++) {
    const float startDeg = i * degPerSlice - 90.0f;
    const float endDeg = startDeg + degPerSlice;
    s_frame.fillArc(kCenter, kCenter, kRingOuter, kRingInner, startDeg, endDeg,
                    colorFor(slices[i].level));
  }
}

void drawSolarAndNegativePriceOverrides(
    const std::vector<PriceService::Slice> &slices,
    const std::vector<float> &solarGenerationKw) {
  const size_t n = slices.size();
  if (n == 0) {
    return;
  }

  const float degPerSlice = 360.0f / n;
  for (size_t i = 0; i < n; i++) {
    uint16_t overrideColor;
    if (slices[i].level == PriceService::Level::DeliverBack) {
      overrideColor = s_frame.color565(COLOR_DELIVER_BACK_R);
    } else {
      const int forecastHour = (int)(i * 1440 / n / 60);
      if (forecastHour >= (int)solarGenerationKw.size() ||
          solarGenerationKw[forecastHour] - SOLAR_BASE_LOAD_KW <
              SOLAR_DISPLAY_SURPLUS_KW) {
        continue;
      }
      overrideColor = s_frame.color565(COLOR_SOLAR_R);
    }

    const float startDeg = i * degPerSlice - 90.0f;
    s_frame.fillArc(kCenter, kCenter, kRingOuter, kRingInner, startDeg,
                    startDeg + degPerSlice, overrideColor);
  }
}

void drawCurrentPriceMarker(int hour24, int minute) {
  const float dayFraction = (hour24 + minute / 60.0f) / 24.0f;
  const float angle = dayFraction * 360.0f - 90.0f;
  const float angleRad = angle * DEG_TO_RAD;
  const int innerX = kCenter + (int)(cosf(angleRad) * (kRingInner + 4));
  const int innerY = kCenter + (int)(sinf(angleRad) * (kRingInner + 4));
  const int outerX = kCenter + (int)(cosf(angleRad) * (kRingOuter - 4));
  const int outerY = kCenter + (int)(sinf(angleRad) * (kRingOuter - 4));
  s_frame.drawWideLine(innerX, innerY, outerX, outerY, 4, TFT_WHITE);
}

void draw24HourScale() {
  const uint16_t minorTick = s_frame.color565(130, 130, 130);
  for (int hour = 0; hour < 24; hour++) {
    const float angle = (hour * 15.0f - 90.0f) * DEG_TO_RAD;
    const int innerRadius = 90;
    const int outerRadius = 96;
    const int innerX = kCenter + (int)(cosf(angle) * innerRadius);
    const int innerY = kCenter + (int)(sinf(angle) * innerRadius);
    const int outerX = kCenter + (int)(cosf(angle) * outerRadius);
    const int outerY = kCenter + (int)(sinf(angle) * outerRadius);
    s_frame.drawWideLine(innerX, innerY, outerX, outerY, 1, minorTick);
  }

  s_frame.setTextColor(TFT_WHITE, TFT_BLACK);
  s_frame.setTextDatum(middle_center);
  for (int hour = 0; hour < 24; hour++) {
    const float angle = (hour * 15.0f - 90.0f) * DEG_TO_RAD;
    const int labelX = kCenter + (int)(cosf(angle) * 80);
    const int labelY = kCenter + (int)(sinf(angle) * 80);
    char label[3];
    std::snprintf(label, sizeof(label), "%02d", hour);
    s_frame.drawString(label, labelX, labelY);
  }
}

void drawClockHands(int hour24, int minute) {
  const float minuteAngle = (minute * 6.0f - 90.0f) * DEG_TO_RAD;
  const float dayFraction = (hour24 + minute / 60.0f) / 24.0f;
  const float hourAngle = (dayFraction * 360.0f - 90.0f) * DEG_TO_RAD;
  const int minuteX =
      kCenter + (int)(cosf(minuteAngle) * MINUTE_HAND_LENGTH);
  const int minuteY =
      kCenter + (int)(sinf(minuteAngle) * MINUTE_HAND_LENGTH);
  const int hourX = kCenter + (int)(cosf(hourAngle) * kHourHandLength);
  const int hourY = kCenter + (int)(sinf(hourAngle) * kHourHandLength);
  if (MINUTE_HAND_ENABLED) {
    s_frame.drawWideLine(kCenter, kCenter, minuteX, minuteY, MINUTE_HAND_WIDTH,
                         s_frame.color565(MINUTE_HAND_COLOR_R));
  }
  s_frame.drawWideLine(kCenter, kCenter, hourX, hourY, 5, TFT_WHITE);
  s_frame.fillCircle(kCenter, kCenter, 4, TFT_WHITE);
}

void drawApplianceIcon(bool dryerMode) {
  constexpr int left = 98;
  constexpr int top = 58;
  constexpr int width = 44;
  constexpr int height = 46;
  const uint16_t accent = s_frame.color565(90, 220, 255);

  s_frame.drawRoundRect(left, top, width, height, 4, TFT_WHITE);
  s_frame.drawLine(left + 2, top + 11, left + width - 3, top + 11,
                   TFT_WHITE);
  s_frame.drawRect(left + 5, top + 4, 10, 3, accent);
  s_frame.drawCircle(left + 34, top + 6, 2, TFT_WHITE);
  s_frame.drawCircle(kCenter, top + 29, 13, TFT_WHITE);
  s_frame.drawCircle(kCenter, top + 29, 10, accent);

  if (dryerMode) {
    s_frame.drawWideLine(kCenter - 6, top + 25, kCenter, top + 29, 2, accent);
    s_frame.drawWideLine(kCenter, top + 29, kCenter + 6, top + 25, 2, accent);
    s_frame.drawWideLine(kCenter - 6, top + 33, kCenter, top + 29, 2, accent);
    s_frame.drawWideLine(kCenter, top + 29, kCenter + 6, top + 33, 2, accent);
  } else {
    s_frame.fillCircle(kCenter - 4, top + 27, 2, accent);
    s_frame.fillCircle(kCenter + 3, top + 25, 2, accent);
    s_frame.fillCircle(kCenter + 2, top + 33, 2, accent);
  }
}

// Overlays the recommended appliance window across the price ring so it stands
// out from the underlying Tibber prices.
void drawHighlight(size_t totalSlices, int highlightStart, int highlightCount) {
  if (totalSlices == 0 || highlightCount <= 0) {
    return;
  }
  const float degPerSlice = 360.0f / totalSlices;
  const float startDeg = highlightStart * degPerSlice - 90.0f;
  const float endDeg = startDeg + highlightCount * degPerSlice;
  s_frame.fillArc(kCenter, kCenter, kRingOuter, kRingInner, startDeg, endDeg,
                  s_frame.color565(COLOR_CHEAP_R));
}

void drawSummaryScreen(const std::vector<PriceService::Slice> &slices,
                       const std::vector<float> &solarGenerationKw) {
  struct Bucket {
    float priceTotal = 0.0f;
    float priceHours = 0.0f;
    float solarKwh = 0.0f;
  };
  Bucket buckets[4];
  const char *labels[] = {"BLUE  SUN", "GREEN CHEAP", "YELLOW EXP", "RED V.EXP"};
  const uint16_t colors[] = {
      s_frame.color565(COLOR_SOLAR_R), s_frame.color565(COLOR_CHEAP_R),
      s_frame.color565(COLOR_PRICY_R), s_frame.color565(COLOR_EXPENSIVE_R)};

  if (!slices.empty()) {
    const int sliceMinutes = 1440 / (int)slices.size();
    const float durationHours = sliceMinutes / 60.0f;
    for (size_t i = 0; i < slices.size(); i++) {
      if (slices[i].level == PriceService::Level::DeliverBack ||
          slices[i].level == PriceService::Level::Normal) {
        continue;
      }

      const int forecastHour = (int)i * sliceMinutes / 60;
      const bool solarSurplus =
          forecastHour < (int)solarGenerationKw.size() &&
          solarGenerationKw[forecastHour] - SOLAR_BASE_LOAD_KW >=
              SOLAR_DISPLAY_SURPLUS_KW;
      int bucketIndex = -1;
      if (solarSurplus) {
        bucketIndex = 0;
      } else if (slices[i].level == PriceService::Level::Cheap) {
        bucketIndex = 1;
      } else if (slices[i].level == PriceService::Level::Pricy) {
        bucketIndex = 2;
      } else if (slices[i].level == PriceService::Level::Expensive) {
        bucketIndex = 3;
      }
      if (bucketIndex < 0) {
        continue;
      }

      Bucket &bucket = buckets[bucketIndex];
      bucket.priceTotal += slices[i].total * durationHours;
      bucket.priceHours += durationHours;
      if (forecastHour < (int)solarGenerationKw.size()) {
        bucket.solarKwh += solarGenerationKw[forecastHour] * durationHours;
      }
    }
  }

  s_frame.setTextDatum(middle_center);
  s_frame.setTextColor(TFT_WHITE, TFT_BLACK);
  s_frame.drawString("TODAY: PRICE / SOLAR", kCenter, 37);
  s_frame.setTextColor(s_frame.color565(170, 170, 180), TFT_BLACK);
  s_frame.setTextDatum(middle_right);
  s_frame.drawString("EUR/kWh", 171, 57);
  s_frame.drawString("PV kWh", 211, 57);

  for (int i = 0; i < 4; i++) {
    const int y = 83 + i * 34;
    s_frame.fillRoundRect(20, y - 10, 200, 28, 4,
                          s_frame.color565(34, 36, 42));
    s_frame.fillCircle(32, y + 4, 5, colors[i]);
    s_frame.setTextDatum(middle_left);
    s_frame.setTextColor(colors[i], TFT_BLACK);
    s_frame.drawString(labels[i], 43, y + 1);

    char priceText[16];
    char solarText[12];
    if (buckets[i].priceHours > 0.0f) {
      const float averagePrice =
          buckets[i].priceTotal / buckets[i].priceHours;
      std::snprintf(priceText, sizeof(priceText), "\xE2\x82\xAC%.2f",
                    averagePrice);
    } else {
      std::snprintf(priceText, sizeof(priceText), "--");
    }
    std::snprintf(solarText, sizeof(solarText), "%.2f", buckets[i].solarKwh);
    s_frame.setTextDatum(middle_right);
    s_frame.setTextColor(TFT_WHITE, TFT_BLACK);
    s_frame.drawString(priceText, 171, y + 1);
    s_frame.drawString(solarText, 211, y + 1);
  }
}

} // namespace

namespace Display {

void begin() {
  s_lcd.init();
  s_lcd.setRotation(0);
  s_lcd.setBrightness(255);
  s_frame.setColorDepth(16);
  s_frame.setPsram(true);
  if (!s_frame.createSprite(240, 240)) {
    Serial.println("LCD framebuffer allocation failed.");
  }
}

void setScreenOn(bool on) {
  s_lcd.setBrightness(on ? 255 : 0);
}

void render(const std::vector<PriceService::Slice> &slices,
            const std::vector<float> &solarGenerationKw, int hour24,
            int minute, bool dryerMode, int highlightStart,
            int highlightCount) {
  s_frame.fillSprite(TFT_BLACK);
  drawRing(slices);
  drawHighlight(slices.size(), highlightStart, highlightCount);
  drawSolarAndNegativePriceOverrides(slices, solarGenerationKw);
  drawApplianceIcon(dryerMode);
  draw24HourScale();
  drawCurrentPriceMarker(hour24, minute);
  drawClockHands(hour24, minute);
  s_frame.pushSprite(0, 0);
}

void renderSummary(const std::vector<PriceService::Slice> &slices,
                   const std::vector<float> &solarGenerationKw) {
  s_frame.fillSprite(TFT_BLACK);
  drawSummaryScreen(slices, solarGenerationKw);
  s_frame.pushSprite(0, 0);
}

} // namespace Display
