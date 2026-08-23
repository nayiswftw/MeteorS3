/*
  WEATHER STATION FINAL
  Waveshare ESP32-S3-LCD-2
  Arduino IDE + LVGL 9

  Goal: V1 feature depth with V2.1 visual minimalism.

  Libraries:
    - ArduinoJson 7.x
    - Arduino_GFX_Library
    - LVGL 9.x

  Controls:
    - Short BOOT press: next page
    - Long BOOT press: home
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

// ============================================================
// 1. USER CONFIGURATION
// ============================================================

const char *WIFI_SSID     = "Xiaomi 11i HyperCharge";
const char *WIFI_PASSWORD = "12345678";

const double LATITUDE  = 30.000000;
const double LONGITUDE = 75.000000;

const char *LOCATION_NAME = "HOME";
const char *TIMEZONE      = "Asia/Kolkata";

enum TemperatureUnit { TEMP_C, TEMP_F };
enum WindUnit { WIND_KMH, WIND_MPH, WIND_MS };
enum PressureUnit { PRESS_HPA, PRESS_INHG };

TemperatureUnit temperatureUnit = TEMP_C;
WindUnit windUnit = WIND_KMH;
PressureUnit pressureUnit = PRESS_HPA;

constexpr bool ENABLE_SD_LOGGING = true;
constexpr bool AUTO_PAGE_ROTATION = false;

// ============================================================
// 2. HARDWARE — PRESERVED FROM WORKING V1
// ============================================================

constexpr int LCD_WIDTH  = 240;
constexpr int LCD_HEIGHT = 320;

constexpr int LCD_SCLK = 39;
constexpr int LCD_MOSI = 38;
constexpr int LCD_MISO = 40;
constexpr int LCD_DC   = 42;
constexpr int LCD_CS   = 45;
constexpr int LCD_RST  = -1;

constexpr int LCD_BACKLIGHT = 1;
constexpr int USER_BUTTON   = 0;
constexpr int BATTERY_ADC   = 4;
constexpr int SD_CS         = 21;

// ============================================================
// 3. TIMING
// ============================================================

constexpr uint32_t WEATHER_REFRESH_MS = 10UL * 60UL * 1000UL;
constexpr uint32_t AIR_REFRESH_MS     = 30UL * 60UL * 1000UL;
constexpr uint32_t HISTORY_LOG_MS     = 15UL * 60UL * 1000UL;
constexpr uint32_t WIFI_RETRY_MS      = 20UL * 1000UL;
constexpr uint32_t UI_TICK_MS         = 1000UL;
constexpr uint32_t AUTO_PAGE_MS       = 25UL * 1000UL;

// ============================================================
// 4. COLORS
// ============================================================

#define C_BG       lv_color_hex(0x071018)
#define C_SURFACE  lv_color_hex(0x0D1A24)
#define C_SURFACE2 lv_color_hex(0x122532)
#define C_TEXT     lv_color_hex(0xF3F7FA)
#define C_MUTED    lv_color_hex(0x8195A3)
#define C_DIM      lv_color_hex(0x50616D)
#define C_CYAN     lv_color_hex(0x55D6E8)
#define C_BLUE     lv_color_hex(0x6BA8FF)
#define C_GREEN    lv_color_hex(0x72D49B)
#define C_YELLOW   lv_color_hex(0xF3CC72)
#define C_ORANGE   lv_color_hex(0xF0A96B)
#define C_RED      lv_color_hex(0xFF7777)
#define C_PURPLE   lv_color_hex(0xB59BFF)

// ============================================================
// 5. DATA
// ============================================================

struct HourData {
  char time[6] = "--:--";
  float temperature = NAN;
  float apparent = NAN;
  float pressure = NAN;
  float wind = NAN;
  float gust = NAN;
  float windDirection = NAN;
  float rain = NAN;
  float precipitation = NAN;
  int rainChance = 0;
  int weatherCode = -1;
};

struct DayData {
  char date[11] = "";
  float minimum = NAN;
  float maximum = NAN;
  float precipitation = NAN;
  int rainChance = 0;
  int weatherCode = -1;
  char sunrise[6] = "--:--";
  char sunset[6] = "--:--";
};

struct WeatherData {
  bool valid = false;
  char sourceTime[20] = "";

  float temperature = NAN;
  float apparent = NAN;
  float humidity = NAN;
  float dewPoint = NAN;
  float pressure = NAN;
  float pressureDelta6h = NAN;
  float wind = NAN;
  float gust = NAN;
  float windDirection = NAN;
  float rain = NAN;
  float precipitation = NAN;
  float visibility = NAN;
  float uv = NAN;

  int weatherCode = -1;
  bool daytime = true;

  HourData hourly[24];
  int hourlyCount = 0;

  DayData daily[7];
  int dailyCount = 0;

  time_t fetchedAt = 0;
};

struct AirData {
  bool valid = false;
  float pm25 = NAN;
  float pm10 = NAN;
  float ozone = NAN;
  float no2 = NAN;
  float so2 = NAN;
  float co = NAN;
  int usAqi = -1;
  int europeanAqi = -1;
  time_t fetchedAt = 0;
};

struct AlertItem {
  String text;
  lv_color_t color;
};

WeatherData weather;
AirData air;
AlertItem alerts[8];
int alertCount = 0;

// ============================================================
// 6. APP STATE
// ============================================================

enum Page {
  PAGE_NOW,
  PAGE_NEXT,
  PAGE_WEEK,
  PAGE_AIR,
  PAGE_SUN,
  PAGE_ALERTS,
  PAGE_HISTORY,
  PAGE_SYSTEM,
  PAGE_COUNT
};

Page currentPage = PAGE_NOW;

bool wifiConnected = false;
bool sdAvailable = false;

uint32_t lastWifiAttempt = 0;
uint32_t lastWeatherUpdate = 0;
uint32_t lastAirUpdate = 0;
uint32_t lastHistoryWrite = 0;
uint32_t lastUiTick = 0;
uint32_t lastPageChange = 0;

Preferences preferences;

// ============================================================
// 7. DISPLAY
// ============================================================

Arduino_DataBus *lcdBus =
  new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, LCD_MISO);

Arduino_GFX *gfx =
  new Arduino_ST7789(lcdBus, LCD_RST, 0, true, LCD_WIDTH, LCD_HEIGHT);

lv_display_t *display = nullptr;
uint8_t *lvBuffer1 = nullptr;
uint8_t *lvBuffer2 = nullptr;

constexpr size_t LV_BUFFER_LINES = 40;

lv_obj_t *root = nullptr;

// ============================================================
// 8. FORMAT HELPERS
// ============================================================

String formatTemperature(float c, unsigned int decimals = 0) {
  if (isnan(c)) return "--";

  if (temperatureUnit == TEMP_F) {
    return String(c * 9.0f / 5.0f + 32.0f, decimals) + " F";
  }

  return String(c, decimals) + " C";
}

String formatWind(float kmh, unsigned int decimals = 0) {
  if (isnan(kmh)) return "--";

  if (windUnit == WIND_MPH) return String(kmh * 0.621371f, decimals) + " mph";
  if (windUnit == WIND_MS)  return String(kmh / 3.6f, decimals) + " m/s";

  return String(kmh, decimals) + " km/h";
}

String formatPressure(float hpa, unsigned int decimals = 0) {
  if (isnan(hpa)) return "--";

  if (pressureUnit == PRESS_INHG) {
    return String(hpa * 0.029529983f, decimals) + " inHg";
  }

  return String(hpa, decimals) + " hPa";
}

String formatValue(float value, unsigned int decimals = 0) {
  return isnan(value) ? "--" : String(value, decimals);
}

String localTimeText() {
  struct tm now;
  if (!getLocalTime(&now, 20)) return "--:--";

  char text[6];
  strftime(text, sizeof(text), "%H:%M", &now);
  return String(text);
}

String localDateText() {
  struct tm now;
  if (!getLocalTime(&now, 20)) return "--- -- ---";

  char text[18];
  strftime(text, sizeof(text), "%a %d %b", &now);
  return String(text);
}

String weatherDescription(int code) {
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
  return "Unknown";
}

String weatherGlyph(int code, bool daytime = true) {
  // ASCII keeps this compatible with stock LVGL font configs.
  if (code == 0) return daytime ? "SUN" : "MOON";
  if (code <= 3) return "CLOUD";
  if (code == 45 || code == 48) return "FOG";
  if (code >= 51 && code <= 67) return "RAIN";
  if (code >= 71 && code <= 77) return "SNOW";
  if (code >= 80 && code <= 82) return "SHOWERS";
  if (code >= 95) return "STORM";
  return "WX";
}

String windDirectionText(float degrees) {
  if (isnan(degrees)) return "--";

  static const char *directions[] = {
    "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
  };

  int index = (int)((degrees + 11.25f) / 22.5f) % 16;
  return directions[index];
}

String pressureTrendText() {
  if (isnan(weather.pressureDelta6h)) return "Stable";
  if (weather.pressureDelta6h > 2.0f) return "Rising";
  if (weather.pressureDelta6h < -2.0f) return "Falling";
  return "Stable";
}

String dataAgeText(time_t timestamp) {
  if (timestamp <= 0) return "NO DATA";

  time_t now = time(nullptr);
  if (now <= 0 || now < timestamp) return "CACHED";

  long minutes = (long)((now - timestamp) / 60);
  if (minutes < 1) return "LIVE";
  if (minutes < 60) return String(minutes) + "m OLD";

  return String(minutes / 60) + "h OLD";
}

String aqiCategory(int aqi) {
  if (aqi < 0) return "--";
  if (aqi <= 50) return "GOOD";
  if (aqi <= 100) return "MODERATE";
  if (aqi <= 150) return "UNHEALTHY*";
  if (aqi <= 200) return "UNHEALTHY";
  if (aqi <= 300) return "VERY BAD";
  return "HAZARDOUS";
}

lv_color_t aqiColor(int aqi) {
  if (aqi < 0) return C_MUTED;
  if (aqi <= 50) return C_GREEN;
  if (aqi <= 100) return C_YELLOW;
  if (aqi <= 150) return C_ORANGE;
  return C_RED;
}

// ============================================================
// 9. UI PRIMITIVES
// ============================================================

void styleSurface(lv_obj_t *object, lv_color_t background, int radius = 10) {
  lv_obj_set_style_bg_color(object, background, 0);
  lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(object, 0, 0);
  lv_obj_set_style_radius(object, radius, 0);
  lv_obj_set_style_pad_all(object, 0, 0);
  lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *makeSurface(int x, int y, int width, int height,
                      lv_color_t color = C_SURFACE) {
  lv_obj_t *object = lv_obj_create(root);
  lv_obj_set_pos(object, x, y);
  lv_obj_set_size(object, width, height);
  styleSurface(object, color);
  return object;
}

lv_obj_t *makeLabel(lv_obj_t *parent, const String &text,
                    int x, int y, int width, int height,
                    lv_color_t color = C_TEXT,
                    lv_text_align_t alignment = LV_TEXT_ALIGN_LEFT) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text.c_str());
  lv_obj_set_pos(label, x, y);
  lv_obj_set_size(label, width, height);
  lv_obj_set_style_text_color(label, color, 0);

  // Stock LVGL builds reliably include Montserrat 14.
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_align(label, alignment, 0);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);

  return label;
}

void makeRule(int x, int y, int width, lv_color_t color = C_SURFACE2) {
  lv_obj_t *rule = lv_obj_create(root);
  lv_obj_set_pos(rule, x, y);
  lv_obj_set_size(rule, width, 1);
  lv_obj_set_style_bg_color(rule, color, 0);
  lv_obj_set_style_bg_opa(rule, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(rule, 0, 0);
}

void makeProgressBar(int x, int y, int width, int height,
                     int percent, lv_color_t color) {
  percent = constrain(percent, 0, 100);

  lv_obj_t *track = makeSurface(x, y, width, height, C_SURFACE2);
  lv_obj_set_style_radius(track, height / 2, 0);

  int fillWidth = max(2, width * percent / 100);

  lv_obj_t *fill = lv_obj_create(root);
  lv_obj_set_pos(fill, x, y);
  lv_obj_set_size(fill, fillWidth, height);
  lv_obj_set_style_bg_color(fill, color, 0);
  lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(fill, 0, 0);
  lv_obj_set_style_radius(fill, height / 2, 0);
}

void clearUi() {
  if (root) lv_obj_clean(root);
}

void drawHeader(const String &title) {
  makeLabel(root, LOCATION_NAME, 10, 7, 105, 19, C_CYAN);
  makeLabel(root, localTimeText(), 125, 7, 55, 19, C_TEXT, LV_TEXT_ALIGN_RIGHT);

  makeLabel(
    root,
    wifiConnected ? "WIFI" : "OFF",
    184, 7, 46, 19,
    wifiConnected ? C_GREEN : C_RED,
    LV_TEXT_ALIGN_RIGHT
  );

  makeRule(10, 31, 220);
  makeLabel(root, title, 10, 36, 160, 19, C_MUTED);
}

void drawFooter() {
  String dots;

  for (int i = 0; i < PAGE_COUNT; i++) {
    dots += (i == currentPage) ? "O" : ".";
    if (i < PAGE_COUNT - 1) dots += " ";
  }

  makeLabel(root, dots, 10, 298, 220, 16, C_DIM, LV_TEXT_ALIGN_CENTER);
}

// ============================================================
// 10. WEATHER INSIGHT
// ============================================================

String weatherInsight() {
  if (!weather.valid) return "Waiting for weather data";

  int rainChance = weather.hourlyCount > 0 ? weather.hourly[0].rainChance : 0;

  if (weather.weatherCode >= 95) return "Thunderstorms possible";
  if (rainChance >= 70) return "Umbrella recommended";
  if (weather.uv >= 8) return "Very high UV outdoors";
  if (weather.uv >= 6) return "High UV outdoors";
  if (weather.gust >= 55) return "Strong gusts possible";
  if (weather.temperature >= 38) return "Extreme heat";
  if (air.valid && air.usAqi > 150) return "Poor air quality";
  if (weather.temperature <= 10) return "Cool conditions";

  return "No major weather signal";
}


// ============================================================
// 10B. MODERN WEATHER UI PRIMITIVES
// ============================================================

lv_obj_t *makeText(const String &text, int x, int y, int width, int height,
                   const lv_font_t *font, lv_color_t color = C_TEXT,
                   lv_text_align_t alignment = LV_TEXT_ALIGN_LEFT) {
  lv_obj_t *label = makeLabel(root, text, x, y, width, height, color, alignment);
  lv_obj_set_style_text_font(label, font, 0);
  return label;
}

lv_obj_t *makeTextOn(lv_obj_t *parent, const String &text, int x, int y,
                     int width, int height, const lv_font_t *font,
                     lv_color_t color = C_TEXT,
                     lv_text_align_t alignment = LV_TEXT_ALIGN_LEFT) {
  lv_obj_t *label = makeLabel(parent, text, x, y, width, height, color, alignment);
  lv_obj_set_style_text_font(label, font, 0);
  return label;
}

void drawTopBar(const String &section = "") {
  makeText(LOCATION_NAME, 12, 7, 126, 24, &lv_font_montserrat_18, C_TEXT);
  makeText(localTimeText(), 140, 8, 50, 20, &lv_font_montserrat_14,
           C_MUTED, LV_TEXT_ALIGN_RIGHT);

  lv_obj_t *dot = makeSurface(199, 13, 7, 7, wifiConnected ? C_GREEN : C_RED);
  lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);

  int battery = batteryPercent();
  makeText(String(battery) + "%", 208, 8, 28, 18, &lv_font_montserrat_12,
           battery <= 20 ? C_RED : C_MUTED, LV_TEXT_ALIGN_RIGHT);

  if (section.length()) {
    makeText(section, 12, 33, 216, 17, &lv_font_montserrat_12, C_DIM);
  }
}

void drawPageDots() {
  int totalWidth = PAGE_COUNT * 12;
  int x = (LCD_WIDTH - totalWidth) / 2;

  for (int i = 0; i < PAGE_COUNT; i++) {
    int size = (i == currentPage) ? 6 : 4;
    lv_obj_t *dot = makeSurface(x + i * 12, 306, size, size,
                                i == currentPage ? C_CYAN : C_DIM);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
  }
}

void drawMetricTile(int x, int y, int width, int height,
                    const String &title, const String &value,
                    const String &detail, lv_color_t accent) {
  lv_obj_t *tile = makeSurface(x, y, width, height, C_SURFACE);
  lv_obj_set_style_radius(tile, 14, 0);

  lv_obj_t *accentBar = lv_obj_create(tile);
  lv_obj_set_pos(accentBar, 0, 0);
  lv_obj_set_size(accentBar, 3, height);
  lv_obj_set_style_bg_color(accentBar, accent, 0);
  lv_obj_set_style_bg_opa(accentBar, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(accentBar, 0, 0);
  lv_obj_set_style_radius(accentBar, 2, 0);

  makeTextOn(tile, title, 10, 8, width - 18, 16, &lv_font_montserrat_12, C_MUTED);
  makeTextOn(tile, value, 10, 27, width - 18, 25, &lv_font_montserrat_18, C_TEXT);
  makeTextOn(tile, detail, 10, height - 20, width - 18, 15,
             &lv_font_montserrat_12, C_DIM);
}

void drawTemperatureCurve(int x, int y, int width, int height,
                          int startIndex, int count, bool showLabels) {
  if (!weather.valid || weather.hourlyCount == 0 || count < 2) return;

  count = min(count, weather.hourlyCount - startIndex);
  if (count < 2) return;

  float minimum = 999.0f;
  float maximum = -999.0f;

  for (int i = 0; i < count; i++) {
    float t = weather.hourly[startIndex + i].temperature;
    if (isnan(t)) continue;
    minimum = min(minimum, t);
    maximum = max(maximum, t);
  }

  if (minimum > 900 || maximum < -900) return;
  if (maximum - minimum < 2.0f) {
    maximum += 1.0f;
    minimum -= 1.0f;
  }

  lv_point_precise_t points[8];
  count = min(count, 8);

  for (int i = 0; i < count; i++) {
    float t = weather.hourly[startIndex + i].temperature;
    float normalized = (t - minimum) / (maximum - minimum);

    points[i].x = x + (width * i) / (count - 1);
    points[i].y = y + height - (int)(normalized * height);

    lv_obj_t *point = makeSurface(points[i].x - 2, points[i].y - 2, 5, 5, C_CYAN);
    lv_obj_set_style_radius(point, LV_RADIUS_CIRCLE, 0);

    if (showLabels) {
      makeText(formatTemperature(t), points[i].x - 20, y - 20, 40, 16,
               &lv_font_montserrat_12, C_TEXT, LV_TEXT_ALIGN_CENTER);
    }
  }

  for (int i = 0; i < count - 1; i++) {
    lv_obj_t *line = lv_line_create(root);
    lv_point_precise_t segment[2] = {points[i], points[i + 1]};
    lv_line_set_points(line, segment, 2);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_color(line, C_CYAN, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
  }
}

// ============================================================
// 11. PAGE: NOW
// ============================================================

void drawNowPage() {
  clearUi();
  drawTopBar();

  if (!weather.valid) {
    makeText("Weather", 12, 77, 216, 24, &lv_font_montserrat_20, C_MUTED);
    makeText("--", 12, 104, 216, 58, &lv_font_montserrat_48, C_TEXT);
    makeText("Waiting for current conditions", 12, 168, 216, 20,
             &lv_font_montserrat_14, C_MUTED);
    drawPageDots();
    return;
  }

  String highLow = "";
  if (weather.dailyCount > 0) {
    highLow = "  H " + formatTemperature(weather.daily[0].maximum) +
              "  L " + formatTemperature(weather.daily[0].minimum);
  }

  makeText(weatherDescription(weather.weatherCode), 12, 57, 145, 22,
           &lv_font_montserrat_16, C_MUTED);
  makeText(dataAgeText(weather.fetchedAt), 160, 59, 68, 18,
           &lv_font_montserrat_12,
           wifiConnected ? C_GREEN : C_YELLOW,
           LV_TEXT_ALIGN_RIGHT);

  makeText(formatTemperature(weather.temperature), 8, 79, 220, 58,
           &lv_font_montserrat_48, C_TEXT);

  makeText("Feels " + formatTemperature(weather.apparent) + highLow,
           12, 140, 216, 18, &lv_font_montserrat_12, C_MUTED);

  lv_obj_t *insight = makeSurface(10, 164, 220, 31, C_SURFACE2);
  lv_obj_set_style_radius(insight, 12, 0);
  makeTextOn(insight, weatherInsight(), 10, 7, 200, 18,
             &lv_font_montserrat_12, C_TEXT);

  int rainChance = weather.hourlyCount > 0 ? weather.hourly[0].rainChance : 0;

  drawMetricTile(10, 205, 105, 80,
                 "HUMIDITY",
                 formatValue(weather.humidity) + "%",
                 "Dew " + formatTemperature(weather.dewPoint),
                 C_BLUE);

  drawMetricTile(125, 205, 105, 80,
                 "RAIN",
                 String(rainChance) + "%",
                 formatValue(weather.precipitation, 1) + " mm now",
                 C_CYAN);

  drawPageDots();
}

// ============================================================
// 12. PAGE: NEXT 24 HOURS
// ============================================================

void drawNextPage() {
  clearUi();
  drawTopBar("HOURLY FORECAST");

  if (!weather.valid || weather.hourlyCount == 0) {
    makeText("No hourly forecast", 12, 140, 216, 20,
             &lv_font_montserrat_14, C_MUTED, LV_TEXT_ALIGN_CENTER);
    drawPageDots();
    return;
  }

  drawTemperatureCurve(20, 94, 200, 54, 0, 7, true);

  int y = 164;
  int columns = 4;

  for (int i = 0; i < columns; i++) {
    int index = i * 2;
    if (index >= weather.hourlyCount) break;

    HourData &hour = weather.hourly[index];
    int x = 10 + i * 57;

    makeText(i == 0 ? "NOW" : String(hour.time), x, y, 50, 17,
             &lv_font_montserrat_12, i == 0 ? C_CYAN : C_MUTED,
             LV_TEXT_ALIGN_CENTER);

    makeText(weatherGlyph(hour.weatherCode), x, y + 23, 50, 17,
             &lv_font_montserrat_12, C_TEXT, LV_TEXT_ALIGN_CENTER);

    makeText(String(hour.rainChance) + "%", x, y + 47, 50, 17,
             &lv_font_montserrat_12,
             hour.rainChance >= 50 ? C_BLUE : C_DIM,
             LV_TEXT_ALIGN_CENTER);

    makeText(formatWind(hour.wind), x - 3, y + 68, 56, 17,
             &lv_font_montserrat_12, C_MUTED, LV_TEXT_ALIGN_CENTER);
  }

  lv_obj_t *summary = makeSurface(10, 267, 220, 27, C_SURFACE2);
  lv_obj_set_style_radius(summary, 11, 0);

  int maxRain = 0;
  float maxGust = 0;

  for (int i = 0; i < weather.hourlyCount; i++) {
    maxRain = max(maxRain, weather.hourly[i].rainChance);
    if (!isnan(weather.hourly[i].gust)) maxGust = max(maxGust, weather.hourly[i].gust);
  }

  makeTextOn(summary,
             "Peak rain " + String(maxRain) + "%   Gust " + formatWind(maxGust),
             8, 5, 204, 17, &lv_font_montserrat_12, C_TEXT,
             LV_TEXT_ALIGN_CENTER);

  drawPageDots();
}

// ============================================================
// 13. PAGE: WEEK
// ============================================================

void drawWeekPage() {
  clearUi();
  drawTopBar("7 DAY FORECAST");

  if (!weather.valid || weather.dailyCount == 0) {
    makeText("No daily forecast", 12, 140, 216, 20,
             &lv_font_montserrat_14, C_MUTED, LV_TEXT_ALIGN_CENTER);
    drawPageDots();
    return;
  }

  float globalMin = 999.0f;
  float globalMax = -999.0f;

  for (int i = 0; i < weather.dailyCount; i++) {
    globalMin = min(globalMin, weather.daily[i].minimum);
    globalMax = max(globalMax, weather.daily[i].maximum);
  }

  if (globalMax - globalMin < 1.0f) globalMax = globalMin + 1.0f;

  int y = 59;

  for (int i = 0; i < weather.dailyCount; i++) {
    DayData &day = weather.daily[i];
    String dayName = i == 0 ? "TODAY" : String(day.date + 5);

    makeText(dayName, 10, y, 48, 18, &lv_font_montserrat_12,
             i == 0 ? C_CYAN : C_MUTED);
    makeText(weatherGlyph(day.weatherCode), 58, y, 47, 18,
             &lv_font_montserrat_12, C_TEXT);

    makeText(formatTemperature(day.minimum), 106, y, 43, 18,
             &lv_font_montserrat_12, C_MUTED, LV_TEXT_ALIGN_RIGHT);

    int trackX = 154;
    int trackW = 45;
    lv_obj_t *track = makeSurface(trackX, y + 7, trackW, 4, C_SURFACE2);
    lv_obj_set_style_radius(track, 2, 0);

    int lowX = trackX + (int)((day.minimum - globalMin) / (globalMax - globalMin) * trackW);
    int highX = trackX + (int)((day.maximum - globalMin) / (globalMax - globalMin) * trackW);
    int rangeW = max(4, highX - lowX);

    lv_obj_t *range = makeSurface(lowX, y + 7, rangeW, 4, C_ORANGE);
    lv_obj_set_style_radius(range, 2, 0);

    makeText(formatTemperature(day.maximum), 201, y, 29, 18,
             &lv_font_montserrat_12, C_TEXT, LV_TEXT_ALIGN_RIGHT);

    makeText(String(day.rainChance) + "%", 58, y + 18, 47, 15,
             &lv_font_montserrat_12,
             day.rainChance >= 50 ? C_BLUE : C_DIM);

    y += 34;
  }

  drawPageDots();
}

// ============================================================
// 14. PAGE: AIR
// ============================================================

void drawAirPage() {
  clearUi();
  drawTopBar("AIR QUALITY");

  if (!air.valid) {
    makeText("Waiting for air quality", 12, 140, 216, 20,
             &lv_font_montserrat_14, C_MUTED, LV_TEXT_ALIGN_CENTER);
    drawPageDots();
    return;
  }

  lv_color_t quality = aqiColor(air.usAqi);

  makeText(String(air.usAqi), 10, 65, 105, 54,
           &lv_font_montserrat_48, quality);
  makeText("US AQI", 120, 73, 108, 18,
           &lv_font_montserrat_12, C_MUTED, LV_TEXT_ALIGN_RIGHT);
  makeText(aqiCategory(air.usAqi), 120, 94, 108, 22,
           &lv_font_montserrat_16, quality, LV_TEXT_ALIGN_RIGHT);

  int aqiPercent = constrain(air.usAqi * 100 / 300, 0, 100);
  makeProgressBar(10, 127, 220, 6, aqiPercent, quality);

  drawMetricTile(10, 151, 105, 63,
                 "PM2.5", formatValue(air.pm25, 1),
                 "ug/m3", quality);

  drawMetricTile(125, 151, 105, 63,
                 "PM10", formatValue(air.pm10, 1),
                 "ug/m3", C_ORANGE);

  drawMetricTile(10, 224, 105, 63,
                 "OZONE", formatValue(air.ozone),
                 "ug/m3", C_CYAN);

  drawMetricTile(125, 224, 105, 63,
                 "NO2", formatValue(air.no2),
                 dataAgeText(air.fetchedAt), C_PURPLE);

  drawPageDots();
}

// ============================================================
// 15. MOON + SUN PAGE
// ============================================================

String moonPhase() {
  struct tm now;
  if (!getLocalTime(&now, 20)) return "Unknown";

  int year = now.tm_year + 1900;
  int month = now.tm_mon + 1;
  int day = now.tm_mday;

  if (month < 3) {
    year--;
    month += 12;
  }

  long days =
    365L * year + year / 4 - year / 100 + year / 400 +
    (153L * (month + 1)) / 5 + day - 730551L;

  double phase = fmod(days + 4.867, 29.530588853);
  if (phase < 0) phase += 29.530588853;

  double fraction = phase / 29.530588853;

  if (fraction < 0.03) return "New moon";
  if (fraction < 0.22) return "Waxing crescent";
  if (fraction < 0.28) return "First quarter";
  if (fraction < 0.47) return "Waxing gibbous";
  if (fraction < 0.53) return "Full moon";
  if (fraction < 0.72) return "Waning gibbous";
  if (fraction < 0.78) return "Last quarter";
  if (fraction < 0.97) return "Waning crescent";

  return "New moon";
}

void drawSunPage() {
  clearUi();
  drawHeader("SUN & MOON");

  if (!weather.valid) {
    makeLabel(root, "Waiting for weather...", 10, 120, 220, 20,
              C_MUTED, LV_TEXT_ALIGN_CENTER);
    drawFooter();
    return;
  }

  const char *sunrise = weather.dailyCount > 0 ? weather.daily[0].sunrise : "--:--";
  const char *sunset  = weather.dailyCount > 0 ? weather.daily[0].sunset : "--:--";

  makeLabel(root, "SUNRISE", 10, 67, 90, 18, C_MUTED);
  makeLabel(root, sunrise, 10, 89, 90, 22, C_YELLOW);

  makeLabel(root, "SUNSET", 130, 67, 90, 18, C_MUTED);
  makeLabel(root, sunset, 130, 89, 90, 22, C_ORANGE);

  makeRule(10, 125, 220);

  makeLabel(root, "STATUS", 10, 145, 80, 18, C_MUTED);
  makeLabel(root, weather.daytime ? "DAY" : "NIGHT",
            130, 145, 100, 18,
            weather.daytime ? C_YELLOW : C_PURPLE,
            LV_TEXT_ALIGN_RIGHT);

  makeLabel(root, "UV INDEX", 10, 178, 80, 18, C_MUTED);
  makeLabel(root, formatValue(weather.uv, 1),
            130, 178, 100, 18,
            weather.uv >= 6 ? C_YELLOW : C_TEXT,
            LV_TEXT_ALIGN_RIGHT);

  makeLabel(root, "MOON", 10, 211, 80, 18, C_MUTED);
  makeLabel(root, moonPhase(), 95, 211, 135, 18,
            C_CYAN, LV_TEXT_ALIGN_RIGHT);

  lv_obj_t *summary = makeSurface(10, 253, 220, 35, C_SURFACE2);

  makeLabel(summary,
            weather.uv >= 6 ? "Sun protection recommended" : "Normal UV conditions",
            8, 8, 204, 19, C_TEXT);

  drawFooter();
}

// ============================================================
// 16. ALERT ENGINE + PAGE
// ============================================================

void addAlert(const String &text, lv_color_t color) {
  if (alertCount >= 8) return;

  alerts[alertCount].text = text;
  alerts[alertCount].color = color;
  alertCount++;
}

void buildAlerts() {
  alertCount = 0;

  if (!weather.valid) {
    addAlert("Weather data unavailable", C_RED);
    return;
  }

  if (weather.temperature >= 40) addAlert("Extreme heat", C_RED);
  else if (weather.temperature >= 35) addAlert("High temperature", C_ORANGE);

  if (weather.wind >= 50 || weather.gust >= 65) addAlert("Strong wind", C_RED);

  if (weather.hourlyCount > 0 && weather.hourly[0].rainChance >= 70) {
    addAlert("High rain chance", C_BLUE);
  }

  if (weather.uv >= 8) addAlert("Very high UV", C_RED);
  else if (weather.uv >= 6) addAlert("High UV", C_YELLOW);

  if (air.valid && air.usAqi >= 151) addAlert("Poor air quality", C_RED);
  if (weather.weatherCode >= 95) addAlert("Thunderstorm", C_PURPLE);
  if (weather.pressure < 995) addAlert("Low pressure", C_ORANGE);

  if (alertCount == 0) addAlert("No active alerts", C_GREEN);
}

void drawAlertsPage() {
  clearUi();
  drawHeader("ALERTS");
  buildAlerts();

  int y = 64;

  for (int i = 0; i < alertCount; i++) {
    lv_obj_t *row = makeSurface(10, y, 220, 29, C_SURFACE);
    makeLabel(row, alerts[i].text, 8, 5, 204, 18, alerts[i].color);

    y += 35;
    if (y > 285) break;
  }

  drawFooter();
}

// ============================================================
// 17. BATTERY
// ============================================================

float batteryVoltage() {
  uint32_t total = 0;

  for (int i = 0; i < 8; i++) {
    total += analogRead(BATTERY_ADC);
    delayMicroseconds(100);
  }

  float raw = total / 8.0f;
  float voltage = raw * 3.3f / 4095.0f;

  // Same simple divider assumption as V1.
  return voltage * 2.0f;
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
// 18. SD HISTORY
// ============================================================

void initializeSD() {
  if (!ENABLE_SD_LOGGING) {
    sdAvailable = false;
    Serial.println("[sd] disabled");
    return;
  }

  sdAvailable = SD.begin(SD_CS);

  if (!sdAvailable) {
    Serial.println("[sd] not mounted");
    return;
  }

  Serial.println("[sd] mounted");

  if (!SD.exists("/weather.csv")) {
    File file = SD.open("/weather.csv", FILE_WRITE);

    if (file) {
      file.println(
        "timestamp,temp,feels,humidity,dewpoint,pressure,"
        "wind,gust,direction,rain,uv,aqi,pm25,pm10"
      );
      file.close();
    }
  }
}

void saveHistory() {
  if (!sdAvailable || !weather.valid) return;

  File file = SD.open("/weather.csv", FILE_APPEND);
  if (!file) return;

  time_t now = time(nullptr);

  file.printf(
    "%lld,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f,%.2f,%.2f,%d,%.2f,%.2f\n",
    (long long)now,
    weather.temperature,
    weather.apparent,
    weather.humidity,
    weather.dewPoint,
    weather.pressure,
    weather.wind,
    weather.gust,
    weather.windDirection,
    weather.rain,
    weather.uv,
    air.usAqi,
    air.pm25,
    air.pm10
  );

  file.close();
  Serial.println("[history] logged");
}

void drawHistoryPage() {
  clearUi();
  drawHeader("HISTORY");

  makeLabel(root, "TF CARD", 10, 68, 90, 18, C_MUTED);
  makeLabel(root, sdAvailable ? "READY" : "OFFLINE",
            130, 68, 100, 18,
            sdAvailable ? C_GREEN : C_RED,
            LV_TEXT_ALIGN_RIGHT);

  makeLabel(root, "LOG INTERVAL", 10, 104, 100, 18, C_MUTED);
  makeLabel(root, String(HISTORY_LOG_MS / 60000UL) + " min",
            130, 104, 100, 18, C_TEXT, LV_TEXT_ALIGN_RIGHT);

  makeLabel(root, "FILE", 10, 140, 90, 18, C_MUTED);
  makeLabel(root, "/weather.csv",
            110, 140, 120, 18, C_TEXT, LV_TEXT_ALIGN_RIGHT);

  makeLabel(root, "LAST WRITE", 10, 176, 100, 18, C_MUTED);

  String lastWrite =
    lastHistoryWrite == 0
      ? "--"
      : String((millis() - lastHistoryWrite) / 60000UL) + "m ago";

  makeLabel(root, lastWrite,
            130, 176, 100, 18, C_TEXT, LV_TEXT_ALIGN_RIGHT);

  lv_obj_t *note = makeSurface(10, 222, 220, 66, C_SURFACE2);

  makeLabel(note,
            sdAvailable ? "Local CSV logging active" : "Station works without SD",
            8, 10, 204, 18,
            sdAvailable ? C_GREEN : C_YELLOW);

  makeLabel(note, "Weather remains usable offline",
            8, 34, 204, 18, C_MUTED);

  drawFooter();
}

// ============================================================
// 19. SYSTEM PAGE
// ============================================================

void drawSystemPage() {
  clearUi();
  drawHeader("SYSTEM");

  int y = 64;

  auto row = [&](const String &name, const String &value,
                 lv_color_t color = C_TEXT) {
    makeLabel(root, name, 10, y, 90, 18, C_MUTED);
    makeLabel(root, value, 105, y, 125, 18, color, LV_TEXT_ALIGN_RIGHT);
    y += 30;
  };

  row("WIFI", wifiConnected ? "CONNECTED" : "OFFLINE",
      wifiConnected ? C_GREEN : C_RED);

  row("RSSI", wifiConnected ? String(WiFi.RSSI()) + " dBm" : "--");
  row("IP", wifiConnected ? WiFi.localIP().toString() : "--");
  row("HEAP", String(ESP.getFreeHeap() / 1024) + " KB");

  row("PSRAM",
      psramFound() ? String(ESP.getFreePsram() / 1024) + " KB" : "NOT FOUND",
      psramFound() ? C_TEXT : C_YELLOW);

  float voltage = batteryVoltage();

  row("BATTERY",
      String(voltage, 2) + " V / " + String(batteryPercent()) + "%",
      C_YELLOW);

  row("WEATHER",
      weather.valid ? dataAgeText(weather.fetchedAt) : "NONE",
      weather.valid ? C_GREEN : C_RED);

  drawFooter();
}

// ============================================================
// 20. PAGE ROUTER
// ============================================================

void drawPage() {
  switch (currentPage) {
    case PAGE_NOW:     drawNowPage(); break;
    case PAGE_NEXT:    drawNextPage(); break;
    case PAGE_WEEK:    drawWeekPage(); break;
    case PAGE_AIR:     drawAirPage(); break;
    case PAGE_SUN:     drawSunPage(); break;
    case PAGE_ALERTS:  drawAlertsPage(); break;
    case PAGE_HISTORY: drawHistoryPage(); break;
    case PAGE_SYSTEM:  drawSystemPage(); break;

    default:
      currentPage = PAGE_NOW;
      drawNowPage();
      break;
  }

  lastPageChange = millis();
}

// ============================================================
// 21. DISPLAY INITIALIZATION
// ============================================================

void lvglFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *pixelMap) {
  uint32_t width = area->x2 - area->x1 + 1;
  uint32_t height = area->y2 - area->y1 + 1;

  gfx->draw16bitRGBBitmap(
    area->x1,
    area->y1,
    (uint16_t *)pixelMap,
    width,
    height
  );

  lv_display_flush_ready(disp);
}

uint32_t lvglTick() {
  return millis();
}

void initializeDisplay() {
  pinMode(LCD_BACKLIGHT, OUTPUT);

  // Preserve the known-working V1 behavior.
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
    MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL
  );

  lvBuffer2 = (uint8_t *)heap_caps_malloc(
    bufferSize,
    MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL
  );

  if (!lvBuffer1 || !lvBuffer2) {
    Serial.println("[display] LVGL buffer allocation failed");
    while (true) delay(1000);
  }

  lv_display_set_buffers(
    display,
    lvBuffer1,
    lvBuffer2,
    bufferSize,
    LV_DISPLAY_RENDER_MODE_PARTIAL
  );

  lv_display_set_flush_cb(display, lvglFlush);

  root = lv_screen_active();

  lv_obj_set_style_bg_color(root, C_BG, 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  lv_obj_set_style_text_font(root, &lv_font_montserrat_14, 0);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

  Serial.println("[display] ready");
}

// ============================================================
// 22. WIFI + TIME
// ============================================================

void startWiFi() {
  if (strlen(WIFI_SSID) == 0 ||
      strcmp(WIFI_SSID, "YOUR_WIFI_NAME") == 0) {
    Serial.println("[wifi] credentials not configured");
    return;
  }

  Serial.printf("[wifi] connecting to %s\n", WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
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

  if (!wifiConnected &&
      millis() - lastWifiAttempt >= WIFI_RETRY_MS) {
    Serial.println("[wifi] retry");

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
    "time.cloudflare.com"
  );
}

// ============================================================
// 23. HTTP JSON
// ============================================================

bool getJson(const String &url, JsonDocument &document) {
  if (!wifiConnected) return false;

  HTTPClient http;
  http.setConnectTimeout(10000);
  http.setTimeout(20000);
  http.useHTTP10(true);

  if (!http.begin(url)) {
    Serial.println("[http] begin failed");
    return false;
  }

  http.addHeader("User-Agent", "ESP32-WeatherStation-Final");

  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    Serial.printf("[http] error %d\n", code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  Serial.printf("[http] received %u bytes\n", payload.length());

  if (payload.length() == 0) {
    Serial.println("[http] empty response");
    return false;
  }

  DeserializationError error = deserializeJson(document, payload);

  if (error) {
    Serial.print("[json] ");
    Serial.println(error.c_str());
    return false;
  }

  return true;
}

// ============================================================
// 24. WEATHER API
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
    "rain,"
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

int findCurrentHourIndex(JsonArray times, const String &currentTime) {
  if (times.size() == 0 || currentTime.length() < 13) return 0;

  String hourKey = currentTime.substring(0, 13);

  for (int i = 0; i < (int)times.size(); i++) {
    String timeValue = times[i].as<String>();

    if (timeValue.startsWith(hourKey)) {
      return i;
    }
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

  snprintf(
    weather.sourceTime,
    sizeof(weather.sourceTime),
    "%s",
    currentTime.c_str()
  );

  weather.temperature   = current["temperature_2m"] | NAN;
  weather.apparent      = current["apparent_temperature"] | NAN;
  weather.humidity      = current["relative_humidity_2m"] | NAN;
  weather.dewPoint      = current["dew_point_2m"] | NAN;
  weather.pressure      = current["pressure_msl"] | NAN;
  weather.wind          = current["wind_speed_10m"] | NAN;
  weather.windDirection = current["wind_direction_10m"] | NAN;
  weather.gust          = current["wind_gusts_10m"] | NAN;
  weather.precipitation = current["precipitation"] | NAN;
  weather.rain          = current["rain"] | NAN;
  weather.weatherCode   = current["weather_code"] | -1;
  weather.visibility    = current["visibility"] | NAN;
  weather.uv            = current["uv_index"] | NAN;
  weather.daytime       = (current["is_day"] | 1) == 1;

  JsonObject hourly = document["hourly"];
  JsonArray times = hourly["time"];

  int currentIndex = findCurrentHourIndex(times, currentTime);

  // Six-hour pressure trend uses actual past hourly data.
  weather.pressureDelta6h = NAN;

  if (currentIndex >= 6) {
    float oldPressure = hourly["pressure_msl"][currentIndex - 6] | NAN;
    float nowPressure = hourly["pressure_msl"][currentIndex] | NAN;

    if (!isnan(oldPressure) && !isnan(nowPressure)) {
      weather.pressureDelta6h = nowPressure - oldPressure;
    }
  }

  weather.hourlyCount = 0;

  for (int source = currentIndex;
       source < (int)times.size() && weather.hourlyCount < 24;
       source++) {
    int target = weather.hourlyCount++;
    String timestamp = times[source].as<String>();

    if (timestamp.length() >= 16) {
      snprintf(
        weather.hourly[target].time,
        sizeof(weather.hourly[target].time),
        "%s",
        timestamp.substring(11, 16).c_str()
      );
    }

    weather.hourly[target].temperature =
      hourly["temperature_2m"][source] | NAN;

    weather.hourly[target].apparent =
      hourly["apparent_temperature"][source] | NAN;

    weather.hourly[target].pressure =
      hourly["pressure_msl"][source] | NAN;

    weather.hourly[target].wind =
      hourly["wind_speed_10m"][source] | NAN;

    weather.hourly[target].windDirection =
      hourly["wind_direction_10m"][source] | NAN;

    weather.hourly[target].gust =
      hourly["wind_gusts_10m"][source] | NAN;

    weather.hourly[target].precipitation =
      hourly["precipitation"][source] | NAN;

    weather.hourly[target].rain =
      hourly["rain"][source] | NAN;

    weather.hourly[target].rainChance =
      hourly["precipitation_probability"][source] | 0;

    weather.hourly[target].weatherCode =
      hourly["weather_code"][source] | -1;
  }

  JsonObject daily = document["daily"];
  JsonArray dates = daily["time"];

  weather.dailyCount = min(7, (int)dates.size());

  for (int i = 0; i < weather.dailyCount; i++) {
    String date = dates[i].as<String>();

    snprintf(
      weather.daily[i].date,
      sizeof(weather.daily[i].date),
      "%s",
      date.c_str()
    );

    weather.daily[i].minimum =
      daily["temperature_2m_min"][i] | NAN;

    weather.daily[i].maximum =
      daily["temperature_2m_max"][i] | NAN;

    weather.daily[i].precipitation =
      daily["precipitation_sum"][i] | NAN;

    weather.daily[i].rainChance =
      daily["precipitation_probability_max"][i] | 0;

    weather.daily[i].weatherCode =
      daily["weather_code"][i] | -1;

    String sunrise = daily["sunrise"][i] | "";
    String sunset = daily["sunset"][i] | "";

    if (sunrise.length() >= 16) {
      snprintf(
        weather.daily[i].sunrise,
        sizeof(weather.daily[i].sunrise),
        "%s",
        sunrise.substring(11, 16).c_str()
      );
    }

    if (sunset.length() >= 16) {
      snprintf(
        weather.daily[i].sunset,
        sizeof(weather.daily[i].sunset),
        "%s",
        sunset.substring(11, 16).c_str()
      );
    }
  }

  weather.valid = true;
  weather.fetchedAt = time(nullptr);
  lastWeatherUpdate = millis();

  Serial.printf(
    "[weather] %.1f C, %s, %d hourly, %d daily\n",
    weather.temperature,
    weatherDescription(weather.weatherCode).c_str(),
    weather.hourlyCount,
    weather.dailyCount
  );

  return true;
}

// ============================================================
// 25. AIR QUALITY API
// ============================================================

String airURL() {
  String url =
    "https://air-quality-api.open-meteo.com/v1/air-quality";

  url += "?latitude=" + String(LATITUDE, 6);
  url += "&longitude=" + String(LONGITUDE, 6);

  // Direct current values avoids V1's hourly[0] problem.
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

  air.pm25        = current["pm2_5"] | NAN;
  air.pm10        = current["pm10"] | NAN;
  air.co          = current["carbon_monoxide"] | NAN;
  air.no2         = current["nitrogen_dioxide"] | NAN;
  air.so2         = current["sulphur_dioxide"] | NAN;
  air.ozone       = current["ozone"] | NAN;
  air.europeanAqi = current["european_aqi"] | -1;
  air.usAqi       = current["us_aqi"] | -1;

  air.valid = air.usAqi >= 0 || !isnan(air.pm25);
  air.fetchedAt = time(nullptr);
  lastAirUpdate = millis();

  Serial.printf("[air] AQI %d, PM2.5 %.1f\n", air.usAqi, air.pm25);

  return air.valid;
}

// ============================================================
// 26. OFFLINE CACHE
// ============================================================

void saveCache() {
  if (!weather.valid) return;

  preferences.begin("weather-final", false);

  preferences.putBool("valid", true);
  preferences.putFloat("temp", weather.temperature);
  preferences.putFloat("apparent", weather.apparent);
  preferences.putFloat("humidity", weather.humidity);
  preferences.putFloat("dew", weather.dewPoint);
  preferences.putFloat("pressure", weather.pressure);
  preferences.putFloat("wind", weather.wind);
  preferences.putFloat("gust", weather.gust);
  preferences.putFloat("direction", weather.windDirection);
  preferences.putFloat("uv", weather.uv);
  preferences.putInt("code", weather.weatherCode);
  preferences.putBool("day", weather.daytime);
  preferences.putLong64("time", (int64_t)weather.fetchedAt);

  preferences.end();
}

void loadCache() {
  preferences.begin("weather-final", true);

  if (!preferences.getBool("valid", false)) {
    preferences.end();
    return;
  }

  weather.temperature =
    preferences.getFloat("temp", NAN);

  weather.apparent =
    preferences.getFloat("apparent", NAN);

  weather.humidity =
    preferences.getFloat("humidity", NAN);

  weather.dewPoint =
    preferences.getFloat("dew", NAN);

  weather.pressure =
    preferences.getFloat("pressure", NAN);

  weather.wind =
    preferences.getFloat("wind", NAN);

  weather.gust =
    preferences.getFloat("gust", NAN);

  weather.windDirection =
    preferences.getFloat("direction", NAN);

  weather.uv =
    preferences.getFloat("uv", NAN);

  weather.weatherCode =
    preferences.getInt("code", -1);

  weather.daytime =
    preferences.getBool("day", true);

  weather.fetchedAt =
    (time_t)preferences.getLong64("time", 0);

  weather.valid = true;

  preferences.end();

  Serial.println("[cache] restored current conditions");
}

// ============================================================
// 27. BUTTON
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

  if (state == LOW &&
      !longPressHandled &&
      millis() - buttonDownAt >= 900) {
    longPressHandled = true;
    currentPage = PAGE_NOW;
    drawPage();
  }

  if (previousButtonState == LOW && state == HIGH) {
    uint32_t held = millis() - buttonDownAt;

    if (!longPressHandled && held >= 30) {
      currentPage = (Page)(((int)currentPage + 1) % PAGE_COUNT);
      drawPage();
    }
  }

  previousButtonState = state;
}

// ============================================================
// 28. SCHEDULED WORK
// ============================================================

void updateDataIfNeeded() {
  if (!wifiConnected) return;

  if (!weather.valid ||
      millis() - lastWeatherUpdate >= WEATHER_REFRESH_MS) {
    if (updateWeather()) {
      saveCache();
      drawPage();
    }
  }

  if (!air.valid ||
      millis() - lastAirUpdate >= AIR_REFRESH_MS) {
    if (updateAir()) {
      drawPage();
    }
  }
}

void historyIfNeeded() {
  if (!ENABLE_SD_LOGGING || !sdAvailable) return;

  if (millis() - lastHistoryWrite < HISTORY_LOG_MS) return;

  saveHistory();
  lastHistoryWrite = millis();
}

void uiTickIfNeeded() {
  if (millis() - lastUiTick < UI_TICK_MS) return;

  lastUiTick = millis();

  static String previousMinute;
  String currentMinute = localTimeText();

  if (currentMinute != previousMinute) {
    previousMinute = currentMinute;
    drawPage();
  }
}

void autoPageIfNeeded() {
  if (!AUTO_PAGE_ROTATION) return;

  if (millis() - lastPageChange >= AUTO_PAGE_MS) {
    currentPage = (Page)(((int)currentPage + 1) % PAGE_COUNT);
    drawPage();
  }
}

// ============================================================
// 29. SETUP
// ============================================================

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println();
  Serial.println("================================");
  Serial.println(" WEATHER STATION FINAL");
  Serial.println(" Waveshare ESP32-S3-LCD-2");
  Serial.println("================================");

  pinMode(USER_BUTTON, INPUT_PULLUP);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, HIGH);

  analogReadResolution(12);

  // Restore useful information before any network work.
  loadCache();

  // First paint comes before WiFi.
  initializeDisplay();
  drawPage();
  lv_timer_handler();

  initializeSD();
  setupTime();
  startWiFi();

  lastPageChange = millis();

  Serial.println("[boot] ready");
}

// ============================================================
// 30. LOOP
// ============================================================

void loop() {
  maintainWiFi();
  handleButton();
  updateDataIfNeeded();
  historyIfNeeded();
  uiTickIfNeeded();
  autoPageIfNeeded();

  lv_timer_handler();
  delay(5);
}
