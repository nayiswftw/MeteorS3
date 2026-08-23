#include "ui/Screen.h"
#include "ui/Widgets.h"
#include "ui/Icons.h"
#include "ui/Theme.h"
#include "core/State.h"
#include "core/Format.h"
#include <stdio.h>

static void render(lv_obj_t* root) {
    const WeatherData& w = state::weather();
    const AirData&     a = state::air();
    const InsightData& in = state::insights();

    if (!w.valid) {
        ui::label(root, "--", 10, 83, 130, 45, &lv_font_montserrat_32, CLR_TEXT);
        ui::label(root, "Waiting for weather", 10, 135, 220, 20, &lv_font_montserrat_14, CLR_MUTED);
        return;
    }

    // Weather icon art
    ui::weatherArt(root, w.weatherCode, w.isDay, 158, 59, 1);

    // Large Temperature
    char tempBuf[16];
    fmt::temperature(tempBuf, sizeof(tempBuf), w.temperature);
    ui::label(root, tempBuf, 10, 72, 143, 45, &lv_font_montserrat_32, CLR_TEXT);

    // Weather Name
    char nameBuf[32];
    fmt::weatherName(nameBuf, sizeof(nameBuf), w.weatherCode);
    ui::label(root, nameBuf, 12, 118, 140, 19, &lv_font_montserrat_14, CLR_TEXT);

    // Secondary info: Feels like, High, Low
    char secBuf[64];
    char appBuf[16], highBuf[16], lowBuf[16];
    fmt::temperature(appBuf, sizeof(appBuf), w.apparent);
    if (w.dailyCount > 0) {
        fmt::temperature(highBuf, sizeof(highBuf), w.daily[0].high);
        fmt::temperature(lowBuf, sizeof(lowBuf), w.daily[0].low);
        snprintf(secBuf, sizeof(secBuf), "Feels %s  H %s  L %s", appBuf, highBuf, lowBuf);
    } else {
        snprintf(secBuf, sizeof(secBuf), "Feels %s", appBuf);
    }
    ui::label(root, secBuf, 12, 139, 218, 17, &lv_font_montserrat_14, CLR_MUTED);

    // Insight card
    lv_obj_t* insightCard = ui::panel(root, 10, 164, 220, 49, CLR_PANEL_ALT, 18);
    ui::label(insightCard, in.primary, 12, 8, 196, 17, &lv_font_montserrat_14, CLR_TEXT);
    ui::label(insightCard, in.secondary, 12, 27, 196, 15, &lv_font_montserrat_14, CLR_DIM);

    // RAIN card
    int rainChance = (w.hourlyCount > 0) ? w.hourly[0].rainChance : 0;
    char rainValBuf[16], rainCapBuf[32];
    snprintf(rainValBuf, sizeof(rainValBuf), "%d%%", rainChance);
    if (in.nextRainHours < 0) {
        snprintf(rainCapBuf, sizeof(rainCapBuf), "No strong signal");
    } else {
        snprintf(rainCapBuf, sizeof(rainCapBuf), "Next in %dh", in.nextRainHours);
    }
    ui::metricCard(root, 10, 224, layout::CARD_W_HALF, 65, "RAIN", rainValBuf, rainCapBuf, CLR_BLUE);

    // AQI card
    char aqiValBuf[16], aqiCapBuf[32];
    lv_color_t aqiAccent = CLR_MUTED;
    if (a.valid) {
        snprintf(aqiValBuf, sizeof(aqiValBuf), "%d", a.usAqi);
        fmt::aqiCategory(aqiCapBuf, sizeof(aqiCapBuf), a.usAqi);
        aqiAccent = ui::aqiColor(a.usAqi);
    } else {
        snprintf(aqiValBuf, sizeof(aqiValBuf), "--");
        fmt::age(aqiCapBuf, sizeof(aqiCapBuf), w.fetchedAt);
    }
    ui::metricCard(root, 125, 224, layout::CARD_W_HALF, 65, "AQI", aqiValBuf, aqiCapBuf, aqiAccent);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_HOME,
    { nullptr, CLR_HOME_DAY, render }
);
