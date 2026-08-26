#include "src/ui/Screen.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include "src/core/State.h"
#include <stdio.h>

static void render(lv_obj_t* root) {
    state::lock();
    WeatherData w = state::weather();
    state::unlock();

    if (!w.valid) {
        ui::metricCard(root, 10, 100, layout::CARD_W_FULL, 85, "UV INDEX", "Syncing...", "Fetching solar radiation & UV index", CLR_UV_TOP);
        return;
    }

    // UV Gauge
    int gaugeVal = (int)constrain(w.uv * 10.0f, 0.0f, 110.0f);
    char uvCenterBuf[16];
    snprintf(uvCenterBuf, sizeof(uvCenterBuf), "%.1f", w.uv);

    const char* uvCategory = "Low";
    if (w.uv >= 8.0f) uvCategory = "Very high";
    else if (w.uv >= 6.0f) uvCategory = "High";
    else if (w.uv >= 3.0f) uvCategory = "Moderate";

    ui::gauge(root, 60, 52, 120, gaugeVal, 0, 110, uvCenterBuf, uvCategory,
              (w.uv >= 6.0f) ? CLR_YELLOW : CLR_GREEN);

    // 12-hour UV Chart
    constexpr int COUNT = 12;
    float uvValues[COUNT];
    int count = min(COUNT, w.hourlyCount);
    float peak = -1.0f;
    int peakIdx = 0;

    for (int i = 0; i < count; i++) {
        uvValues[i] = w.hourly[i].uv;
        if (uvValues[i] > peak) {
            peak = uvValues[i];
            peakIdx = i;
        }
    }

    ui::label(root, "Next 12 hours", 10, 187, 120, 15, &lv_font_montserrat_12, CLR_MUTED);
    ui::chart(root, 10, 206, 220, 66, uvValues, count, CLR_YELLOW);

    // Peak label
    char peakBuf[48];
    if (peak >= 0.0f) {
        snprintf(peakBuf, sizeof(peakBuf), "Peak %.1f at %s", peak, w.hourly[peakIdx].time);
    } else {
        snprintf(peakBuf, sizeof(peakBuf), "No UV forecast");
    }
    ui::label(root, peakBuf, 10, 278, 220, 16, &lv_font_montserrat_12, CLR_MUTED, LV_TEXT_ALIGN_CENTER);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_UV,
    { "UV CENTER", CLR_UV_TOP, render }
);
