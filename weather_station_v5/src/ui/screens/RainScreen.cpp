#include "src/ui/Screen.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include "src/core/State.h"
#include <stdio.h>

static void render(lv_obj_t* root) {
    state::lock();
    WeatherData w  = state::weather();
    InsightData in = state::insights();
    state::unlock();
    if (!w.valid || w.hourlyCount == 0) return;

    int peakChance   = 0;
    float total12h   = 0.0f;
    int peakHour     = 0;

    constexpr int COUNT = 12;
    float rainValues[COUNT];
    int count = min(COUNT, w.hourlyCount);

    for (int i = 0; i < count; i++) {
        const HourData& h = w.hourly[i];
        rainValues[i] = h.precipitation;
        if (!isnan(h.precipitation)) {
            total12h += h.precipitation;
        }
        if (h.rainChance > peakChance) {
            peakChance = h.rainChance;
            peakHour   = i;
        }
    }

    // NEXT RAIN Card
    char nextValBuf[16], nextCapBuf[32];
    if (in.nextRainHours < 0) {
        snprintf(nextValBuf, sizeof(nextValBuf), "None");
    } else {
        snprintf(nextValBuf, sizeof(nextValBuf), "%dh", in.nextRainHours);
    }
    if (peakChance > 0) {
        snprintf(nextCapBuf, sizeof(nextCapBuf), "Peak %d%%", peakChance);
    } else {
        snprintf(nextCapBuf, sizeof(nextCapBuf), "No strong signal");
    }
    ui::metricCard(root, 10, 57, layout::CARD_W_HALF, 71, "NEXT RAIN", nextValBuf, nextCapBuf, CLR_BLUE);

    // 12H TOTAL Card
    char totValBuf[16], totCapBuf[32];
    snprintf(totValBuf, sizeof(totValBuf), "%.1f mm", total12h);
    if (peakChance > 0) {
        snprintf(totCapBuf, sizeof(totCapBuf), "%s wettest", w.hourly[peakHour].time);
    } else {
        snprintf(totCapBuf, sizeof(totCapBuf), "Dry");
    }
    ui::metricCard(root, 125, 57, layout::CARD_W_HALF, 71, "12H TOTAL", totValBuf, totCapBuf, CLR_CYAN);

    // Chart
    ui::label(root, "Precipitation", 10, 143, 120, 16, &lv_font_montserrat_14, CLR_MUTED);
    ui::chart(root, 10, 166, 220, 92, rainValues, count, CLR_BLUE);

    // Probability now
    char probBuf[32];
    int nowChance = w.hourly[0].rainChance;
    snprintf(probBuf, sizeof(probBuf), "Probability now %d%%", nowChance);
    ui::label(root, probBuf, 10, 268, 220, 18, &lv_font_montserrat_14,
              (nowChance >= 60) ? CLR_BLUE : CLR_MUTED, LV_TEXT_ALIGN_CENTER);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_RAIN,
    { "RAIN CENTER", CLR_RAIN_TOP, render }
);
