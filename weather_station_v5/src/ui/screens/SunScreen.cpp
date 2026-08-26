#include "src/ui/Screen.h"
#include "src/ui/Widgets.h"
#include "src/ui/Icons.h"
#include "src/ui/Theme.h"
#include "src/core/State.h"
#include "src/core/Format.h"
#include <time.h>

static void render(lv_obj_t* root) {
    state::lock();
    WeatherData w = state::weather();
    state::unlock();

    // Sun Icon Graphic on left
    ui::weatherArt(root, 0, true, 20, 52, 1);

    // High-resolution Lunar Art on right
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    int year = t ? (t->tm_year + 1900) : 2026;
    int month = t ? (t->tm_mon + 1) : 8;
    int day = t ? t->tm_mday : 24;

    // Conway's accurate Moon Phase approximation
    double c = 0, e = 0, jd = 0, b = 0;
    if (month < 3) { year--; month += 12; }
    month++;
    c = 365.25 * year;
    e = 30.6 * month;
    jd = c + e + day - 694039.09;
    jd /= 29.5305882;
    b = jd - (int)jd;
    float phase = (float)b;

    ui::lunarArt(root, 155, 54, 52, phase);

    const char* sunrise = (w.dailyCount > 0) ? w.daily[0].sunrise : "--:--";
    const char* sunset  = (w.dailyCount > 0) ? w.daily[0].sunset  : "--:--";

    // SUNRISE Card
    ui::metricCard(root, 10, 140, layout::CARD_W_HALF, 68, "SUNRISE", sunrise,
                   w.isDay ? "Dawn today" : "Next morning", CLR_YELLOW);

    // SUNSET Card
    ui::metricCard(root, 125, 140, layout::CARD_W_HALF, 68, "SUNSET", sunset,
                   w.isDay ? "Dusk today" : "Passed", CLR_ORANGE);

    // MOON Card
    char moonBuf[32];
    fmt::moonPhase(moonBuf, sizeof(moonBuf));
    char moonIllumBuf[48];
    float illum = (1.0f - cosf(phase * 2.0f * 3.14159265f)) * 50.0f;
    snprintf(moonIllumBuf, sizeof(moonIllumBuf), "%s (%.0f%% illuminated)", moonBuf, illum);
    ui::metricCard(root, 10, 218, layout::CARD_W_FULL, 68, "LUNAR CYCLE", moonIllumBuf,
                   w.isDay ? "Tonight" : "Current sky", CLR_PURPLE);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_SUN,
    { "CELESTIAL HUB", CLR_SUN_DAY, render }
);

