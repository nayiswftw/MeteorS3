#include "src/ui/Screen.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include "src/core/State.h"
#include "src/core/Format.h"
#include <stdio.h>

static void render(lv_obj_t* root) {
    state::lock();
    WeatherData w = state::weather();
    state::unlock();
    if (!w.valid || w.hourlyCount == 0) return;

    constexpr int CHART_POINTS = 16;
    float temps[CHART_POINTS];
    int count = min(CHART_POINTS, w.hourlyCount);

    for (int i = 0; i < count; i++) {
        temps[i] = w.hourly[i].temperature;
    }

    ui::label(root, "Temperature", 10, 56, 100, 16, &lv_font_montserrat_14, CLR_MUTED);
    ui::chart(root, 10, 78, 220, 85, temps, count, CLR_CYAN);

    // Time ticks below chart
    for (int i = 0; i < 4; i++) {
        int idx = i * 4;
        if (idx >= count) break;

        const char* tStr = (i == 0) ? "NOW" : w.hourly[idx].time;
        ui::label(root, tStr, 9 + i * 58, 168, 52, 16,
                  &lv_font_montserrat_14, (i == 0) ? CLR_CYAN : CLR_MUTED, LV_TEXT_ALIGN_CENTER);
    }

    // 4-step forecast strip (every 2h)
    lv_obj_t* strip = ui::panel(root, 10, 197, 220, 92, CLR_PANEL, 18);

    for (int i = 0; i < 4; i++) {
        int idx = i * 2;
        if (idx >= w.hourlyCount) break;

        const HourData& h = w.hourly[idx];
        int x = i * 55;

        // Temp
        char tBuf[16];
        fmt::temperature(tBuf, sizeof(tBuf), h.temperature);
        ui::label(strip, tBuf, x, 9, 55, 17, &lv_font_montserrat_14, CLR_TEXT, LV_TEXT_ALIGN_CENTER);

        // Rain chance
        char rBuf[16];
        snprintf(rBuf, sizeof(rBuf), "%d%%", h.rainChance);
        ui::label(strip, rBuf, x, 34, 55, 17, &lv_font_montserrat_14,
                  (h.rainChance >= 50) ? CLR_BLUE : CLR_DIM, LV_TEXT_ALIGN_CENTER);

        // Wind
        char wBuf[16];
        fmt::wind(wBuf, sizeof(wBuf), h.wind);
        ui::label(strip, wBuf, x, 59, 55, 17, &lv_font_montserrat_14, CLR_MUTED, LV_TEXT_ALIGN_CENTER);
    }
}

static const bool s_reg = screen::registerScreen(
    SCREEN_HOURLY,
    { "48 HOUR FORECAST", CLR_HOURLY_TOP, render }
);
