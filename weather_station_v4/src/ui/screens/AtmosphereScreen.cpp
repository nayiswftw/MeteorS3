#include "ui/Screen.h"
#include "ui/Widgets.h"
#include "ui/Theme.h"
#include "core/State.h"
#include "core/Format.h"
#include <stdio.h>
#include <math.h>

static void render(lv_obj_t* root) {
    const WeatherData& w = state::weather();
    if (!w.valid) return;

    // Humidity Card
    char humValBuf[16], dewBuf[16], humCapBuf[32];
    snprintf(humValBuf, sizeof(humValBuf), "%.0f%%", w.humidity);
    fmt::temperature(dewBuf, sizeof(dewBuf), w.dewPoint);
    snprintf(humCapBuf, sizeof(humCapBuf), "Dew %s", dewBuf);
    ui::metricCard(root, 10, 57, layout::CARD_W_HALF, 73, "HUMIDITY", humValBuf, humCapBuf, CLR_BLUE);

    // Clouds Card
    char cloudValBuf[16], cloudCapBuf[32];
    snprintf(cloudValBuf, sizeof(cloudValBuf), "%.0f%%", w.cloudCover);
    if (isnan(w.visibility)) {
        snprintf(cloudCapBuf, sizeof(cloudCapBuf), "Vis --");
    } else {
        snprintf(cloudCapBuf, sizeof(cloudCapBuf), "Vis %.1f km", w.visibility / 1000.0f);
    }
    ui::metricCard(root, 125, 57, layout::CARD_W_HALF, 73, "CLOUDS", cloudValBuf, cloudCapBuf, CLR_CYAN);

    // Pressure Card
    char pressValBuf[16], pressCapBuf[64];
    fmt::pressure(pressValBuf, sizeof(pressValBuf), w.pressure);

    const char* trend = "Stable";
    if (w.pressureDelta6h > 2.0f) trend = "Rising";
    else if (w.pressureDelta6h < -2.0f) trend = "Falling";

    snprintf(pressCapBuf, sizeof(pressCapBuf), "%s  3h %.1f  6h %.1f",
             trend, w.pressureDelta3h, w.pressureDelta6h);

    ui::metricCard(root, 10, 141, layout::CARD_W_FULL, 66, "PRESSURE", pressValBuf, pressCapBuf, CLR_PURPLE);

    // Pressure Chart
    constexpr int COUNT = 12;
    float pressValues[COUNT];
    int count = min(COUNT, w.hourlyCount);

    for (int i = 0; i < count; i++) {
        pressValues[i] = w.hourly[i].pressure;
    }

    ui::chart(root, 10, 220, 220, 66, pressValues, count, CLR_PURPLE);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_ATMOSPHERE,
    { "ATMOSPHERE", CLR_ATMOS_TOP, render }
);
