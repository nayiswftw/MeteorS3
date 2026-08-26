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

    if (!w.valid || w.dailyCount == 0) {
        ui::metricCard(root, 10, 100, layout::CARD_W_FULL, 85, "10-DAY FORECAST", "Syncing...", "Fetching 10-day outlook", CLR_WEEK_TOP);
        return;
    }

    int days = min(7, w.dailyCount);
    float globalLow  = 999.0f;
    float globalHigh = -999.0f;

    for (int i = 0; i < days; i++) {
        if (!isnan(w.daily[i].low))  globalLow  = min(globalLow,  w.daily[i].low);
        if (!isnan(w.daily[i].high)) globalHigh = max(globalHigh, w.daily[i].high);
    }

    int y = 52;
    for (int i = 0; i < days; i++) {
        const DayData& d = w.daily[i];

        // Date (e.g. "TODAY" or "08-23")
        const char* dateStr = (i == 0) ? "TODAY" : (strlen(d.date) >= 5 ? d.date + 5 : d.date);
        ui::label(root, dateStr, 10, y, 48, 16, &lv_font_montserrat_12,
                  (i == 0) ? CLR_CYAN : CLR_MUTED);

        // Low Temp
        char lowBuf[16];
        fmt::tempShort(lowBuf, sizeof(lowBuf), d.low);
        ui::label(root, lowBuf, 54, y, 42, 16, &lv_font_montserrat_14, CLR_MUTED, LV_TEXT_ALIGN_RIGHT);

        // Range Bar
        ui::rangeBar(root, 100, y + 6, 80, d.low, d.high, globalLow, globalHigh,
                     (i == 0) ? CLR_YELLOW : CLR_ORANGE);

        // High Temp
        char highBuf[16];
        fmt::tempShort(highBuf, sizeof(highBuf), d.high);
        ui::label(root, highBuf, 184, y, 46, 16, &lv_font_montserrat_14, CLR_TEXT, LV_TEXT_ALIGN_RIGHT);

        // Rain % under date if > 0
        if (d.rainChance > 0) {
            char rainBuf[16];
            snprintf(rainBuf, sizeof(rainBuf), "%d%%", d.rainChance);
            ui::label(root, rainBuf, 10, y + 15, 48, 14, &lv_font_montserrat_12,
                      (d.rainChance >= 50) ? CLR_BLUE : CLR_DIM);
        }

        y += 34;
    }
}

static const bool s_reg = screen::registerScreen(
    SCREEN_WEEK,
    { "10 DAY OUTLOOK", CLR_WEEK_TOP, render }
);
