#include "ui/Screen.h"
#include "ui/Widgets.h"
#include "ui/Theme.h"
#include "core/State.h"
#include <stdio.h>

static void render(lv_obj_t* root) {
    const WeatherData& w = state::weather();
    if (!w.valid) return;

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

    ui::label(root, "Next 12 hours", 10, 187, 120, 16, &lv_font_montserrat_14, CLR_MUTED);
    ui::chart(root, 10, 208, 220, 64, uvValues, count, CLR_YELLOW);

    // Peak label
    char peakBuf[48];
    if (peak >= 0.0f) {
        snprintf(peakBuf, sizeof(peakBuf), "Peak %.1f at %s", peak, w.hourly[peakIdx].time);
    } else {
        snprintf(peakBuf, sizeof(peakBuf), "No UV forecast");
    }
    ui::label(root, peakBuf, 10, 278, 220, 17, &lv_font_montserrat_14, CLR_MUTED, LV_TEXT_ALIGN_CENTER);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_UV,
    { "UV CENTER", CLR_UV_TOP, render }
);
