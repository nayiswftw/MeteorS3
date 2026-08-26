#include "src/ui/Screen.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include "src/core/State.h"
#include "src/core/Format.h"

static void render(lv_obj_t* root) {
    state::lock();
    WeatherData w = state::weather();
    state::unlock();

    if (!w.valid) {
        ui::metricCard(root, 10, 100, layout::CARD_W_FULL, 85, "WIND & GUSTS", "Syncing...", "Fetching wind vectors & gusts", CLR_WIND_TOP);
        return;
    }

    // Wind Speed Gauge
    char windBuf[16], dirBuf[16];
    fmt::wind(windBuf, sizeof(windBuf), w.wind);
    fmt::direction(dirBuf, sizeof(dirBuf), w.direction);
    ui::gauge(root, 10, 57, 100, (int)w.wind, 0, 100, windBuf, dirBuf, CLR_GREEN);

    // Gust Speed Gauge
    char gustBuf[16];
    fmt::wind(gustBuf, sizeof(gustBuf), w.gust);
    ui::gauge(root, 130, 57, 100, (int)w.gust, 0, 120, gustBuf, "Gust",
              (w.gust >= 50.0f) ? CLR_ORANGE : CLR_CYAN);

    // Gust Trend Chart
    constexpr int COUNT = 12;
    float gusts[COUNT];
    int count = min(COUNT, w.hourlyCount);

    for (int i = 0; i < count; i++) {
        gusts[i] = w.hourly[i].gust;
    }

    ui::label(root, "Gust trend", 10, 174, 100, 15, &lv_font_montserrat_12, CLR_MUTED);
    ui::chart(root, 10, 194, 220, 91, gusts, count, CLR_GREEN);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_WIND,
    { "WIND CENTER", CLR_WIND_TOP, render }
);
