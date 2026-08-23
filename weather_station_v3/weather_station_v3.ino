/*
  ================================================================
   WEATHER STATION V3.1 "AURA"
   Waveshare ESP32-S3-LCD-2
   Arduino IDE | LVGL 9
  ================================================================

  This version deliberately uses LVGL as a graphics engine:
    - atmospheric gradients
    - vector weather scenes
    - real lv_chart graphs
    - arcs and gauges
    - layered surfaces
    - page-specific compositions

  It is not a Xiaomi clone. The visual hierarchy is inspired by
  polished phone/watch weather apps and redesigned for 240x320.

  Libraries:
    ArduinoJson 7.x
    Arduino_GFX_Library
    LVGL 9.x
*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <SPI.h>
#include <SD.h>
#include <time.h>
#include <math.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>

#include "Config.h"
#include "Models.h"

// ============================================================
// STATE
// ============================================================

WeatherData weather;
AirData air;
AlertItem alerts[8];
int alertCount = 0;

Preferences prefs;

bool wifiConnected = false;
bool sdAvailable = false;

uint32_t lastWifiAttempt = 0;
uint32_t lastWeatherUpdate = 0;
uint32_t lastAirUpdate = 0;
uint32_t lastHistoryWrite = 0;
uint32_t lastUiRefresh = 0;
uint32_t lastPageChange = 0;

enum Page {
  PAGE_HOME,
  PAGE_HOURLY,
  PAGE_WEEK,
  PAGE_DETAILS,
  PAGE_AIR,
  PAGE_SUN,
  PAGE_ALERTS,
  PAGE_SYSTEM,
  PAGE_COUNT
};

Page currentPage = PAGE_HOME;

void drawPage();
bool updateWeather();
bool updateAir();
void saveCache();

// ============================================================
// DISPLAY
// ============================================================

Arduino_DataBus *lcdBus =
  new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, LCD_MISO);

Arduino_GFX *gfx =
  new Arduino_ST7789(lcdBus, LCD_RST, 0, true, LCD_WIDTH, LCD_HEIGHT);

lv_display_t *display = nullptr;
lv_obj_t *root = nullptr;

uint8_t *lvBuffer1 = nullptr;
uint8_t *lvBuffer2 = nullptr;

constexpr int LV_BUFFER_LINES = 40;

// ============================================================
// PALETTE
// ============================================================

#define COL_BG0 lv_color_hex(0x061019)
#define COL_BG1 lv_color_hex(0x0B2532)
#define COL_PANEL lv_color_hex(0x102431)
#define COL_PANEL2 lv_color_hex(0x173442)

#define COL_TEXT lv_color_hex(0xF5F8FA)
#define COL_MUTED lv_color_hex(0x95A7B0)
#define COL_DIM lv_color_hex(0x5E717C)

#define COL_SKY lv_color_hex(0x66D9EC)
#define COL_BLUE lv_color_hex(0x75AFFF)
#define COL_GREEN lv_color_hex(0x72DBA0)
#define COL_YELLOW lv_color_hex(0xFFD57A)
#define COL_ORANGE lv_color_hex(0xFFAA67)
#define COL_RED lv_color_hex(0xFF7F83)
#define COL_PURPLE lv_color_hex(0xB9A2FF)

// ============================================================
// FORMATTING
// ============================================================

String tempText(float c, unsigned int decimals = 0) {
  if (isnan(c)) return "--";

  if (temperatureUnit == TEMP_F) {
    return String(c * 9.0f / 5.0f + 32.0f, decimals) + " F";
  }

  return String(c, decimals) + " C";
}

String windText(float kmh, unsigned int decimals = 0) {
  if (isnan(kmh)) return "--";

  if (windUnit == WIND_MPH) return String(kmh * 0.621371f, decimals) + " mph";
  if (windUnit == WIND_MS) return String(kmh / 3.6f, decimals) + " m/s";

  return String(kmh, decimals) + " km/h";
}

String pressureText(float hpa, unsigned int decimals = 0) {
  if (isnan(hpa)) return "--";

  if (pressureUnit == PRESS_INHG) {
    return String(hpa * 0.029529983f, decimals) + " inHg";
  }

  return String(hpa, decimals) + " hPa";
}

String valueText(float value, unsigned int decimals = 0) {
  return isnan(value) ? "--" : String(value, decimals);
}

String clockText() {
  struct tm t;
  if (!getLocalTime(&t, 20)) return "--:--";

  char buffer[6];
  strftime(buffer, sizeof(buffer), "%H:%M", &t);
  return String(buffer);
}

String dateText() {
  struct tm t;
  if (!getLocalTime(&t, 20)) return "--- -- ---";

  char buffer[20];
  strftime(buffer, sizeof(buffer), "%a, %d %b", &t);
  return String(buffer);
}

String weatherName(int code) {
  if (code == 0) return "Clear";
  if (code == 1) return "Mostly clear";
  if (code == 2) return "Partly cloudy";
  if (code == 3) return "Overcast";
  if (code == 45 || code == 48) return "Fog";
  if (code >= 51 && code <= 57) return "Drizzle";
  if (code >= 61 && code <= 67) return "Rain";
  if (code >= 71 && code <= 77) return "Snow";
  if (code >= 80 && code <= 82) return "Showers";
  if (code >= 85 && code <= 86) return "Snow showers";
  if (code >= 95) return "Thunderstorm";
  return "Weather";
}

String directionText(float degrees) {
  if (isnan(degrees)) return "--";

  static const char *directions[] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
  };

  int index = (int)((degrees + 11.25f) / 22.5f) % 16;
  return directions[index];
}

String ageText(time_t timestamp) {
  if (timestamp <= 0) return "cached";

  time_t now = time(nullptr);
  if (now <= 0 || now < timestamp) return "cached";

  long minutes = (long)((now - timestamp) / 60);

  if (minutes < 1) return "live";
  if (minutes < 60) return String(minutes) + "m ago";

  return String(minutes / 60) + "h ago";
}

String pressureTrend() {
  if (isnan(weather.pressureDelta6h)) return "stable";
  if (weather.pressureDelta6h > 2.0f) return "rising";
  if (weather.pressureDelta6h < -2.0f) return "falling";
  return "stable";
}

String aqiName(int aqi) {
  if (aqi < 0) return "--";
  if (aqi <= 50) return "Good";
  if (aqi <= 100) return "Moderate";
  if (aqi <= 150) return "Sensitive";
  if (aqi <= 200) return "Unhealthy";
  if (aqi <= 300) return "Very unhealthy";
  return "Hazardous";
}

lv_color_t aqiColor(int aqi) {
  if (aqi < 0) return COL_MUTED;
  if (aqi <= 50) return COL_GREEN;
  if (aqi <= 100) return COL_YELLOW;
  if (aqi <= 150) return COL_ORANGE;
  return COL_RED;
}

// ============================================================
// BATTERY
// ============================================================

float batteryVoltage() {
  uint32_t total = 0;

  for (int i = 0; i < 8; i++) {
    total += analogRead(BATTERY_ADC);
    delayMicroseconds(80);
  }

  float raw = total / 8.0f;
  return (raw * 3.3f / 4095.0f) * 2.0f;
}

int batteryPercent() {
  float v = batteryVoltage();

  if (v >= 4.20f) return 100;
  if (v >= 4.00f) return map((int)(v * 1000), 4000, 4200, 80, 100);
  if (v >= 3.80f) return map((int)(v * 1000), 3800, 4000, 45, 80);
  if (v >= 3.65f) return map((int)(v * 1000), 3650, 3800, 15, 45);
  if (v >= 3.40f) return map((int)(v * 1000), 3400, 3650, 0, 15);

  return 0;
}

// ============================================================
// UI CORE
// ============================================================

void stylePlain(lv_obj_t *object) {
  lv_obj_set_style_border_width(object, 0, 0);
  lv_obj_set_style_pad_all(object, 0, 0);
  lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *panel(int x, int y, int w, int h,
                lv_color_t color = COL_PANEL,
                int radius = 16) {
  lv_obj_t *object = lv_obj_create(root);

  lv_obj_set_pos(object, x, y);
  lv_obj_set_size(object, w, h);

  stylePlain(object);

  lv_obj_set_style_bg_color(object, color, 0);
  lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(object, radius, 0);

  return object;
}

lv_obj_t *textOn(lv_obj_t *parent, const String &textValue,
                 int x, int y, int w, int h,
                 const lv_font_t *font = &lv_font_montserrat_14,
                 lv_color_t color = COL_TEXT,
                 lv_text_align_t alignment = LV_TEXT_ALIGN_LEFT) {
  lv_obj_t *object = lv_label_create(parent);

  lv_label_set_text(object, textValue.c_str());
  lv_obj_set_pos(object, x, y);
  lv_obj_set_size(object, w, h);

  lv_obj_set_style_text_font(object, font, 0);
  lv_obj_set_style_text_color(object, color, 0);
  lv_obj_set_style_text_align(object, alignment, 0);

  lv_label_set_long_mode(object, LV_LABEL_LONG_CLIP);

  return object;
}

lv_obj_t *text(const String &value,
               int x, int y, int w, int h,
               const lv_font_t *font = &lv_font_montserrat_14,
               lv_color_t color = COL_TEXT,
               lv_text_align_t alignment = LV_TEXT_ALIGN_LEFT) {
  return textOn(root, value, x, y, w, h, font, color, alignment);
}

void setGradientBackground(lv_color_t top, lv_color_t bottom) {
  lv_obj_set_style_bg_color(root, top, 0);
  lv_obj_set_style_bg_grad_color(root, bottom, 0);
  lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
}

void clearUi() {
  if (root) lv_obj_clean(root);
}

void circle(int x, int y, int diameter, lv_color_t color,
            lv_opa_t opacity = LV_OPA_COVER) {
  lv_obj_t *object = panel(x, y, diameter, diameter, color, LV_RADIUS_CIRCLE);
  lv_obj_set_style_bg_opa(object, opacity, 0);
}

void lineSegment(int x1, int y1, int x2, int y2,
                 lv_color_t color, int width = 2) {
  lv_obj_t *line = lv_line_create(root);

  static lv_point_precise_t points[32][2];
  static uint8_t next = 0;

  lv_point_precise_t *pair = points[next++ % 32];

  pair[0].x = x1;
  pair[0].y = y1;
  pair[1].x = x2;
  pair[1].y = y2;

  lv_line_set_points(line, pair, 2);

  lv_obj_set_style_line_width(line, width, 0);
  lv_obj_set_style_line_color(line, color, 0);
  lv_obj_set_style_line_rounded(line, true, 0);
}

// ============================================================
// VECTOR WEATHER ART
// ============================================================

void drawSunArt(int cx, int cy, int scale) {
  int radius = 12 * scale;

  circle(cx - radius, cy - radius, radius * 2, COL_YELLOW);

  for (int i = 0; i < 8; i++) {
    float angle = i * PI / 4.0f;

    int x1 = cx + cos(angle) * (radius + 5);
    int y1 = cy + sin(angle) * (radius + 5);
    int x2 = cx + cos(angle) * (radius + 12);
    int y2 = cy + sin(angle) * (radius + 12);

    lineSegment(x1, y1, x2, y2, COL_YELLOW, 2);
  }
}

void drawCloudArt(int x, int y, int scale, lv_color_t color = COL_TEXT) {
  circle(x + 7 * scale, y + 10 * scale, 18 * scale, color);
  circle(x + 20 * scale, y + 3 * scale, 24 * scale, color);
  circle(x + 36 * scale, y + 10 * scale, 18 * scale, color);

  lv_obj_t *base =
    panel(x + 4 * scale, y + 14 * scale,
          42 * scale, 13 * scale,
          color, 6 * scale);
}

void drawRainArt(int x, int y, int scale) {
  drawCloudArt(x, y, scale, lv_color_hex(0xD8E7EE));

  for (int i = 0; i < 3; i++) {
    int dx = x + (12 + i * 12) * scale;

    lineSegment(dx, y + 32 * scale,
                dx - 3 * scale, y + 41 * scale,
                COL_SKY, 2 * scale);
  }
}

void drawStormArt(int x, int y, int scale) {
  drawCloudArt(x, y, scale, lv_color_hex(0xAABAC5));

  lv_point_precise_t bolt[4] = {
    { (lv_coord_t)(x + 25 * scale), (lv_coord_t)(y + 27 * scale) },
    { (lv_coord_t)(x + 18 * scale), (lv_coord_t)(y + 39 * scale) },
    { (lv_coord_t)(x + 25 * scale), (lv_coord_t)(y + 39 * scale) },
    { (lv_coord_t)(x + 19 * scale), (lv_coord_t)(y + 50 * scale) }
  };

  lv_obj_t *line = lv_line_create(root);
  lv_line_set_points(line, bolt, 4);
  lv_obj_set_style_line_width(line, 3 * scale, 0);
  lv_obj_set_style_line_color(line, COL_YELLOW, 0);
}

void drawWeatherArt(int code, bool isDay, int x, int y, int scale = 1) {
  if (code == 0) {
    drawSunArt(x + 25 * scale, y + 25 * scale, scale);
    return;
  }

  if (code == 1 || code == 2) {
    drawSunArt(x + 15 * scale, y + 12 * scale, scale);
    drawCloudArt(x + 8 * scale, y + 18 * scale, scale);
    return;
  }

  if (code <= 3 || code == 45 || code == 48) {
    drawCloudArt(x, y + 10 * scale, scale);
    return;
  }

  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
    drawRainArt(x, y, scale);
    return;
  }

  if (code >= 95) {
    drawStormArt(x, y, scale);
    return;
  }

  drawCloudArt(x, y + 10 * scale, scale);
}

// ============================================================
// HEADER + NAV
// ============================================================

void drawHeader(const String &section = "") {
  text(LOCATION_NAME, 10, 7, 105, 20,
       &lv_font_montserrat_14, COL_TEXT);

  text(clockText(), 126, 7, 52, 20,
       &lv_font_montserrat_14, COL_MUTED,
       LV_TEXT_ALIGN_RIGHT);

  circle(188, 12, 7, wifiConnected ? COL_GREEN : COL_RED);

  int battery = batteryPercent();

  text(String(battery) + "%", 199, 7, 32, 20,
       &lv_font_montserrat_14,
       battery <= 20 ? COL_RED : COL_MUTED,
       LV_TEXT_ALIGN_RIGHT);

  if (section.length()) {
    text(section, 10, 33, 220, 16,
         &lv_font_montserrat_14, COL_DIM);
  }
}

void drawNavDots() {
  int startX = 72;

  for (int i = 0; i < PAGE_COUNT; i++) {
    int diameter = i == currentPage ? 6 : 4;

    circle(
      startX + i * 14,
      i == currentPage ? 305 : 306,
      diameter,
      i == currentPage ? COL_SKY : COL_DIM);
  }
}

// ============================================================
// INSIGHTS
// ============================================================

String insightText() {
  if (!weather.valid) return "Waiting for weather";

  int rainChance =
    weather.hourlyCount ? weather.hourly[0].rainChance : 0;

  if (weather.weatherCode >= 95) return "Thunderstorms possible";
  if (rainChance >= 70) return "Umbrella recommended";
  if (weather.uv >= 8) return "Very high UV outdoors";
  if (weather.uv >= 6) return "High UV outdoors";
  if (weather.gust >= 55) return "Strong gusts possible";
  if (weather.temperature >= 38) return "Extreme heat";
  if (air.valid && air.usAqi >= 151) return "Poor air quality";
  if (weather.pressureDelta6h < -2.0f) return "Pressure is falling";

  return "Comfortable weather window";
}

String rainSignalText() {
  if (!weather.valid || !weather.hourlyCount) return "Forecast unavailable";

  for (int i = 0; i < weather.hourlyCount; i++) {
    if (weather.hourly[i].rainChance >= 60) {
      if (i == 0) return "Rain likely now";
      if (i == 1) return "Rain in ~1 hour";
      return "Rain in ~" + String(i) + " hours";
    }
  }

  return "No rain signal";
}

// ============================================================
// HOME — ATMOSPHERIC HERO
// ============================================================

void drawHomePage() {
  clearUi();

  if (weather.valid && weather.isDay) {
    setGradientBackground(lv_color_hex(0x0A2A39), COL_BG0);
  } else {
    setGradientBackground(lv_color_hex(0x10162C), COL_BG0);
  }

  drawHeader();

  // Atmospheric glow behind the weather art.
  circle(167, 55, 85,
         weather.isDay ? lv_color_hex(0x294C54) : lv_color_hex(0x28254C),
         LV_OPA_40);

  if (!weather.valid) {
    text("--", 12, 82, 140, 46,
         &lv_font_montserrat_32, COL_TEXT);

    text("Waiting for weather", 12, 133, 210, 20,
         &lv_font_montserrat_14, COL_MUTED);

    drawNavDots();
    return;
  }

  drawWeatherArt(weather.weatherCode, weather.isDay, 155, 62, 1);

  text(tempText(weather.temperature),
       10, 70, 145, 45,
       &lv_font_montserrat_32, COL_TEXT);

  text(weatherName(weather.weatherCode),
       12, 116, 140, 20,
       &lv_font_montserrat_14, COL_TEXT);

  String secondary = "Feels " + tempText(weather.apparent);

  if (weather.dailyCount) {
    secondary += "  H " + tempText(weather.daily[0].high);
    secondary += "  L " + tempText(weather.daily[0].low);
  }

  text(secondary, 12, 138, 218, 18,
       &lv_font_montserrat_14, COL_MUTED);

  lv_obj_t *insight =
    panel(10, 166, 220, 36, COL_PANEL2, 18);

  lv_obj_set_style_bg_opa(insight, LV_OPA_70, 0);

  textOn(insight, insightText(),
         12, 9, 196, 18,
         &lv_font_montserrat_14, COL_TEXT);

  // Lower glass-like information strip.
  lv_obj_t *bottom =
    panel(10, 213, 220, 77, COL_PANEL, 18);

  lv_obj_set_style_bg_opa(bottom, LV_OPA_80, 0);

  int rainChance = weather.hourlyCount ? weather.hourly[0].rainChance : 0;

  textOn(bottom, "RAIN", 12, 10, 50, 15,
         &lv_font_montserrat_14, COL_MUTED);

  textOn(bottom, String(rainChance) + "%",
         12, 31, 55, 23,
         &lv_font_montserrat_14,
         rainChance >= 50 ? COL_SKY : COL_TEXT);

  textOn(bottom, rainSignalText(),
         12, 55, 100, 15,
         &lv_font_montserrat_14, COL_DIM);

  textOn(bottom, "AQI", 125, 10, 45, 15,
         &lv_font_montserrat_14, COL_MUTED);

  textOn(bottom,
         air.valid ? String(air.usAqi) : "--",
         125, 31, 45, 23,
         &lv_font_montserrat_14,
         air.valid ? aqiColor(air.usAqi) : COL_MUTED);

  textOn(bottom,
         air.valid ? aqiName(air.usAqi) : "Waiting",
         125, 55, 85, 15,
         &lv_font_montserrat_14, COL_DIM);

  drawNavDots();
}

// ============================================================
// REAL LVGL CHART
// ============================================================

lv_obj_t *createTemperatureChart(int x, int y, int w, int h,
                                 int pointCount = 12) {
  if (!weather.valid || weather.hourlyCount < 2) return nullptr;

  int count = min(pointCount, weather.hourlyCount);

  float low = 999;
  float high = -999;

  for (int i = 0; i < count; i++) {
    low = min(low, weather.hourly[i].temperature);
    high = max(high, weather.hourly[i].temperature);
  }

  int axisLow = floor(low) - 1;
  int axisHigh = ceil(high) + 1;

  if (axisHigh <= axisLow) axisHigh = axisLow + 2;

  lv_obj_t *chart = lv_chart_create(root);

  lv_obj_set_pos(chart, x, y);
  lv_obj_set_size(chart, w, h);

  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, count);
  lv_chart_set_axis_range(
    chart,
    LV_CHART_AXIS_PRIMARY_Y,
    axisLow * 10,
    axisHigh * 10);

  lv_chart_set_div_line_count(chart, 3, 0);

  lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(chart, 0, LV_PART_MAIN);

  lv_obj_set_style_line_color(
    chart,
    lv_color_hex(0x294451),
    LV_PART_MAIN);

  lv_obj_set_style_line_opa(chart, LV_OPA_30, LV_PART_MAIN);
  lv_obj_set_style_pad_all(chart, 4, 0);

  lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);
  lv_obj_set_style_width(chart, 6, LV_PART_INDICATOR);
  lv_obj_set_style_height(chart, 6, LV_PART_INDICATOR);
  lv_obj_set_style_radius(chart, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);

  lv_chart_series_t *series =
    lv_chart_add_series(
      chart,
      COL_SKY,
      LV_CHART_AXIS_PRIMARY_Y);

  for (int i = 0; i < count; i++) {
    lv_chart_set_next_value(
      chart,
      series,
      (int32_t)round(weather.hourly[i].temperature * 10.0f));
  }

  return chart;
}

// ============================================================
// HOURLY — GRAPH FIRST
// ============================================================

void drawHourlyPage() {
  clearUi();
  setGradientBackground(COL_BG1, COL_BG0);
  drawHeader("HOURLY");

  if (!weather.valid || !weather.hourlyCount) {
    text("No hourly forecast", 10, 140, 220, 20,
         &lv_font_montserrat_14, COL_MUTED, LV_TEXT_ALIGN_CENTER);
    drawNavDots();
    return;
  }

  text("Temperature",
       10, 57, 110, 18,
       &lv_font_montserrat_14, COL_MUTED);

  createTemperatureChart(10, 82, 220, 86, 12);

  // Chart time labels.
  for (int i = 0; i < 4; i++) {
    int index = i * 3;
    int x = 10 + i * 69;

    if (index >= weather.hourlyCount) break;

    text(
      i == 0 ? "NOW" : String(weather.hourly[index].time),
      x, 172, 48, 16,
      &lv_font_montserrat_14,
      i == 0 ? COL_SKY : COL_MUTED,
      LV_TEXT_ALIGN_CENTER);
  }

  lv_obj_t *forecast =
    panel(10, 198, 220, 91, COL_PANEL, 18);

  for (int i = 0; i < 4; i++) {
    int index = i * 2;
    if (index >= weather.hourlyCount) break;

    HourData &hour = weather.hourly[index];
    int x = i * 55;

    textOn(
      forecast,
      tempText(hour.temperature),
      x, 10, 55, 18,
      &lv_font_montserrat_14,
      COL_TEXT,
      LV_TEXT_ALIGN_CENTER);

    textOn(
      forecast,
      String(hour.rainChance) + "%",
      x, 36, 55, 17,
      &lv_font_montserrat_14,
      hour.rainChance >= 50 ? COL_SKY : COL_DIM,
      LV_TEXT_ALIGN_CENTER);

    textOn(
      forecast,
      windText(hour.wind),
      x, 61, 55, 17,
      &lv_font_montserrat_14,
      COL_MUTED,
      LV_TEXT_ALIGN_CENTER);
  }

  drawNavDots();
}

// ============================================================
// WEEK — RANGE BARS
// ============================================================

void drawWeekPage() {
  clearUi();
  setGradientBackground(lv_color_hex(0x102331), COL_BG0);
  drawHeader("7 DAY");

  if (!weather.valid || !weather.dailyCount) {
    text("No daily forecast", 10, 140, 220, 20,
         &lv_font_montserrat_14, COL_MUTED, LV_TEXT_ALIGN_CENTER);
    drawNavDots();
    return;
  }

  int days = min(weather.dailyCount, 7);

  float globalLow = 999;
  float globalHigh = -999;

  for (int i = 0; i < days; i++) {
    globalLow = min(globalLow, weather.daily[i].low);
    globalHigh = max(globalHigh, weather.daily[i].high);
  }

  if (globalHigh - globalLow < 1) globalHigh = globalLow + 1;

  int y = 58;

  for (int i = 0; i < days; i++) {
    DayData &day = weather.daily[i];

    String date =
      i == 0 ? "TODAY" : String(day.date + 5);

    text(date, 10, y, 48, 17,
         &lv_font_montserrat_14,
         i == 0 ? COL_SKY : COL_MUTED);

    text(tempText(day.low), 60, y, 42, 17,
         &lv_font_montserrat_14, COL_MUTED,
         LV_TEXT_ALIGN_RIGHT);

    int trackX = 111;
    int trackW = 73;

    panel(trackX, y + 7, trackW, 5, COL_PANEL2, 3);

    int lowX =
      trackX + (int)((day.low - globalLow) / (globalHigh - globalLow) * trackW);

    int highX =
      trackX + (int)((day.high - globalLow) / (globalHigh - globalLow) * trackW);

    int rangeW = max(5, highX - lowX);

    panel(
      lowX,
      y + 7,
      rangeW,
      5,
      i == 0 ? COL_YELLOW : COL_ORANGE,
      3);

    text(tempText(day.high), 190, y, 40, 17,
         &lv_font_montserrat_14, COL_TEXT,
         LV_TEXT_ALIGN_RIGHT);

    text(String(day.rainChance) + "%",
         60, y + 19, 42, 14,
         &lv_font_montserrat_14,
         day.rainChance >= 50 ? COL_SKY : COL_DIM,
         LV_TEXT_ALIGN_RIGHT);

    y += 35;
  }

  drawNavDots();
}

// ============================================================
// GAUGE
// ============================================================

void gauge(int x, int y, int diameter,
           int value, int minValue, int maxValue,
           lv_color_t color,
           const String &center,
           const String &caption) {
  lv_obj_t *arc = lv_arc_create(root);

  lv_obj_set_pos(arc, x, y);
  lv_obj_set_size(arc, diameter, diameter);

  lv_arc_set_range(arc, minValue, maxValue);
  lv_arc_set_value(arc, constrain(value, minValue, maxValue));
  lv_arc_set_rotation(arc, 135);
  lv_arc_set_bg_angles(arc, 0, 270);

  lv_obj_remove_style(arc, NULL, LV_PART_KNOB);

  lv_obj_set_style_arc_width(arc, 7, LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc, COL_PANEL2, LV_PART_MAIN);

  lv_obj_set_style_arc_width(arc, 7, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);

  text(center,
       x + 8, y + diameter / 2 - 13,
       diameter - 16, 20,
       &lv_font_montserrat_14,
       COL_TEXT,
       LV_TEXT_ALIGN_CENTER);

  text(caption,
       x + 4, y + diameter - 19,
       diameter - 8, 16,
       &lv_font_montserrat_14,
       COL_MUTED,
       LV_TEXT_ALIGN_CENTER);
}

// ============================================================
// DETAILS — INSTRUMENT PANEL
// ============================================================

void drawDetailsPage() {
  clearUi();
  setGradientBackground(lv_color_hex(0x0B202B), COL_BG0);
  drawHeader("CONDITIONS");

  if (!weather.valid) {
    drawNavDots();
    return;
  }

  gauge(
    10, 59, 100,
    (int)weather.humidity,
    0, 100,
    COL_BLUE,
    valueText(weather.humidity) + "%",
    "Humidity");

  gauge(
    130, 59, 100,
    (int)constrain(weather.uv * 10, 0.0f, 110.0f),
    0, 110,
    weather.uv >= 6 ? COL_YELLOW : COL_GREEN,
    valueText(weather.uv, 1),
    "UV index");

  lv_obj_t *wind = panel(10, 174, 105, 111, COL_PANEL, 18);

  textOn(wind, "WIND", 10, 9, 85, 16,
         &lv_font_montserrat_14, COL_MUTED);

  textOn(wind, directionText(weather.direction),
         10, 31, 85, 24,
         &lv_font_montserrat_14, COL_SKY);

  textOn(wind, windText(weather.wind),
         10, 58, 85, 18,
         &lv_font_montserrat_14, COL_TEXT);

  textOn(wind, "gust " + windText(weather.gust),
         10, 84, 85, 16,
         &lv_font_montserrat_14, COL_DIM);

  lv_obj_t *pressure = panel(125, 174, 105, 111, COL_PANEL, 18);

  textOn(pressure, "PRESSURE", 10, 9, 85, 16,
         &lv_font_montserrat_14, COL_MUTED);

  textOn(pressure, pressureText(weather.pressure),
         10, 31, 85, 24,
         &lv_font_montserrat_14, COL_TEXT);

  textOn(pressure, pressureTrend(),
         10, 58, 85, 18,
         &lv_font_montserrat_14,
         weather.pressureDelta6h < -2 ? COL_ORANGE : COL_SKY);

  textOn(
    pressure,
    isnan(weather.visibility)
      ? "visibility --"
      : "vis " + String(weather.visibility / 1000.0f, 1) + " km",
    10, 84, 85, 16,
    &lv_font_montserrat_14, COL_DIM);

  drawNavDots();
}

// ============================================================
// AIR — HERO + RADIAL AQI
// ============================================================

void drawAirPage() {
  clearUi();
  setGradientBackground(lv_color_hex(0x11231E), COL_BG0);
  drawHeader("AIR QUALITY");

  if (!air.valid) {
    text("Waiting for AQI", 10, 140, 220, 20,
         &lv_font_montserrat_14, COL_MUTED,
         LV_TEXT_ALIGN_CENTER);
    drawNavDots();
    return;
  }

  lv_color_t color = aqiColor(air.usAqi);

  gauge(
    60, 55, 120,
    constrain(air.usAqi, 0, 300),
    0, 300,
    color,
    String(air.usAqi),
    aqiName(air.usAqi));

  lv_obj_t *grid = panel(10, 188, 220, 98, COL_PANEL, 18);

  const char *names[] = { "PM2.5", "PM10", "O3", "NO2" };

  String values[] = {
    valueText(air.pm25, 1),
    valueText(air.pm10, 1),
    valueText(air.ozone),
    valueText(air.no2)
  };

  lv_color_t colors[] = {
    color, COL_ORANGE, COL_SKY, COL_PURPLE
  };

  for (int i = 0; i < 4; i++) {
    int x = (i % 2) * 110;
    int y = (i / 2) * 48;

    textOn(grid, names[i],
           x + 10, y + 8, 50, 15,
           &lv_font_montserrat_14, COL_MUTED);

    textOn(grid, values[i],
           x + 60, y + 8, 39, 18,
           &lv_font_montserrat_14, colors[i],
           LV_TEXT_ALIGN_RIGHT);

    textOn(grid, "ug/m3",
           x + 10, y + 27, 89, 14,
           &lv_font_montserrat_14, COL_DIM,
           LV_TEXT_ALIGN_RIGHT);
  }

  drawNavDots();
}

// ============================================================
// MOON + SUN
// ============================================================

String moonPhase() {
  struct tm t;
  if (!getLocalTime(&t, 20)) return "Unknown";

  int year = t.tm_year + 1900;
  int month = t.tm_mon + 1;
  int day = t.tm_mday;

  if (month < 3) {
    year--;
    month += 12;
  }

  long days =
    365L * year + year / 4 - year / 100 + year / 400 + (153L * (month + 1)) / 5 + day - 730551L;

  double phase = fmod(days + 4.867, 29.530588853);
  if (phase < 0) phase += 29.530588853;

  double f = phase / 29.530588853;

  if (f < 0.03) return "New moon";
  if (f < 0.22) return "Waxing crescent";
  if (f < 0.28) return "First quarter";
  if (f < 0.47) return "Waxing gibbous";
  if (f < 0.53) return "Full moon";
  if (f < 0.72) return "Waning gibbous";
  if (f < 0.78) return "Last quarter";
  if (f < 0.97) return "Waning crescent";

  return "New moon";
}

void drawSunPage() {
  clearUi();

  setGradientBackground(
    weather.isDay ? lv_color_hex(0x30271B) : lv_color_hex(0x17172D),
    COL_BG0);

  drawHeader("SUN & MOON");

  if (!weather.valid) {
    drawNavDots();
    return;
  }

  // Horizon composition.
  circle(
    83, 68, 74,
    weather.isDay ? COL_YELLOW : COL_PURPLE,
    LV_OPA_80);

  panel(0, 124, 240, 2, COL_PANEL2, 0);

  text(
    weather.isDay ? "DAYLIGHT" : "NIGHT",
    10, 147, 90, 18,
    &lv_font_montserrat_14,
    weather.isDay ? COL_YELLOW : COL_PURPLE);

  String sunrise =
    weather.dailyCount ? String(weather.daily[0].sunrise) : "--:--";

  String sunset =
    weather.dailyCount ? String(weather.daily[0].sunset) : "--:--";

  lv_obj_t *times = panel(10, 178, 220, 55, COL_PANEL, 18);

  textOn(times, "Sunrise", 12, 8, 70, 16,
         &lv_font_montserrat_14, COL_MUTED);

  textOn(times, sunrise, 12, 29, 70, 18,
         &lv_font_montserrat_14, COL_YELLOW);

  textOn(times, "Sunset", 132, 8, 70, 16,
         &lv_font_montserrat_14, COL_MUTED);

  textOn(times, sunset, 132, 29, 70, 18,
         &lv_font_montserrat_14, COL_ORANGE);

  lv_obj_t *moon = panel(10, 244, 220, 42, COL_PANEL2, 18);

  textOn(moon, "Moon", 12, 12, 50, 18,
         &lv_font_montserrat_14, COL_MUTED);

  textOn(moon, moonPhase(), 70, 12, 138, 18,
         &lv_font_montserrat_14, COL_PURPLE,
         LV_TEXT_ALIGN_RIGHT);

  drawNavDots();
}

// ============================================================
// ALERTS
// ============================================================

void addAlert(const String &value, lv_color_t color) {
  if (alertCount >= 8) return;

  alerts[alertCount].text = value;
  alerts[alertCount].color = color;
  alertCount++;
}

void buildAlerts() {
  alertCount = 0;

  if (!weather.valid) {
    addAlert("Weather data unavailable", COL_RED);
    return;
  }

  if (weather.weatherCode >= 95)
    addAlert("Thunderstorm conditions", COL_PURPLE);

  if (weather.temperature >= 40)
    addAlert("Extreme heat", COL_RED);
  else if (weather.temperature >= 35)
    addAlert("High temperature", COL_ORANGE);

  if (weather.gust >= 65 || weather.wind >= 50)
    addAlert("Strong wind", COL_RED);

  if (weather.hourlyCount && weather.hourly[0].rainChance >= 70)
    addAlert("High rain probability", COL_BLUE);

  if (weather.uv >= 8)
    addAlert("Very high UV", COL_RED);
  else if (weather.uv >= 6)
    addAlert("High UV", COL_YELLOW);

  if (air.valid && air.usAqi >= 151)
    addAlert("Poor air quality", COL_RED);

  if (weather.pressureDelta6h < -4)
    addAlert("Pressure falling quickly", COL_ORANGE);

  if (!alertCount)
    addAlert("No active weather alerts", COL_GREEN);
}

void drawAlertsPage() {
  clearUi();
  setGradientBackground(lv_color_hex(0x241A20), COL_BG0);
  drawHeader("ALERT CENTER");

  buildAlerts();

  text(
    String(alertCount),
    10, 58, 60, 42,
    &lv_font_montserrat_32,
    alertCount == 1 && alerts[0].text.startsWith("No active")
      ? COL_GREEN
      : COL_YELLOW);

  text("ACTIVE",
       71, 72, 70, 16,
       &lv_font_montserrat_14, COL_MUTED);

  int y = 112;

  for (int i = 0; i < alertCount && i < 5; i++) {
    lv_obj_t *row = panel(10, y, 220, 31, COL_PANEL, 15);

    lv_obj_t *accent = lv_obj_create(row);

    lv_obj_set_pos(accent, 9, 11);
    lv_obj_set_size(accent, 9, 9);
    stylePlain(accent);

    lv_obj_set_style_bg_color(accent, alerts[i].color, 0);
    lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(accent, LV_RADIUS_CIRCLE, 0);

    textOn(row, alerts[i].text,
           28, 7, 180, 18,
           &lv_font_montserrat_14, COL_TEXT);

    y += 39;
  }

  drawNavDots();
}

// ============================================================
// SYSTEM
// ============================================================

void drawSystemPage() {
  clearUi();
  setGradientBackground(lv_color_hex(0x101D27), COL_BG0);
  drawHeader("SYSTEM");

  float voltage = batteryVoltage();

  lv_obj_t *status = panel(10, 57, 220, 52, COL_PANEL, 18);

  textOn(
    status,
    wifiConnected ? "ONLINE" : "OFFLINE",
    12, 9, 90, 18,
    &lv_font_montserrat_14,
    wifiConnected ? COL_GREEN : COL_RED);

  textOn(
    status,
    weather.valid ? ageText(weather.fetchedAt) : "no weather",
    115, 9, 93, 18,
    &lv_font_montserrat_14, COL_MUTED,
    LV_TEXT_ALIGN_RIGHT);

  textOn(
    status,
    wifiConnected ? WiFi.localIP().toString() : "No network",
    12, 29, 196, 16,
    &lv_font_montserrat_14, COL_DIM);

  gauge(
    10, 127, 95,
    batteryPercent(), 0, 100,
    batteryPercent() <= 20 ? COL_RED : COL_GREEN,
    String(batteryPercent()) + "%",
    "Battery");

  gauge(
    135, 127, 95,
    constrain(WiFi.RSSI() + 100, 0, 70), 0, 70,
    wifiConnected ? COL_SKY : COL_DIM,
    wifiConnected ? String(WiFi.RSSI()) : "--",
    "WiFi dBm");

  lv_obj_t *memory = panel(10, 238, 220, 50, COL_PANEL, 18);

  textOn(
    memory,
    "Heap  " + String(ESP.getFreeHeap() / 1024) + " KB",
    12, 8, 95, 17,
    &lv_font_montserrat_14, COL_TEXT);

  textOn(
    memory,
    psramFound()
      ? "PSRAM " + String(ESP.getFreePsram() / 1024) + " KB"
      : "PSRAM --",
    113, 8, 95, 17,
    &lv_font_montserrat_14, COL_TEXT,
    LV_TEXT_ALIGN_RIGHT);

  textOn(
    memory,
    String(voltage, 2) + " V   SD " + (sdAvailable ? "ready" : "offline"),
    12, 28, 196, 16,
    &lv_font_montserrat_14, COL_MUTED,
    LV_TEXT_ALIGN_CENTER);

  drawNavDots();
}

// ============================================================
// ROUTER
// ============================================================

void drawPage() {
  switch (currentPage) {
    case PAGE_HOME: drawHomePage(); break;
    case PAGE_HOURLY: drawHourlyPage(); break;
    case PAGE_WEEK: drawWeekPage(); break;
    case PAGE_DETAILS: drawDetailsPage(); break;
    case PAGE_AIR: drawAirPage(); break;
    case PAGE_SUN: drawSunPage(); break;
    case PAGE_ALERTS: drawAlertsPage(); break;
    case PAGE_SYSTEM: drawSystemPage(); break;

    default:
      currentPage = PAGE_HOME;
      drawHomePage();
      break;
  }

  lastPageChange = millis();
}

// ============================================================
// DISPLAY PORT
// ============================================================

void lvglFlush(lv_display_t *disp,
               const lv_area_t *area,
               uint8_t *pixelMap) {
  uint32_t width = area->x2 - area->x1 + 1;
  uint32_t height = area->y2 - area->y1 + 1;

  gfx->draw16bitRGBBitmap(
    area->x1,
    area->y1,
    (uint16_t *)pixelMap,
    width,
    height);

  lv_display_flush_ready(disp);
}

uint32_t lvglTick() {
  return millis();
}

void initializeDisplay() {
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, HIGH);

  if (!gfx->begin()) {
    Serial.println("[display] gfx begin failed");
  }

  gfx->fillScreen(RGB565_BLACK);

  lv_init();
  lv_tick_set_cb(lvglTick);

  display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);

  size_t bufferSize =
    LCD_WIDTH * LV_BUFFER_LINES * sizeof(lv_color_t);

  lvBuffer1 = (uint8_t *)heap_caps_malloc(
    bufferSize,
    MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

  lvBuffer2 = (uint8_t *)heap_caps_malloc(
    bufferSize,
    MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

  if (!lvBuffer1 || !lvBuffer2) {
    Serial.println("[display] LVGL buffers failed");

    while (true) delay(1000);
  }

  lv_display_set_buffers(
    display,
    lvBuffer1,
    lvBuffer2,
    bufferSize,
    LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_display_set_flush_cb(display, lvglFlush);

  root = lv_screen_active();

  lv_obj_set_style_bg_color(root, COL_BG0, 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

  Serial.println("[display] ready");
}

// ============================================================
// WIFI + TIME
// ============================================================

void startWiFi() {
  if (strlen(WIFI_SSID) == 0 || strcmp(WIFI_SSID, "YOUR_WIFI_NAME") == 0) {
    Serial.println("[wifi] credentials not configured");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  Serial.printf("[wifi] connecting to %s\n", WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttempt = millis();
}

void maintainWiFi() {
  bool connectedNow = WiFi.status() == WL_CONNECTED;

  if (connectedNow != wifiConnected) {
    wifiConnected = connectedNow;

    if (wifiConnected) {
      Serial.print("[wifi] connected: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("[wifi] disconnected");
    }

    drawPage();
  }

  if (!wifiConnected && millis() - lastWifiAttempt >= WIFI_RETRY_MS) {
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    lastWifiAttempt = millis();
  }
}

void setupTime() {
  configTzTime(
    TIMEZONE,
    "pool.ntp.org",
    "time.google.com",
    "time.cloudflare.com");
}

// ============================================================
// HTTP
// ============================================================

bool getJson(const String &url, JsonDocument &document) {
  if (!wifiConnected) return false;

  HTTPClient http;

  http.setConnectTimeout(10000);
  http.setTimeout(20000);
  http.useHTTP10(true);

  if (!http.begin(url)) return false;

  http.addHeader("User-Agent", "ESP32-WeatherStation-Aura");

  int responseCode = http.GET();

  if (responseCode != HTTP_CODE_OK) {
    Serial.printf("[http] %d\n", responseCode);
    http.end();
    return false;
  }

  // Weather JSON was being truncated when parsed directly from the
  // stream on this board/library combination. Read it completely first.
  String payload = http.getString();

  http.end();

  Serial.printf("[http] %u bytes\n", payload.length());

  if (!payload.length()) return false;

  DeserializationError error =
    deserializeJson(document, payload);

  if (error) {
    Serial.print("[json] ");
    Serial.println(error.c_str());
    return false;
  }

  return true;
}

// ============================================================
// WEATHER API
// ============================================================

String weatherURL() {
  String url = "https://api.open-meteo.com/v1/forecast";

  url += "?latitude=" + String(LATITUDE, 6);
  url += "&longitude=" + String(LONGITUDE, 6);

  url +=
    "&current="
    "temperature_2m,"
    "relative_humidity_2m,"
    "apparent_temperature,"
    "dew_point_2m,"
    "pressure_msl,"
    "wind_speed_10m,"
    "wind_direction_10m,"
    "wind_gusts_10m,"
    "precipitation,"
    "rain,"
    "weather_code,"
    "visibility,"
    "uv_index,"
    "is_day";

  url +=
    "&hourly="
    "temperature_2m,"
    "apparent_temperature,"
    "pressure_msl,"
    "wind_speed_10m,"
    "wind_direction_10m,"
    "wind_gusts_10m,"
    "precipitation,"
    "precipitation_probability,"
    "weather_code";

  url +=
    "&daily="
    "weather_code,"
    "temperature_2m_max,"
    "temperature_2m_min,"
    "precipitation_sum,"
    "precipitation_probability_max,"
    "sunrise,"
    "sunset";

  url += "&past_hours=6";
  url += "&forecast_days=8";
  url += "&timezone=auto";

  return url;
}

int currentHourIndex(JsonArray times, const String &currentTime) {
  if (!times.size() || currentTime.length() < 13) return 0;

  String key = currentTime.substring(0, 13);

  for (int i = 0; i < (int)times.size(); i++) {
    String item = times[i].as<String>();

    if (item.startsWith(key)) return i;
  }

  return 0;
}

bool updateWeather() {
  Serial.println("[weather] updating");

  JsonDocument document;

  if (!getJson(weatherURL(), document)) {
    Serial.println("[weather] failed");
    return false;
  }

  JsonObject current = document["current"];
  String currentTime = current["time"] | "";

  weather.temperature = current["temperature_2m"] | NAN;
  weather.apparent = current["apparent_temperature"] | NAN;
  weather.humidity = current["relative_humidity_2m"] | NAN;
  weather.dewPoint = current["dew_point_2m"] | NAN;
  weather.pressure = current["pressure_msl"] | NAN;
  weather.wind = current["wind_speed_10m"] | NAN;
  weather.direction = current["wind_direction_10m"] | NAN;
  weather.gust = current["wind_gusts_10m"] | NAN;
  weather.precipitation = current["precipitation"] | NAN;
  weather.rain = current["rain"] | NAN;
  weather.weatherCode = current["weather_code"] | -1;
  weather.visibility = current["visibility"] | NAN;
  weather.uv = current["uv_index"] | NAN;
  weather.isDay = (current["is_day"] | 1) == 1;

  JsonObject hourly = document["hourly"];
  JsonArray times = hourly["time"];

  int nowIndex = currentHourIndex(times, currentTime);

  weather.pressureDelta6h = NAN;

  if (nowIndex >= 6) {
    float oldPressure = hourly["pressure_msl"][nowIndex - 6] | NAN;
    float nowPressure = hourly["pressure_msl"][nowIndex] | NAN;

    if (!isnan(oldPressure) && !isnan(nowPressure)) {
      weather.pressureDelta6h = nowPressure - oldPressure;
    }
  }

  weather.hourlyCount = 0;

  for (int source = nowIndex;
       source < (int)times.size() && weather.hourlyCount < 24;
       source++) {
    HourData &hour = weather.hourly[weather.hourlyCount++];

    String timestamp = times[source].as<String>();

    if (timestamp.length() >= 16) {
      snprintf(
        hour.time,
        sizeof(hour.time),
        "%s",
        timestamp.substring(11, 16).c_str());
    }

    hour.temperature =
      hourly["temperature_2m"][source] | NAN;

    hour.apparent =
      hourly["apparent_temperature"][source] | NAN;

    hour.pressure =
      hourly["pressure_msl"][source] | NAN;

    hour.wind =
      hourly["wind_speed_10m"][source] | NAN;

    hour.direction =
      hourly["wind_direction_10m"][source] | NAN;

    hour.gust =
      hourly["wind_gusts_10m"][source] | NAN;

    hour.precipitation =
      hourly["precipitation"][source] | NAN;

    hour.rainChance =
      hourly["precipitation_probability"][source] | 0;

    hour.weatherCode =
      hourly["weather_code"][source] | -1;
  }

  JsonObject daily = document["daily"];
  JsonArray dates = daily["time"];

  weather.dailyCount = min(8, (int)dates.size());

  for (int i = 0; i < weather.dailyCount; i++) {
    DayData &day = weather.daily[i];

    String date = dates[i].as<String>();

    snprintf(day.date, sizeof(day.date), "%s", date.c_str());

    day.low =
      daily["temperature_2m_min"][i] | NAN;

    day.high =
      daily["temperature_2m_max"][i] | NAN;

    day.precipitation =
      daily["precipitation_sum"][i] | NAN;

    day.rainChance =
      daily["precipitation_probability_max"][i] | 0;

    day.weatherCode =
      daily["weather_code"][i] | -1;

    String sunrise = daily["sunrise"][i] | "";
    String sunset = daily["sunset"][i] | "";

    if (sunrise.length() >= 16) {
      snprintf(
        day.sunrise,
        sizeof(day.sunrise),
        "%s",
        sunrise.substring(11, 16).c_str());
    }

    if (sunset.length() >= 16) {
      snprintf(
        day.sunset,
        sizeof(day.sunset),
        "%s",
        sunset.substring(11, 16).c_str());
    }
  }

  weather.valid = true;
  weather.fetchedAt = time(nullptr);
  lastWeatherUpdate = millis();

  Serial.printf(
    "[weather] %.1f C | %s | %d hourly | %d daily\n",
    weather.temperature,
    weatherName(weather.weatherCode).c_str(),
    weather.hourlyCount,
    weather.dailyCount);

  return true;
}

// ============================================================
// AIR API
// ============================================================

String airURL() {
  String url =
    "https://air-quality-api.open-meteo.com/v1/air-quality";

  url += "?latitude=" + String(LATITUDE, 6);
  url += "&longitude=" + String(LONGITUDE, 6);

  url +=
    "&current="
    "pm2_5,"
    "pm10,"
    "carbon_monoxide,"
    "nitrogen_dioxide,"
    "sulphur_dioxide,"
    "ozone,"
    "european_aqi,"
    "us_aqi";

  url += "&timezone=auto";

  return url;
}

bool updateAir() {
  Serial.println("[air] updating");

  JsonDocument document;

  if (!getJson(airURL(), document)) {
    Serial.println("[air] failed");
    return false;
  }

  JsonObject current = document["current"];

  air.pm25 = current["pm2_5"] | NAN;
  air.pm10 = current["pm10"] | NAN;
  air.co = current["carbon_monoxide"] | NAN;
  air.no2 = current["nitrogen_dioxide"] | NAN;
  air.so2 = current["sulphur_dioxide"] | NAN;
  air.ozone = current["ozone"] | NAN;
  air.europeanAqi = current["european_aqi"] | -1;
  air.usAqi = current["us_aqi"] | -1;

  air.valid = air.usAqi >= 0 || !isnan(air.pm25);
  air.fetchedAt = time(nullptr);
  lastAirUpdate = millis();

  Serial.printf("[air] AQI %d | PM2.5 %.1f\n", air.usAqi, air.pm25);

  return air.valid;
}

// ============================================================
// CACHE
// ============================================================

void saveCache() {
  if (!weather.valid) return;

  prefs.begin("weather-aura", false);

  prefs.putBool("valid", true);
  prefs.putFloat("temp", weather.temperature);
  prefs.putFloat("apparent", weather.apparent);
  prefs.putFloat("humidity", weather.humidity);
  prefs.putFloat("dew", weather.dewPoint);
  prefs.putFloat("pressure", weather.pressure);
  prefs.putFloat("wind", weather.wind);
  prefs.putFloat("gust", weather.gust);
  prefs.putFloat("direction", weather.direction);
  prefs.putFloat("uv", weather.uv);
  prefs.putInt("code", weather.weatherCode);
  prefs.putBool("day", weather.isDay);
  prefs.putLong64("time", (int64_t)weather.fetchedAt);

  prefs.end();
}

void loadCache() {
  prefs.begin("weather-aura", true);

  if (!prefs.getBool("valid", false)) {
    prefs.end();
    return;
  }

  weather.temperature = prefs.getFloat("temp", NAN);
  weather.apparent = prefs.getFloat("apparent", NAN);
  weather.humidity = prefs.getFloat("humidity", NAN);
  weather.dewPoint = prefs.getFloat("dew", NAN);
  weather.pressure = prefs.getFloat("pressure", NAN);
  weather.wind = prefs.getFloat("wind", NAN);
  weather.gust = prefs.getFloat("gust", NAN);
  weather.direction = prefs.getFloat("direction", NAN);
  weather.uv = prefs.getFloat("uv", NAN);
  weather.weatherCode = prefs.getInt("code", -1);
  weather.isDay = prefs.getBool("day", true);
  weather.fetchedAt = (time_t)prefs.getLong64("time", 0);
  weather.valid = true;

  prefs.end();

  Serial.println("[cache] restored");
}

// ============================================================
// SD HISTORY
// ============================================================

void initializeSD() {
  if (!ENABLE_SD_LOGGING) return;

  sdAvailable = SD.begin(SD_CS);

  if (!sdAvailable) {
    Serial.println("[sd] offline");
    return;
  }

  Serial.println("[sd] ready");

  if (!SD.exists("/weather.csv")) {
    File file = SD.open("/weather.csv", FILE_WRITE);

    if (file) {
      file.println(
        "time,temp,feels,humidity,dew,pressure,wind,gust,"
        "direction,precip,uv,aqi,pm25,pm10");

      file.close();
    }
  }
}

void logHistory() {
  if (!sdAvailable || !weather.valid) return;

  File file = SD.open("/weather.csv", FILE_APPEND);
  if (!file) return;

  file.printf(
    "%lld,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,"
    "%.1f,%.2f,%.2f,%d,%.2f,%.2f\n",
    (long long)time(nullptr),
    weather.temperature,
    weather.apparent,
    weather.humidity,
    weather.dewPoint,
    weather.pressure,
    weather.wind,
    weather.gust,
    weather.direction,
    weather.precipitation,
    weather.uv,
    air.usAqi,
    air.pm25,
    air.pm10);

  file.close();
  lastHistoryWrite = millis();
}

// ============================================================
// INPUT
// ============================================================

bool previousButtonState = HIGH;
uint32_t buttonDownAt = 0;
bool longPressHandled = false;

void handleButton() {
  bool state = digitalRead(USER_BUTTON);

  if (previousButtonState == HIGH && state == LOW) {
    buttonDownAt = millis();
    longPressHandled = false;
  }

  if (state == LOW && !longPressHandled && millis() - buttonDownAt >= 900) {
    longPressHandled = true;
    currentPage = PAGE_HOME;
    drawPage();
  }

  if (previousButtonState == LOW && state == HIGH) {
    uint32_t duration = millis() - buttonDownAt;

    if (!longPressHandled && duration >= 30) {
      currentPage =
        (Page)(((int)currentPage + 1) % PAGE_COUNT);

      drawPage();
    }
  }

  previousButtonState = state;
}

// ============================================================
// SCHEDULER
// ============================================================

void scheduledWork() {
  if (wifiConnected) {
    if (!weather.valid || millis() - lastWeatherUpdate >= WEATHER_REFRESH_MS) {
      if (updateWeather()) {
        saveCache();
        drawPage();
      }
    }

    if (!air.valid || millis() - lastAirUpdate >= AIR_REFRESH_MS) {
      if (updateAir()) drawPage();
    }
  }

  if (sdAvailable && millis() - lastHistoryWrite >= HISTORY_LOG_MS) {
    logHistory();
  }

  if (millis() - lastUiRefresh >= UI_REFRESH_MS) {
    lastUiRefresh = millis();

    static String previousMinute;
    String currentMinute = clockText();

    if (currentMinute != previousMinute) {
      previousMinute = currentMinute;
      drawPage();
    }
  }

  if (ENABLE_AUTO_PAGE && millis() - lastPageChange >= AUTO_PAGE_MS) {
    currentPage =
      (Page)(((int)currentPage + 1) % PAGE_COUNT);

    drawPage();
  }
}

// ============================================================
// SETUP / LOOP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("================================");
  Serial.println(" WEATHER STATION V3.1 AURA");
  Serial.println("================================");

  pinMode(USER_BUTTON, INPUT_PULLUP);
  pinMode(LCD_BACKLIGHT, OUTPUT);

  digitalWrite(LCD_BACKLIGHT, HIGH);

  analogReadResolution(12);

  loadCache();
  initializeDisplay();

  // UI before network.
  drawPage();
  lv_timer_handler();

  initializeSD();
  setupTime();
  startWiFi();

  lastPageChange = millis();

  Serial.println("[boot] ready");
}

void loop() {
  maintainWiFi();
  handleButton();
  scheduledWork();

  lv_timer_handler();
  delay(5);
}
