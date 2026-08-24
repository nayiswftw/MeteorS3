#include "src/ui/Screen.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include "src/core/State.h"
#include "src/core/Format.h"
#include <stdio.h>

static void render(lv_obj_t* root) {
    const AirData& a = state::air();
    if (!a.valid) return;

    lv_color_t aqiClr = ui::aqiColor(a.usAqi);

    // Large Center AQI Gauge
    char aqiCenterBuf[16], aqiCapBuf[32];
    snprintf(aqiCenterBuf, sizeof(aqiCenterBuf), "%d", a.usAqi);
    fmt::aqiCategory(aqiCapBuf, sizeof(aqiCapBuf), a.usAqi);
    ui::gauge(root, 60, 52, 120, a.usAqi, 0, 300, aqiCenterBuf, aqiCapBuf, aqiClr);

    // Pollutant Cards
    char pm25Buf[16], pm10Buf[16], ozoneBuf[16], no2Buf[16];
    snprintf(pm25Buf, sizeof(pm25Buf), "%.1f", a.pm25);
    snprintf(pm10Buf, sizeof(pm10Buf), "%.1f", a.pm10);
    snprintf(ozoneBuf, sizeof(ozoneBuf), "%.0f", a.ozone);
    snprintf(no2Buf, sizeof(no2Buf), "%.0f", a.no2);

    ui::metricCard(root, 10, 185, layout::CARD_W_HALF, 48, "PM2.5", pm25Buf, "ug/m3", aqiClr);
    ui::metricCard(root, 125, 185, layout::CARD_W_HALF, 48, "PM10", pm10Buf, "ug/m3", CLR_ORANGE);
    ui::metricCard(root, 10, 242, layout::CARD_W_HALF, 48, "OZONE", ozoneBuf, "ug/m3", CLR_CYAN);
    ui::metricCard(root, 125, 242, layout::CARD_W_HALF, 48, "NO2", no2Buf, "ug/m3", CLR_PURPLE);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_AIR,
    { "AIR QUALITY", CLR_AIR_TOP, render }
);
