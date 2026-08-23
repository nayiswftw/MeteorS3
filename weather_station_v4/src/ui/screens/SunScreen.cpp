#include "ui/Screen.h"
#include "ui/Widgets.h"
#include "ui/Icons.h"
#include "ui/Theme.h"
#include "core/State.h"
#include "core/Format.h"

static void render(lv_obj_t* root) {
    const WeatherData& w = state::weather();
    if (!w.valid) return;

    // Sun Icon Graphic
    ui::weatherArt(root, 0, w.isDay, 90, 54, 1);

    const char* sunrise = (w.dailyCount > 0) ? w.daily[0].sunrise : "--:--";
    const char* sunset  = (w.dailyCount > 0) ? w.daily[0].sunset  : "--:--";

    // SUNRISE Card
    ui::metricCard(root, 10, 143, layout::CARD_W_HALF, 67, "SUNRISE", sunrise,
                   w.isDay ? "Today" : "Next morning", CLR_YELLOW);

    // SUNSET Card
    ui::metricCard(root, 125, 143, layout::CARD_W_HALF, 67, "SUNSET", sunset,
                   w.isDay ? "Today" : "Passed", CLR_ORANGE);

    // MOON Card
    char moonBuf[32];
    fmt::moonPhase(moonBuf, sizeof(moonBuf));
    ui::metricCard(root, 10, 221, layout::CARD_W_FULL, 68, "MOON", moonBuf,
                   w.isDay ? "Tonight" : "Current night", CLR_PURPLE);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_SUN,
    { "SUN & MOON", CLR_SUN_DAY, render }
);
