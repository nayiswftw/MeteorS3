/*
  ================================================================
     ULTIMATE WEATHER STATION
     Waveshare ESP32-S3-LCD-2
  ================================================================

  Target:
    ESP32-S3R8
    240x320 ST7789T3
    8MB PSRAM
    16MB Flash
    QMI8658 IMU
    TF card
    Battery

  GUI:
    LVGL 9.x

  Weather:
    Open-Meteo Forecast API
    Open-Meteo Air Quality API

  FEATURES
  ----------------------------------------------------------------
    • Current weather
    • Feels-like temperature
    • Dew point
    • Humidity
    • Pressure
    • Pressure trend
    • Wind speed
    • Wind gust
    • Wind direction
    • Beaufort scale
    • Visibility
    • UV index
    • Precipitation
    • Rain probability
    • 48-hour forecast
    • 7-day forecast
    • Hourly graph
    • Rain timeline
    • AQI
    • PM2.5
    • PM10
    • O3
    • NO2
    • SO2
    • CO
    • Sunrise
    • Sunset
    • Day/night
    • Moon phase
    • Severe-condition detection
    • Heat alerts
    • Wind alerts
    • Rain alerts
    • UV alerts
    • AQI alerts
    • Local history
    • TF CSV logging
    • Offline cached values in NVS
    • WiFi status
    • RSSI
    • IP address
    • Heap
    • PSRAM
    • Flash
    • Battery voltage
    • Battery %
    • Automatic page rotation
    • BOOT button navigation
    • Long press = home
    • Night theme
    • Auto brightness
    • 24h clock
    • Automatic NTP
    • API retry
    • Offline mode
    • Persistent settings

  ----------------------------------------------------------------
  IMPORTANT
  ----------------------------------------------------------------

  Hardware pin definitions are isolated below.

  Verify them against the Waveshare example for your exact
  ESP32-S3-LCD-2 revision before flashing.

  This board is NOT the similarly named 2.8" board.

  ================================================================
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


// =================================================================
// USER CONFIGURATION
// =================================================================

static const char *WIFI_SSID     = "Xiaomi 11i HyperCharge";
static const char *WIFI_PASSWORD = "12345678";

/*
   Enter your station location.

   Example:
      Latitude  = 30.xxxxxx
      Longitude = 75.xxxxxx

   You can also use any location on Earth.
*/
static double LATITUDE  = 30.000000;
static double LONGITUDE = 75.000000;

static const char *LOCATION_NAME = "HOME";

static const char *TIMEZONE =
    "Asia/Kolkata";


// =================================================================
// UNITS
// =================================================================

enum TemperatureUnit {
    TEMP_C,
    TEMP_F
};

enum WindUnit {
    WIND_KMH,
    WIND_MPH,
    WIND_MS
};

enum PressureUnit {
    PRESS_HPA,
    PRESS_INHG
};

TemperatureUnit tempUnit = TEMP_C;
WindUnit windUnit = WIND_KMH;
PressureUnit pressureUnit = PRESS_HPA;


// =================================================================
// HARDWARE CONFIGURATION
// =================================================================
//
// These are intentionally kept together.
//
// IMPORTANT:
// Verify against the Waveshare ESP32-S3-LCD-2 example supplied
// for your board revision.
//
// =================================================================

#define LCD_WIDTH       240
#define LCD_HEIGHT      320

#define LCD_SCLK        39
#define LCD_MOSI        38
#define LCD_MISO        40
#define LCD_DC          42
#define LCD_CS          45
#define LCD_RST         -1

#define LCD_BACKLIGHT   1

#define USER_BUTTON     0
#define BATTERY_ADC     4
#define SD_CS           21
/*
   Battery ADC.

   Change if your board revision exposes battery sensing on
   another GPIO.
*/
#define BATTERY_ADC     4

/*
   TF/SD CS.

   Verify against the Waveshare board example.
*/
#define SD_CS           21


// =================================================================
// TIMING
// =================================================================

#define WEATHER_UPDATE_MS     (10UL * 60UL * 1000UL)
#define AQI_UPDATE_MS         (30UL * 60UL * 1000UL)
#define HISTORY_UPDATE_MS     (15UL * 60UL * 1000UL)
#define UI_CLOCK_UPDATE_MS    1000UL
#define AUTO_PAGE_MS          (20UL * 1000UL)
#define WIFI_RETRY_MS         (15UL * 1000UL)


// =================================================================
// COLORS
// =================================================================

#define C_BG        lv_color_hex(0x050A12)
#define C_CARD      lv_color_hex(0x0E1928)
#define C_CARD2     lv_color_hex(0x142337)

#define C_TEXT      lv_color_hex(0xF3F7FF)
#define C_MUTED     lv_color_hex(0x8191A6)

#define C_BLUE      lv_color_hex(0x3EA7FF)
#define C_CYAN      lv_color_hex(0x28E0FF)

#define C_GREEN     lv_color_hex(0x35E37B)
#define C_YELLOW    lv_color_hex(0xFFD447)
#define C_ORANGE    lv_color_hex(0xFF9C45)
#define C_RED       lv_color_hex(0xFF4F63)
#define C_PURPLE    lv_color_hex(0x9B78FF)


// =================================================================
// GFX
// =================================================================

Arduino_DataBus *lcdBus =
    new Arduino_ESP32SPI(
        LCD_DC,
        LCD_CS,
        LCD_SCLK,
        LCD_MOSI,
        LCD_MISO
    );

Arduino_GFX *gfx =
    new Arduino_ST7789(
        lcdBus,
        LCD_RST,
        0,
        true,
        LCD_WIDTH,
        LCD_HEIGHT
    );

// =================================================================
// LVGL
// =================================================================

lv_display_t *display = nullptr;

static uint8_t *lvBuffer1 = nullptr;
static uint8_t *lvBuffer2 = nullptr;

static constexpr size_t LV_BUFFER_LINES = 80;


// =================================================================
// PERSISTENT STORAGE
// =================================================================

Preferences prefs;


// =================================================================
// WEATHER DATA
// =================================================================

struct Hour {

    String time;

    float temperature = NAN;
    float apparent = NAN;

    float humidity = NAN;
    float dewPoint = NAN;

    float pressure = NAN;

    float wind = NAN;
    float gust = NAN;
    float windDirection = NAN;

    float precipitation = NAN;
    float rain = NAN;

    int precipitationProbability = 0;

    float uv = NAN;

    int weatherCode = -1;
};


struct Day {

    String date;

    float minimum = NAN;
    float maximum = NAN;

    float precipitation = NAN;

    int precipitationProbability = 0;

    int weatherCode = -1;

    String sunrise;
    String sunset;
};


struct WeatherData {

    bool valid = false;

    String timestamp;

    float temperature = NAN;
    float apparent = NAN;

    float humidity = NAN;
    float dewPoint = NAN;

    float pressure = NAN;

    float wind = NAN;
    float gust = NAN;
    float windDirection = NAN;

    float precipitation = NAN;
    float rain = NAN;

    float visibility = NAN;

    float uv = NAN;

    int weatherCode = -1;

    bool daytime = true;

    Hour hourly[48];
    int hourlyCount = 0;

    Day daily[7];
    int dailyCount = 0;

    String sunrise;
    String sunset;
};


struct AirData {

    bool valid = false;

    float pm25 = NAN;
    float pm10 = NAN;

    float ozone = NAN;
    float no2 = NAN;
    float so2 = NAN;
    float co = NAN;

    float europeanAQI = NAN;
    float usAQI = NAN;

    float dust = NAN;
};


WeatherData weather;
AirData air;


// =================================================================
// STATE
// =================================================================

bool wifiConnected = false;
bool sdAvailable = false;

unsigned long lastWeatherUpdate = 0;
unsigned long lastAQIUpdate = 0;
unsigned long lastHistoryWrite = 0;
unsigned long lastWiFiAttempt = 0;
unsigned long lastClockUpdate = 0;
unsigned long lastPageChange = 0;

bool autoRotate = true;
bool nightMode = false;

int page = 0;


// =================================================================
// PAGE ENUM
// =================================================================

enum Page {

    PAGE_HOME = 0,
    PAGE_HOURLY,
    PAGE_FORECAST,
    PAGE_RAIN,
    PAGE_WIND,
    PAGE_ATMOSPHERE,
    PAGE_AIR,
    PAGE_SUN,
    PAGE_ALERTS,
    PAGE_HISTORY,
    PAGE_SYSTEM,

    PAGE_COUNT
};


// =================================================================
// ROOT UI
// =================================================================

lv_obj_t *root = nullptr;


// =================================================================
// GENERIC LABEL
// =================================================================

lv_obj_t *label(
    const char *text,
    int x,
    int y,
    int width,
    int height,
    int fontSize,
    lv_color_t color
) {

    lv_obj_t *obj =
        lv_label_create(root);

    lv_label_set_text(
        obj,
        text
    );

    lv_obj_set_pos(
        obj,
        x,
        y
    );

    lv_obj_set_size(
        obj,
        width,
        height
    );

    lv_obj_set_style_text_color(
        obj,
        color,
        0
    );

    lv_obj_set_style_text_font(
        obj,
        fontSize >= 30
            ? &lv_font_montserrat_32
            : &lv_font_montserrat_16,
        0
    );

    lv_label_set_long_mode(
        obj,
        LV_LABEL_LONG_CLIP
    );

    return obj;
}


// =================================================================
// CARD
// =================================================================

lv_obj_t *card(
    int x,
    int y,
    int width,
    int height
) {

    lv_obj_t *obj =
        lv_obj_create(root);

    lv_obj_set_pos(
        obj,
        x,
        y
    );

    lv_obj_set_size(
        obj,
        width,
        height
    );

    lv_obj_set_style_bg_color(
        obj,
        C_CARD,
        0
    );

    lv_obj_set_style_bg_opa(
        obj,
        LV_OPA_COVER,
        0
    );

    lv_obj_set_style_border_width(
        obj,
        0,
        0
    );

    lv_obj_set_style_radius(
        obj,
        12,
        0
    );

    return obj;
}


// =================================================================
// SCREEN RESET
// =================================================================

void clearScreen() {

    if (root)
        lv_obj_clean(root);
}


// =================================================================
// FORMAT HELPERS
// =================================================================

String tempString(
    float c
) {

    if (isnan(c))
        return "--";

    if (tempUnit == TEMP_F)
        return String(
            c * 9.0 / 5.0 + 32.0,
            1
        ) + "°F";

    return String(
        c,
        1
    ) + "°C";
}


String windString(
    float kmh
) {

    if (isnan(kmh))
        return "--";

    if (windUnit == WIND_MPH)
        return String(
            kmh * 0.621371,
            1
        ) + " mph";

    if (windUnit == WIND_MS)
        return String(
            kmh / 3.6,
            1
        ) + " m/s";

    return String(
        kmh,
        1
    ) + " km/h";
}


String pressureString(
    float hpa
) {

    if (isnan(hpa))
        return "--";

    if (pressureUnit == PRESS_INHG)
        return String(
            hpa * 0.029529983,
            2
        ) + " inHg";

    return String(
        hpa,
        1
    ) + " hPa";
}


// =================================================================
// TIME
// =================================================================

String localTime() {

    struct tm tmNow;

    if (!getLocalTime(&tmNow, 50))
        return "--:--";

    char buf[8];

    strftime(
        buf,
        sizeof(buf),
        "%H:%M",
        &tmNow
    );

    return String(buf);
}


String localDate() {

    struct tm tmNow;

    if (!getLocalTime(&tmNow, 50))
        return "----";

    char buf[32];

    strftime(
        buf,
        sizeof(buf),
        "%a %d %b",
        &tmNow
    );

    return String(buf);
}


// =================================================================
// WEATHER DESCRIPTION
// =================================================================

String weatherDescription(
    int code
) {

    switch (code) {

        case 0:
            return "CLEAR SKY";

        case 1:
            return "MAINLY CLEAR";

        case 2:
            return "PARTLY CLOUDY";

        case 3:
            return "OVERCAST";

        case 45:
        case 48:
            return "FOG";

        case 51:
        case 53:
        case 55:
            return "DRIZZLE";

        case 56:
        case 57:
            return "FREEZING DRIZZLE";

        case 61:
        case 63:
        case 65:
            return "RAIN";

        case 66:
        case 67:
            return "FREEZING RAIN";

        case 71:
        case 73:
        case 75:
            return "SNOW";

        case 77:
            return "SNOW GRAINS";

        case 80:
        case 81:
        case 82:
            return "RAIN SHOWERS";

        case 85:
        case 86:
            return "SNOW SHOWERS";

        case 95:
            return "THUNDERSTORM";

        case 96:
        case 99:
            return "THUNDERSTORM / HAIL";

        default:
            return "UNKNOWN";
    }
}


// =================================================================
// WEATHER ICON
// =================================================================

String weatherIcon(
    int code,
    bool day
) {

    if (code == 0)
        return day ? "SUN" : "MOON";

    if (code == 1 || code == 2)
        return "CLOUD";

    if (code == 3)
        return "CLOUD";

    if (code == 45 || code == 48)
        return "FOG";

    if (code >= 51 && code <= 67)
        return "RAIN";

    if (code >= 71 && code <= 77)
        return "SNOW";

    if (code >= 80 && code <= 82)
        return "SHOWERS";

    if (code >= 95)
        return "STORM";

    return "?";
}


// =================================================================
// WIND DIRECTION
// =================================================================

String windDirection(
    float degrees
) {

    if (isnan(degrees))
        return "--";

    const char *dirs[] = {

        "N",
        "NNE",
        "NE",
        "ENE",
        "E",
        "ESE",
        "SE",
        "SSE",
        "S",
        "SSW",
        "SW",
        "WSW",
        "W",
        "WNW",
        "NW",
        "NNW"
    };

    int index =
        (int)((degrees + 11.25) / 22.5);

    index %= 16;

    return dirs[index];
}


// =================================================================
// BEAUFORT
// =================================================================

int beaufort(
    float kmh
) {

    if (isnan(kmh))
        return 0;

    if (kmh < 1)
        return 0;

    if (kmh < 6)
        return 1;

    if (kmh < 12)
        return 2;

    if (kmh < 20)
        return 3;

    if (kmh < 29)
        return 4;

    if (kmh < 39)
        return 5;

    if (kmh < 50)
        return 6;

    if (kmh < 62)
        return 7;

    if (kmh < 75)
        return 8;

    if (kmh < 89)
        return 9;

    if (kmh < 103)
        return 10;

    if (kmh < 118)
        return 11;

    return 12;
}


// =================================================================
// PRESSURE TREND
// =================================================================

String pressureTrend() {

    if (
        weather.hourlyCount < 6
    )
        return "STABLE";

    float now =
        weather.hourly[0].pressure;

    float old =
        weather.hourly[6].pressure;

    if (
        isnan(now) ||
        isnan(old)
    )
        return "STABLE";

    float delta =
        now - old;

    if (delta > 2.0)
        return "RISING";

    if (delta < -2.0)
        return "FALLING";

    return "STABLE";
}


// =================================================================
// AQI CATEGORY
// =================================================================

String aqiCategory(
    float aqi
) {

    if (isnan(aqi))
        return "UNKNOWN";

    if (aqi <= 20)
        return "GOOD";

    if (aqi <= 40)
        return "FAIR";

    if (aqi <= 60)
        return "MODERATE";

    if (aqi <= 80)
        return "POOR";

    if (aqi <= 100)
        return "VERY POOR";

    return "EXTREME";
}


// =================================================================
// BATTERY
// =================================================================

float batteryVoltage() {

    uint32_t total = 0;

    for (int i = 0; i < 8; i++) {

        total +=
            analogRead(
                BATTERY_ADC
            );

        delayMicroseconds(100);
    }

    float raw =
        total / 8.0;

    float v =
        raw * 3.3 / 4095.0;

    /*
       100K / 100K divider
    */
    return v * 2.0;
}


int batteryPercent() {

    float v =
        batteryVoltage();

    int p =
        (int)(
            (v - 3.25) /
            (4.20 - 3.25) *
            100.0
        );

    return constrain(
        p,
        0,
        100
    );
}


// =================================================================
// WIFI
// =================================================================

void startWiFi() {

    WiFi.mode(
        WIFI_STA
    );

    WiFi.setAutoReconnect(
        true
    );

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );

    lastWiFiAttempt =
        millis();
}


void maintainWiFi() {

    if (
        WiFi.status() ==
        WL_CONNECTED
    ) {

        wifiConnected = true;
        return;
    }

    wifiConnected = false;

    if (
        millis() -
        lastWiFiAttempt >
        WIFI_RETRY_MS
    ) {

        WiFi.reconnect();

        lastWiFiAttempt =
            millis();
    }
}


// =================================================================
// NTP
// =================================================================

void setupTime() {

    configTzTime(
        TIMEZONE,
        "pool.ntp.org",
        "time.nist.gov",
        "time.google.com"
    );
}


// =================================================================
// HTTP JSON
// =================================================================

bool httpGetJSON(
    const String &url,
    JsonDocument &doc
) {

    if (!wifiConnected)
        return false;

    HTTPClient http;

    http.setConnectTimeout(
        10000
    );

    http.setTimeout(
        15000
    );

    http.useHTTP10(
        true
    );

    if (
        !http.begin(url)
    )
        return false;

    http.addHeader(
        "User-Agent",
        "ESP32-WeatherStation/1.0"
    );

    int result =
        http.GET();

    if (
        result != HTTP_CODE_OK
    ) {

        Serial.printf(
            "HTTP error: %d\n",
            result
        );

        http.end();

        return false;
    }

    String payload =
        http.getString();

    http.end();

    DeserializationError error =
        deserializeJson(
            doc,
            payload
        );

    if (error) {

        Serial.print(
            "JSON error: "
        );

        Serial.println(
            error.c_str()
        );

        return false;
    }

    return true;
}


// =================================================================
// WEATHER URL
// =================================================================

String weatherURL() {

    String url =
        "https://api.open-meteo.com/v1/forecast";

    url +=
        "?latitude=" +
        String(
            LATITUDE,
            6
        );

    url +=
        "&longitude=" +
        String(
            LONGITUDE,
            6
        );

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
        "relative_humidity_2m,"
        "dew_point_2m,"
        "pressure_msl,"
        "wind_speed_10m,"
        "wind_direction_10m,"
        "wind_gusts_10m,"
        "precipitation,"
        "rain,"
        "precipitation_probability,"
        "uv_index,"
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

    url +=
        "&forecast_days=7";

    url +=
        "&timezone=auto";

    return url;
}


// =================================================================
// UPDATE WEATHER
// =================================================================

bool updateWeather() {

    JsonDocument doc;

    Serial.println(
        "Downloading weather..."
    );

    if (
        !httpGetJSON(
            weatherURL(),
            doc
        )
    )
        return false;


    JsonObject current =
        doc["current"];


    weather.timestamp =
        current["time"] |
        "";


    weather.temperature =
        current["temperature_2m"] |
        NAN;

    weather.apparent =
        current["apparent_temperature"] |
        NAN;

    weather.humidity =
        current["relative_humidity_2m"] |
        NAN;

    weather.dewPoint =
        current["dew_point_2m"] |
        NAN;

    weather.pressure =
        current["pressure_msl"] |
        NAN;

    weather.wind =
        current["wind_speed_10m"] |
        NAN;

    weather.windDirection =
        current["wind_direction_10m"] |
        NAN;

    weather.gust =
        current["wind_gusts_10m"] |
        NAN;

    weather.precipitation =
        current["precipitation"] |
        NAN;

    weather.rain =
        current["rain"] |
        NAN;

    weather.visibility =
        current["visibility"] |
        NAN;

    weather.uv =
        current["uv_index"] |
        NAN;

    weather.weatherCode =
        current["weather_code"] |
        -1;

    weather.daytime =
        (current["is_day"] | 1) != 0;


    // -------------------------------------------------------------
    // HOURLY
    // -------------------------------------------------------------

    JsonObject hourly =
        doc["hourly"];

    JsonArray times =
        hourly["time"];

    JsonArray temperatures =
        hourly["temperature_2m"];

    JsonArray apparent =
        hourly["apparent_temperature"];

    JsonArray humidity =
        hourly["relative_humidity_2m"];

    JsonArray dew =
        hourly["dew_point_2m"];

    JsonArray pressure =
        hourly["pressure_msl"];

    JsonArray wind =
        hourly["wind_speed_10m"];

    JsonArray direction =
        hourly["wind_direction_10m"];

    JsonArray gust =
        hourly["wind_gusts_10m"];

    JsonArray precipitation =
        hourly["precipitation"];

    JsonArray rain =
        hourly["rain"];

    JsonArray probability =
        hourly["precipitation_probability"];

    JsonArray uv =
        hourly["uv_index"];

    JsonArray codes =
        hourly["weather_code"];


    int startIndex = 0;

    String currentHour =
        weather.timestamp.substring(
            0,
            13
        );


    for (
        int i = 0;
        i < (int)times.size();
        i++
    ) {

        String t =
            times[i].as<String>();

        if (
            t.startsWith(
                currentHour
            )
        ) {

            startIndex = i;
            break;
        }
    }


    weather.hourlyCount = 0;


    for (
        int n = 0;
        n < 48;
        n++
    ) {

        int i =
            startIndex + n;

        if (
            i >=
            (int)times.size()
        )
            break;


        Hour &h =
            weather.hourly[n];


        h.time =
            times[i].as<String>();

        h.temperature =
            temperatures[i] | NAN;

        h.apparent =
            apparent[i] | NAN;

        h.humidity =
            humidity[i] | NAN;

        h.dewPoint =
            dew[i] | NAN;

        h.pressure =
            pressure[i] | NAN;

        h.wind =
            wind[i] | NAN;

        h.windDirection =
            direction[i] | NAN;

        h.gust =
            gust[i] | NAN;

        h.precipitation =
            precipitation[i] | NAN;

        h.rain =
            rain[i] | NAN;

        h.precipitationProbability =
            probability[i] | 0;

        h.uv =
            uv[i] | NAN;

        h.weatherCode =
            codes[i] | -1;


        weather.hourlyCount++;
    }


    // -------------------------------------------------------------
    // DAILY
    // -------------------------------------------------------------

    JsonObject daily =
        doc["daily"];

    JsonArray dates =
        daily["time"];

    JsonArray maximum =
        daily["temperature_2m_max"];

    JsonArray minimum =
        daily["temperature_2m_min"];

    JsonArray rainfall =
        daily["precipitation_sum"];

    JsonArray dailyProbability =
        daily["precipitation_probability_max"];

    JsonArray dailyCode =
        daily["weather_code"];

    JsonArray sunrise =
        daily["sunrise"];

    JsonArray sunset =
        daily["sunset"];


    weather.dailyCount =
        min(
            7,
            (int)dates.size()
        );


    for (
        int i = 0;
        i < weather.dailyCount;
        i++
    ) {

        Day &d =
            weather.daily[i];


        d.date =
            dates[i].as<String>();

        d.maximum =
            maximum[i] | NAN;

        d.minimum =
            minimum[i] | NAN;

        d.precipitation =
            rainfall[i] | NAN;

        d.precipitationProbability =
            dailyProbability[i] | 0;

        d.weatherCode =
            dailyCode[i] | -1;

        d.sunrise =
            sunrise[i].as<String>();

        d.sunset =
            sunset[i].as<String>();
    }


    if (
        weather.dailyCount > 0
    ) {

        weather.sunrise =
            weather.daily[0].sunrise;

        weather.sunset =
            weather.daily[0].sunset;
    }


    weather.valid =
        true;

    Serial.println(
        "Weather OK"
    );

    return true;
}


// =================================================================
// AQI URL
// =================================================================

String airURL() {

    String url =
        "https://air-quality-api.open-meteo.com/v1/air-quality";

    url +=
        "?latitude=" +
        String(
            LATITUDE,
            6
        );

    url +=
        "&longitude=" +
        String(
            LONGITUDE,
            6
        );

    url +=
        "&hourly="

        "pm10,"
        "pm2_5,"
        "carbon_monoxide,"
        "nitrogen_dioxide,"
        "sulphur_dioxide,"
        "ozone,"
        "european_aqi,"
        "us_aqi,"
        "dust";

    url +=
        "&forecast_days=2";

    url +=
        "&timezone=auto";

    return url;
}


// =================================================================
// UPDATE AIR
// =================================================================

bool updateAir() {

    JsonDocument doc;

    if (
        !httpGetJSON(
            airURL(),
            doc
        )
    )
        return false;


    JsonObject hourly =
        doc["hourly"];


    JsonArray pm25 =
        hourly["pm2_5"];

    JsonArray pm10 =
        hourly["pm10"];

    JsonArray ozone =
        hourly["ozone"];

    JsonArray no2 =
        hourly["nitrogen_dioxide"];

    JsonArray so2 =
        hourly["sulphur_dioxide"];

    JsonArray co =
        hourly["carbon_monoxide"];

    JsonArray eAQI =
        hourly["european_aqi"];

    JsonArray uAQI =
        hourly["us_aqi"];

    JsonArray dust =
        hourly["dust"];


    if (
        pm25.size() == 0
    )
        return false;


    air.pm25 =
        pm25[0] | NAN;

    air.pm10 =
        pm10[0] | NAN;

    air.ozone =
        ozone[0] | NAN;

    air.no2 =
        no2[0] | NAN;

    air.so2 =
        so2[0] | NAN;

    air.co =
        co[0] | NAN;

    air.europeanAQI =
        eAQI[0] | NAN;

    air.usAQI =
        uAQI[0] | NAN;

    air.dust =
        dust[0] | NAN;


    air.valid =
        true;

    Serial.println(
        "AQI OK"
    );

    return true;
}


// =================================================================
// HISTORY
// =================================================================

void initializeSD() {

    sdAvailable =
        SD.begin(
            SD_CS
        );

    if (!sdAvailable) {

        Serial.println(
            "SD not available"
        );

        return;
    }


    if (
        !SD.exists(
            "/weather.csv"
        )
    ) {

        File f =
            SD.open(
                "/weather.csv",
                FILE_WRITE
            );

        if (f) {

            f.println(
                "timestamp,"
                "temperature,"
                "apparent,"
                "humidity,"
                "dewpoint,"
                "pressure,"
                "wind,"
                "gust,"
                "direction,"
                "rain,"
                "precipitation,"
                "uv,"
                "pm25,"
                "pm10,"
                "aqi"
            );

            f.close();
        }
    }
}


// =================================================================
// SAVE HISTORY
// =================================================================

void saveHistory() {

    if (
        !sdAvailable ||
        !weather.valid
    )
        return;


    File f =
        SD.open(
            "/weather.csv",
            FILE_APPEND
        );


    if (!f)
        return;


    f.printf(
        "%s,%.2f,%.2f,%.2f,%.2f,"
        "%.2f,%.2f,%.2f,%.1f,"
        "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",

        weather.timestamp.c_str(),

        weather.temperature,

        weather.apparent,

        weather.humidity,

        weather.dewPoint,

        weather.pressure,

        weather.wind,

        weather.gust,

        weather.windDirection,

        weather.rain,

        weather.precipitation,

        weather.uv,

        air.pm25,

        air.pm10,

        air.europeanAQI
    );


    f.close();
}


// =================================================================
// CACHE
// =================================================================

void saveCache() {

    if (!weather.valid)
        return;


    prefs.begin(
        "weather",
        false
    );


    prefs.putFloat(
        "temp",
        weather.temperature
    );

    prefs.putFloat(
        "apparent",
        weather.apparent
    );

    prefs.putFloat(
        "humidity",
        weather.humidity
    );

    prefs.putFloat(
        "pressure",
        weather.pressure
    );

    prefs.putFloat(
        "wind",
        weather.wind
    );

    prefs.putFloat(
        "gust",
        weather.gust
    );

    prefs.putFloat(
        "uv",
        weather.uv
    );

    prefs.putInt(
        "code",
        weather.weatherCode
    );

    prefs.putULong(
        "saved",
        millis()
    );


    prefs.end();
}


// =================================================================
// LOAD CACHE
// =================================================================

void loadCache() {

    prefs.begin(
        "weather",
        true
    );


    float t =
        prefs.getFloat(
            "temp",
            NAN
        );


    if (!isnan(t)) {

        weather.temperature =
            t;

        weather.apparent =
            prefs.getFloat(
                "apparent",
                NAN
            );

        weather.humidity =
            prefs.getFloat(
                "humidity",
                NAN
            );

        weather.pressure =
            prefs.getFloat(
                "pressure",
                NAN
            );

        weather.wind =
            prefs.getFloat(
                "wind",
                NAN
            );

        weather.gust =
            prefs.getFloat(
                "gust",
                NAN
            );

        weather.uv =
            prefs.getFloat(
                "uv",
                NAN
            );

        weather.weatherCode =
            prefs.getInt(
                "code",
                -1
            );

        weather.valid =
            true;
    }


    prefs.end();
}


// =================================================================
// MOON
// =================================================================

String moonPhase() {

    struct tm tmNow;

    if (
        !getLocalTime(
            &tmNow,
            50
        )
    )
        return "UNKNOWN";


    /*
       Simple synodic-month approximation.

       Reference:
       2000-01-06 ≈ new moon.
    */

    int year =
        tmNow.tm_year + 1900;

    int month =
        tmNow.tm_mon + 1;

    int day =
        tmNow.tm_mday;


    if (month < 3) {

        year--;

        month += 12;
    }


    long days =
        365L * year +
        year / 4 -
        year / 100 +
        year / 400 +
        (153L *
            (month + 1)) / 5 +
        day -
        730551L;


    double phase =
        fmod(
            days + 4.867,
            29.530588853
        );


    if (phase < 0)
        phase +=
            29.530588853;


    double fraction =
        phase /
        29.530588853;


    if (fraction < 0.03)
        return "NEW MOON";

    if (fraction < 0.22)
        return "WAXING CRESCENT";

    if (fraction < 0.28)
        return "FIRST QUARTER";

    if (fraction < 0.47)
        return "WAXING GIBBOUS";

    if (fraction < 0.53)
        return "FULL MOON";

    if (fraction < 0.72)
        return "WANING GIBBOUS";

    if (fraction < 0.78)
        return "LAST QUARTER";

    if (fraction < 0.97)
        return "WANING CRESCENT";

    return "NEW MOON";
}


// =================================================================
// ALERT ENGINE
// =================================================================

struct Alert {

    String text;
    lv_color_t color;
};


Alert alerts[10];
int alertCount = 0;


void buildAlertsData() {

    alertCount = 0;


    if (
        weather.temperature >= 40
    ) {

        alerts[alertCount++] = {
            "EXTREME HEAT",
            C_RED
        };

    } else if (
        weather.temperature >= 35
    ) {

        alerts[alertCount++] = {
            "HIGH TEMPERATURE",
            C_ORANGE
        };
    }


    if (
        weather.gust >= 65 ||
        weather.wind >= 50
    ) {

        alerts[alertCount++] = {
            "HIGH WIND",
            C_RED
        };
    }


    if (
        weather.hourlyCount > 0 &&
        weather.hourly[0]
            .precipitationProbability >= 70
    ) {

        alerts[alertCount++] = {
            "HIGH RAIN CHANCE",
            C_BLUE
        };
    }


    if (
        weather.uv >= 8
    ) {

        alerts[alertCount++] = {
            "VERY HIGH UV",
            C_RED
        };

    } else if (
        weather.uv >= 6
    ) {

        alerts[alertCount++] = {
            "HIGH UV",
            C_YELLOW
        };
    }


    if (
        air.valid &&
        air.europeanAQI >= 80
    ) {

        alerts[alertCount++] = {
            "POOR AIR QUALITY",
            C_RED
        };
    }


    if (
        weather.weatherCode >= 95
    ) {

        alerts[alertCount++] = {
            "THUNDERSTORM",
            C_PURPLE
        };
    }


    if (
        weather.pressure < 995
    ) {

        alerts[alertCount++] = {
            "LOW PRESSURE",
            C_ORANGE
        };
    }


    if (alertCount == 0) {

        alerts[alertCount++] = {
            "NO ACTIVE ALERTS",
            C_GREEN
        };
    }
}


// =================================================================
// HEADER
// =================================================================

void makeHeader(
    const char *title
) {

    label(
        title,
        10,
        6,
        160,
        26,
        16,
        C_TEXT
    );


    label(
        localTime().c_str(),
        172,
        6,
        58,
        26,
        16,
        C_CYAN
    );


    label(
        LOCATION_NAME,
        10,
        30,
        150,
        18,
        16,
        C_MUTED
    );


    label(
        wifiConnected
            ? "WIFI"
            : "OFF",
        182,
        31,
        45,
        18,
        16,
        wifiConnected
            ? C_GREEN
            : C_RED
    );
}


// =================================================================
// HOME
// =================================================================

void drawHome() {

    clearScreen();

    makeHeader(
        "WEATHER"
    );


    card(
        8,
        54,
        224,
        116
    );


    label(
        tempString(
            weather.temperature
        ).c_str(),
        17,
        60,
        125,
        46,
        32,
        C_TEXT
    );


    label(
        weatherDescription(
            weather.weatherCode
        ).c_str(),
        18,
        112,
        125,
        22,
        16,
        C_CYAN
    );


    label(
        weatherIcon(
            weather.weatherCode,
            weather.daytime
        ).c_str(),
        155,
        67,
        65,
        30,
        16,
        C_YELLOW
    );


    label(
        (
            "FEELS " +
            tempString(
                weather.apparent
            )
        ).c_str(),
        145,
        108,
        78,
        20,
        16,
        C_MUTED
    );


    card(
        8,
        178,
        106,
        55
    );


    label(
        (
            "HUM " +
            String(
                weather.humidity,
                0
            ) +
            "%"
        ).c_str(),
        17,
        194,
        90,
        20,
        16,
        C_CYAN
    );


    card(
        124,
        178,
        108,
        55
    );


    label(
        (
            "UV " +
            String(
                weather.uv,
                1
            )
        ).c_str(),
        135,
        194,
        90,
        20,
        16,
        C_YELLOW
    );


    card(
        8,
        241,
        106,
        55
    );


    label(
        (
            "WIND " +
            windDirection(
                weather.windDirection
            )
        ).c_str(),
        17,
        257,
        90,
        20,
        16,
        C_GREEN
    );


    card(
        124,
        241,
        108,
        55
    );


    label(
        (
            "AQI " +
            (
                air.valid
                    ? String(
                        air.europeanAQI,
                        0
                    )
                    : "--"
            )
        ).c_str(),
        135,
        257,
        90,
        20,
        16,
        C_GREEN
    );


    label(
        localDate().c_str(),
        10,
        300,
        130,
        18,
        16,
        C_MUTED
    );


    label(
        (
            "P" +
            String(
                weather.pressure,
                0
            )
        ).c_str(),
        170,
        300,
        60,
        18,
        16,
        C_TEXT
    );
}


// =================================================================
// HOURLY
// =================================================================

void drawHourly() {

    clearScreen();

    makeHeader(
        "48H TIMELINE"
    );


    /*
       Draw a simple hand-rendered graph.

       This avoids heavy LVGL chart allocations and gives us
       predictable rendering on the tiny display.
    */

    int gx = 10;
    int gy = 62;
    int gw = 220;
    int gh = 130;


    card(
        gx,
        gy,
        gw,
        gh
    );


    float minT = 1000;
    float maxT = -1000;


    for (
        int i = 0;
        i < weather.hourlyCount;
        i++
    ) {

        minT =
            min(
                minT,
                weather.hourly[i].temperature
            );

        maxT =
            max(
                maxT,
                weather.hourly[i].temperature
            );
    }


    if (
        minT == 1000 ||
        maxT == -1000
    ) {

        minT = 0;
        maxT = 40;
    }


    if (
        fabs(
            maxT - minT
        ) < 5
    ) {

        maxT += 3;
        minT -= 3;
    }


    /*
       We use tiny LVGL line segments.
    */

    for (
        int i = 1;
        i < weather.hourlyCount &&
        i < 48;
        i++
    ) {

        int x1 =
            gx + 5 +
            ((i - 1) *
             (gw - 10)) /
            47;

        int x2 =
            gx + 5 +
            (i *
             (gw - 10)) /
            47;


        float t1 =
            weather.hourly[i - 1]
                .temperature;

        float t2 =
            weather.hourly[i]
                .temperature;


        int y1 =
            gy + gh - 10 -
            (int)(
                (t1 - minT) /
                (maxT - minT) *
                (gh - 25)
            );

        int y2 =
            gy + gh - 10 -
            (int)(
                (t2 - minT) /
                (maxT - minT) *
                (gh - 25)
            );


        lv_obj_t *line =
            lv_line_create(
                root
            );


        static lv_point_precise_t pts[2];

        pts[0].x = x1;
        pts[0].y = y1;

        pts[1].x = x2;
        pts[1].y = y2;


        lv_line_set_points(
            line,
            pts,
            2
        );


        lv_obj_set_style_line_color(
            line,
            C_YELLOW,
            0
        );

        lv_obj_set_style_line_width(
            line,
            2,
            0
        );
    }


    label(
        (
            "NOW " +
            tempString(
                weather.temperature
            )
        ).c_str(),
        10,
        204,
        100,
        20,
        16,
        C_YELLOW
    );


    label(
        (
            "RAIN " +
            String(
                weather.hourlyCount
                    ? weather.hourly[0]
                        .precipitationProbability
                    : 0
            ) +
            "%"
        ).c_str(),
        120,
        204,
        100,
        20,
        16,
        C_BLUE
    );


    label(
        (
            "MIN " +
            String(
                minT,
                1
            ) +
            "°"
        ).c_str(),
        10,
        235,
        100,
        20,
        16,
        C_MUTED
    );


    label(
        (
            "MAX " +
            String(
                maxT,
                1
            ) +
            "°"
        ).c_str(),
        120,
        235,
        100,
        20,
        16,
        C_TEXT
    );


    label(
        (
            "PRESSURE " +
            pressureTrend()
        ).c_str(),
        10,
        270,
        220,
        20,
        16,
        C_CYAN
    );
}


// =================================================================
// FORECAST
// =================================================================

void drawForecast() {

    clearScreen();

    makeHeader(
        "7 DAY FORECAST"
    );


    for (
        int i = 0;
        i < weather.dailyCount;
        i++
    ) {

        int y =
            52 + i * 35;


        String date =
            weather.daily[i]
                .date;


        String row =
            date.substring(
                5
            ) +
            "  " +
            weatherDescription(
                weather.daily[i]
                    .weatherCode
            ).substring(
                0,
                min(
                    8,
                    (int)weatherDescription(
                        weather.daily[i]
                            .weatherCode
                    ).length()
                )
            );


        label(
            row.c_str(),
            8,
            y,
            120,
            20,
            16,
            i == 0
                ? C_YELLOW
                : C_TEXT
        );


        String temp =
            String(
                weather.daily[i]
                    .minimum,
                0
            ) +
            "/" +
            String(
                weather.daily[i]
                    .maximum,
                0
            ) +
            "°";


        label(
            temp.c_str(),
            135,
            y,
            55,
            20,
            16,
            C_TEXT
        );


        String rain =
            String(
                weather.daily[i]
                    .precipitationProbability
            ) +
            "%";


        label(
            rain.c_str(),
            194,
            y,
            40,
            20,
            16,
            C_BLUE
        );
    }
}


// =================================================================
// RAIN
// =================================================================

void drawRain() {

    clearScreen();

    makeHeader(
        "PRECIPITATION"
    );


    card(
        8,
        54,
        224,
        75
    );


    label(
        (
            "NOW  " +
            String(
                weather.rain,
                1
            ) +
            " mm"
        ).c_str(),
        17,
        65,
        205,
        25,
        16,
        C_CYAN
    );


    int p =
        weather.hourlyCount
            ? weather.hourly[0]
                .precipitationProbability
            : 0;


    label(
        (
            "CHANCE " +
            String(p) +
            "%"
        ).c_str(),
        17,
        96,
        205,
        22,
        16,
        C_BLUE
    );


    int firstRain = -1;


    for (
        int i = 0;
        i < weather.hourlyCount;
        i++
    ) {

        if (
            weather.hourly[i]
                .precipitationProbability >=
            50
        ) {

            firstRain = i;
            break;
        }
    }


    if (
        firstRain >= 0
    ) {

        label(
            (
                "RAIN SIGNAL IN ~" +
                String(firstRain) +
                " HOURS"
            ).c_str(),
            10,
            145,
            220,
            22,
            16,
            C_YELLOW
        );

    } else {

        label(
            "NO MAJOR RAIN SIGNAL",
            10,
            145,
            220,
            22,
            16,
            C_GREEN
        );
    }


    int y = 185;


    for (
        int i = 0;
        i < 8 &&
        i < weather.hourlyCount;
        i++
    ) {

        String time =
            weather.hourly[i]
                .time.substring(
                    11,
                    16
                );


        String row =
            time +
            "   " +
            String(
                weather.hourly[i]
                    .precipitationProbability
            ) +
            "%   " +
            String(
                weather.hourly[i]
                    .precipitation,
                1
            ) +
            "mm";


        label(
            row.c_str(),
            12,
            y,
            220,
            18,
            16,
            C_TEXT
        );


        y += 16;
    }
}


// =================================================================
// WIND
// =================================================================

void drawWind() {

    clearScreen();

    makeHeader(
        "WIND"
    );


    card(
        8,
        54,
        224,
        120
    );


    label(
        windString(
            weather.wind
        ).c_str(),
        20,
        66,
        150,
        35,
        32,
        C_GREEN
    );


    label(
        windDirection(
            weather.windDirection
        ).c_str(),
        180,
        68,
        40,
        30,
        32,
        C_CYAN
    );


    label(
        (
            String(
                weather.windDirection,
                0
            ) +
            "°"
        ).c_str(),
        180,
        108,
        40,
        20,
        16,
        C_MUTED
    );


    label(
        (
            "GUST " +
            windString(
                weather.gust
            )
        ).c_str(),
        20,
        125,
        180,
        20,
        16,
        C_ORANGE
    );


    label(
        (
            "BEAUFORT " +
            String(
                beaufort(
                    weather.wind
                )
            )
        ).c_str(),
        12,
        195,
        220,
        22,
        16,
        C_TEXT
    );


    label(
        (
            "DIRECTION " +
            windDirection(
                weather.windDirection
            )
        ).c_str(),
        12,
        230,
        220,
        22,
        16,
        C_CYAN
    );


    label(
        (
            "GUST " +
            windString(
                weather.gust
            )
        ).c_str(),
        12,
        265,
        220,
        22,
        16,
        C_ORANGE
    );
}


// =================================================================
// ATMOSPHERE
// =================================================================

void drawAtmosphere() {

    clearScreen();

    makeHeader(
        "ATMOSPHERE"
    );


    label(
        (
            "TEMP       " +
            tempString(
                weather.temperature
            )
        ).c_str(),
        10,
        55,
        220,
        20,
        16,
        C_YELLOW
    );


    label(
        (
            "FEELS      " +
            tempString(
                weather.apparent
            )
        ).c_str(),
        10,
        82,
        220,
        20,
        16,
        C_ORANGE
    );


    label(
        (
            "DEW POINT  " +
            tempString(
                weather.dewPoint
            )
        ).c_str(),
        10,
        109,
        220,
        20,
        16,
        C_CYAN
    );


    label(
        (
            "HUMIDITY   " +
            String(
                weather.humidity,
                0
            ) +
            "%"
        ).c_str(),
        10,
        136,
        220,
        20,
        16,
        C_BLUE
    );


    label(
        (
            "PRESSURE   " +
            pressureString(
                weather.pressure
            )
        ).c_str(),
        10,
        163,
        220,
        20,
        16,
        C_TEXT
    );


    label(
        (
            "TREND      " +
            pressureTrend()
        ).c_str(),
        10,
        190,
        220,
        20,
        16,
        C_CYAN
    );


    label(
        (
            "VISIBILITY " +
            String(
                weather.visibility /
                1000.0,
                1
            ) +
            " km"
        ).c_str(),
        10,
        217,
        220,
        20,
        16,
        C_GREEN
    );


    label(
        (
            "PRECIP     " +
            String(
                weather.precipitation,
                1
            ) +
            " mm"
        ).c_str(),
        10,
        244,
        220,
        20,
        16,
        C_BLUE
    );


    label(
        (
            "UV INDEX   " +
            String(
                weather.uv,
                1
            )
        ).c_str(),
        10,
        271,
        220,
        20,
        16,
        C_YELLOW
    );
}


// =================================================================
// AIR QUALITY
// =================================================================

void drawAir() {

    clearScreen();

    makeHeader(
        "AIR QUALITY"
    );


    String aqi =
        air.valid
            ? String(
                air.europeanAQI,
                0
            )
            : "--";


    label(
        aqi.c_str(),
        20,
        55,
        100,
        45,
        32,
        C_GREEN
    );


    label(
        aqiCategory(
            air.europeanAQI
        ).c_str(),
        20,
        100,
        190,
        20,
        16,
        C_CYAN
    );


    label(
        (
            "PM2.5   " +
            String(
                air.pm25,
                1
            )
        ).c_str(),
        12,
        145,
        105,
        20,
        16,
        C_TEXT
    );


    label(
        (
            "PM10    " +
            String(
                air.pm10,
                1
            )
        ).c_str(),
        122,
        145,
        105,
        20,
        16,
        C_TEXT
    );


    label(
        (
            "O3      " +
            String(
                air.ozone,
                1
            )
        ).c_str(),
        12,
        175,
        105,
        20,
        16,
        C_GREEN
    );


    label(
        (
            "NO2     " +
            String(
                air.no2,
                1
            )
        ).c_str(),
        122,
        175,
        105,
        20,
        16,
        C_ORANGE
    );


    label(
        (
            "SO2     " +
            String(
                air.so2,
                1
            )
        ).c_str(),
        12,
        205,
        105,
        20,
        16,
        C_YELLOW
    );


    label(
        (
            "CO      " +
            String(
                air.co,
                1
            )
        ).c_str(),
        122,
        205,
        105,
        20,
        16,
        C_MUTED
    );


    label(
        (
            "US AQI  " +
            String(
                air.usAQI,
                0
            )
        ).c_str(),
        12,
        245,
        220,
        20,
        16,
        C_TEXT
    );


    label(
        (
            "DUST    " +
            String(
                air.dust,
                1
            )
        ).c_str(),
        12,
        275,
        220,
        20,
        16,
        C_MUTED
    );
}


// =================================================================
// SUN
// =================================================================

String shortTime(
    const String &value
) {

    if (
        value.length() < 16
    )
        return "--:--";

    return value.substring(
        11,
        16
    );
}


void drawSun() {

    clearScreen();

    makeHeader(
        "SUN & MOON"
    );


    label(
        "SUNRISE",
        15,
        60,
        90,
        18,
        16,
        C_MUTED
    );


    label(
        shortTime(
            weather.sunrise
        ).c_str(),
        15,
        83,
        90,
        35,
        32,
        C_YELLOW
    );


    label(
        "SUNSET",
        130,
        60,
        90,
        18,
        16,
        C_MUTED
    );


    label(
        shortTime(
            weather.sunset
        ).c_str(),
        130,
        83,
        90,
        35,
        32,
        C_ORANGE
    );


    label(
        (
            "STATUS     " +
            String(
                weather.daytime
                    ? "DAY"
                    : "NIGHT"
            )
        ).c_str(),
        15,
        145,
        210,
        20,
        16,
        weather.daytime
            ? C_YELLOW
            : C_PURPLE
    );


    label(
        (
            "UV         " +
            String(
                weather.uv,
                1
            )
        ).c_str(),
        15,
        180,
        210,
        20,
        16,
        C_YELLOW
    );


    label(
        "MOON PHASE",
        15,
        220,
        210,
        20,
        16,
        C_MUTED
    );


    label(
        moonPhase().c_str(),
        15,
        247,
        210,
        35,
        16,
        C_CYAN
    );
}


// =================================================================
// ALERTS
// =================================================================

void drawAlerts() {

    clearScreen();

    makeHeader(
        "ALERT CENTER"
    );


    buildAlertsData();


    int y = 58;


    for (
        int i = 0;
        i < alertCount;
        i++
    ) {

        lv_obj_t *c =
            card(
                8,
                y,
                224,
                34
            );


        label(
            alerts[i].text.c_str(),
            18,
            y + 8,
            205,
            20,
            16,
            alerts[i].color
        );


        y += 42;


        if (
            y > 285
        )
            break;
    }
}


// =================================================================
// HISTORY
// =================================================================

void drawHistory() {

    clearScreen();

    makeHeader(
        "LOCAL HISTORY"
    );


    if (!sdAvailable) {

        label(
            "TF CARD NOT AVAILABLE",
            20,
            75,
            200,
            25,
            16,
            C_RED
        );


        label(
            "Insert a TF card",
            20,
            110,
            200,
            20,
            16,
            C_MUTED
        );


        return;
    }


    label(
        "LOGGING ENABLED",
        12,
        58,
        210,
        20,
        16,
        C_GREEN
    );


    label(
        "/weather.csv",
        12,
        90,
        210,
        20,
        16,
        C_CYAN
    );


    uint64_t used =
        SD.usedBytes();

    uint64_t total =
        SD.totalBytes();


    label(
        (
            "USED  " +
            String(
                (double)used /
                1048576.0,
                1
            ) +
            " MB"
        ).c_str(),
        12,
        130,
        210,
        20,
        16,
        C_TEXT
    );


    label(
        (
            "TOTAL " +
            String(
                (double)total /
                1048576.0,
                1
            ) +
            " MB"
        ).c_str(),
        12,
        160,
        210,
        20,
        16,
        C_MUTED
    );


    label(
        "Sampling every 15 minutes",
        12,
        210,
        220,
        20,
        16,
        C_CYAN
    );


    label(
        "Temperature / humidity / pressure",
        12,
        240,
        220,
        20,
        16,
        C_TEXT
    );


    label(
        "Wind / rain / UV / AQI",
        12,
        265,
        220,
        20,
        16,
        C_TEXT
    );
}


// =================================================================
// SYSTEM
// =================================================================

void drawSystem() {

    clearScreen();

    makeHeader(
        "SYSTEM"
    );


    label(
        (
            "WiFi       " +
            String(
                wifiConnected
                    ? "CONNECTED"
                    : "OFFLINE"
            )
        ).c_str(),
        10,
        55,
        220,
        20,
        16,
        wifiConnected
            ? C_GREEN
            : C_RED
    );


    if (wifiConnected) {

        label(
            (
                "RSSI       " +
                String(
                    WiFi.RSSI()
                ) +
                " dBm"
            ).c_str(),
            10,
            80,
            220,
            20,
            16,
            C_TEXT
        );


        label(
            WiFi.localIP()
                .toString()
                .c_str(),
            10,
            105,
            220,
            20,
            16,
            C_MUTED
        );
    }


    label(
        (
            "HEAP       " +
            String(
                ESP.getFreeHeap() /
                1024
            ) +
            " KB"
        ).c_str(),
        10,
        135,
        220,
        20,
        16,
        C_CYAN
    );


    label(
        (
            "PSRAM      " +
            String(
                ESP.getFreePsram() /
                1024
            ) +
            " KB"
        ).c_str(),
        10,
        160,
        220,
        20,
        16,
        C_GREEN
    );


    label(
        (
            "FLASH      " +
            String(
                ESP.getFlashChipSize() /
                1048576
            ) +
            " MB"
        ).c_str(),
        10,
        185,
        220,
        20,
        16,
        C_TEXT
    );


    label(
        (
            "BATTERY    " +
            String(
                batteryVoltage(),
                2
            ) +
            " V  " +
            String(
                batteryPercent()
            ) +
            "%"
        ).c_str(),
        10,
        210,
        220,
        20,
        16,
        C_YELLOW
    );


    label(
        (
            "TF CARD    " +
            String(
                sdAvailable
                    ? "READY"
                    : "OFFLINE"
            )
        ).c_str(),
        10,
        235,
        220,
        20,
        16,
        sdAvailable
            ? C_GREEN
            : C_RED
    );


    label(
        (
            "WEATHER    " +
            String(
                weather.valid
                    ? "VALID"
                    : "NONE"
            )
        ).c_str(),
        10,
        260,
        220,
        20,
        16,
        weather.valid
            ? C_GREEN
            : C_RED
    );


    label(
        "ESP32-S3 WEATHER STATION",
        10,
        295,
        220,
        18,
        16,
        C_PURPLE
    );
}


// =================================================================
// DRAW PAGE
// =================================================================

void drawPage() {

    switch (page) {

        case PAGE_HOME:
            drawHome();
            break;

        case PAGE_HOURLY:
            drawHourly();
            break;

        case PAGE_FORECAST:
            drawForecast();
            break;

        case PAGE_RAIN:
            drawRain();
            break;

        case PAGE_WIND:
            drawWind();
            break;

        case PAGE_ATMOSPHERE:
            drawAtmosphere();
            break;

        case PAGE_AIR:
            drawAir();
            break;

        case PAGE_SUN:
            drawSun();
            break;

        case PAGE_ALERTS:
            drawAlerts();
            break;

        case PAGE_HISTORY:
            drawHistory();
            break;

        case PAGE_SYSTEM:
            drawSystem();
            break;
    }


    lastPageChange =
        millis();
}


// =================================================================
// PAGE NAVIGATION
// =================================================================

void nextPage() {

    page++;

    if (
        page >= PAGE_COUNT
    )
        page = PAGE_HOME;

    drawPage();
}


void homePage() {

    page =
        PAGE_HOME;

    drawPage();
}


// =================================================================
// LVGL FLUSH
// =================================================================

void lvglFlush(
    lv_display_t *disp,
    const lv_area_t *area,
    uint8_t *px_map
) {

    uint32_t width =
        area->x2 -
        area->x1 +
        1;

    uint32_t height =
        area->y2 -
        area->y1 +
        1;


    gfx->draw16bitRGBBitmap(
        area->x1,
        area->y1,
        (uint16_t *)px_map,
        width,
        height
    );


    lv_display_flush_ready(
        disp
    );
}


// =================================================================
// LVGL TICK
// =================================================================

uint32_t lvglTick() {

    return millis();
}


// =================================================================
// DISPLAY INITIALIZATION
// =================================================================

void initializeDisplay() {

    Serial.println("Initializing LCD...");

    pinMode(
        LCD_BACKLIGHT,
        OUTPUT
    );

    digitalWrite(
        LCD_BACKLIGHT,
        HIGH
    );

    Serial.println("Backlight ON");

    if (!gfx->begin()) {
        Serial.println("ERROR: gfx->begin() failed");
    } else {
        Serial.println("gfx->begin() OK");
    }

    // Hardware test
    gfx->fillScreen(RGB565_RED);
    delay(1000);

    gfx->fillScreen(RGB565_GREEN);
    delay(1000);

    gfx->fillScreen(RGB565_BLUE);
    delay(1000);

    gfx->fillScreen(RGB565_BLACK);
    delay(500);

    // LVGL
    lv_init();

    lv_tick_set_cb(
        lvglTick
    );

    display =
        lv_display_create(
            LCD_WIDTH,
            LCD_HEIGHT
        );

    size_t bufferSize =
        LCD_WIDTH *
        LV_BUFFER_LINES *
        sizeof(lv_color_t);

    lvBuffer1 =
        (uint8_t *)
        heap_caps_malloc(
            bufferSize,
            MALLOC_CAP_DMA |
            MALLOC_CAP_INTERNAL
        );

    lvBuffer2 =
        (uint8_t *)
        heap_caps_malloc(
            bufferSize,
            MALLOC_CAP_DMA |
            MALLOC_CAP_INTERNAL
        );

    if (
        !lvBuffer1 ||
        !lvBuffer2
    ) {

        Serial.println(
            "LVGL buffer allocation failed"
        );

        while (true)
            delay(1000);
    }

    lv_display_set_buffers(
        display,
        lvBuffer1,
        lvBuffer2,
        bufferSize,
        LV_DISPLAY_RENDER_MODE_PARTIAL
    );

    lv_display_set_flush_cb(
        display,
        lvglFlush
    );

    root =
        lv_screen_active();

    lv_obj_set_style_bg_color(
        root,
        C_BG,
        0
    );

    lv_obj_set_style_bg_opa(
        root,
        LV_OPA_COVER,
        0
    );

    Serial.println("LVGL initialized");
}


// =================================================================
// BUTTON HANDLING
// =================================================================

bool lastButtonState =
    HIGH;

unsigned long buttonDown =
    0;

bool longPressHandled =
    false;


void handleButton() {

    bool state =
        digitalRead(
            USER_BUTTON
        );


    if (
        lastButtonState == HIGH &&
        state == LOW
    ) {

        buttonDown =
            millis();

        longPressHandled =
            false;
    }


    if (
        state == LOW &&
        !longPressHandled &&
        millis() -
        buttonDown >
        1200
    ) {

        longPressHandled =
            true;

        homePage();
    }


    if (
        lastButtonState == LOW &&
        state == HIGH
    ) {

        if (
            !longPressHandled &&
            millis() -
            buttonDown >
            30
        ) {

            nextPage();
        }
    }


    lastButtonState =
        state;
}


// =================================================================
// WEATHER UPDATE
// =================================================================

void updateWeatherIfNeeded() {

    if (!wifiConnected)
        return;


    if (
        !weather.valid ||
        millis() -
        lastWeatherUpdate >
        WEATHER_UPDATE_MS
    ) {

        if (
            updateWeather()
        ) {

            lastWeatherUpdate =
                millis();

            saveCache();

            drawPage();
        }
    }
}


// =================================================================
// AQI UPDATE
// =================================================================

void updateAirIfNeeded() {

    if (!wifiConnected)
        return;


    if (
        !air.valid ||
        millis() -
        lastAQIUpdate >
        AQI_UPDATE_MS
    ) {

        if (
            updateAir()
        ) {

            lastAQIUpdate =
                millis();

            drawPage();
        }
    }
}


// =================================================================
// HISTORY
// =================================================================

void historyIfNeeded() {

    if (
        millis() -
        lastHistoryWrite >
        HISTORY_UPDATE_MS
    ) {

        saveHistory();

        lastHistoryWrite =
            millis();
    }
}


// =================================================================
// LIVE CLOCK
// =================================================================

void clockIfNeeded() {

    if (
        millis() -
        lastClockUpdate <
        UI_CLOCK_UPDATE_MS
    )
        return;


    lastClockUpdate =
        millis();


    /*
       Rebuild only when the minute changes.

       Avoiding a full LVGL rebuild every second is important.
    */

    static String lastTime;


    String now =
        localTime();


    if (
        now != lastTime
    ) {

        lastTime =
            now;

        drawPage();
    }
}


// =================================================================
// AUTO PAGE
// =================================================================

void autoRotateIfNeeded() {

    if (!autoRotate)
        return;


    if (
        millis() -
        lastPageChange >
        AUTO_PAGE_MS
    ) {

        nextPage();
    }
}


// =================================================================
// SETUP
// =================================================================

void setup() {

    Serial.begin(
        115200
    );


    delay(500);


    Serial.println();
    Serial.println(
        "================================"
    );
    Serial.println(
        " ULTIMATE WEATHER STATION"
    );
    Serial.println(
        " ESP32-S3-LCD-2"
    );
    Serial.println(
        "================================"
    );


    // -------------------------------------------------------------
    // GPIO
    // -------------------------------------------------------------

    pinMode(
        USER_BUTTON,
        INPUT_PULLUP
    );


    pinMode(
        LCD_BACKLIGHT,
        OUTPUT
    );

    digitalWrite(
        LCD_BACKLIGHT,
        HIGH
    );


    analogReadResolution(
        12
    );


    // -------------------------------------------------------------
    // Preferences
    // -------------------------------------------------------------

    loadCache();


    // -------------------------------------------------------------
    // Display
    // -------------------------------------------------------------

    initializeDisplay();


    // -------------------------------------------------------------
    // SD
    // -------------------------------------------------------------

    initializeSD();


    // -------------------------------------------------------------
    // WiFi
    // -------------------------------------------------------------

    startWiFi();


    /*
       Give WiFi a short initial opportunity.

       The station still boots even without Internet.
    */

    unsigned long start =
        millis();


    while (
        WiFi.status() !=
        WL_CONNECTED &&
        millis() -
        start <
        12000
    ) {

        delay(100);
    }


    if (
        WiFi.status() ==
        WL_CONNECTED
    ) {

        wifiConnected =
            true;

        Serial.print(
            "IP: "
        );

        Serial.println(
            WiFi.localIP()
        );


        setupTime();

    } else {

        wifiConnected =
            false;

        Serial.println(
            "Starting offline"
        );
    }


    // -------------------------------------------------------------
    // Initial weather
    // -------------------------------------------------------------

    if (wifiConnected) {

        updateWeather();

        updateAir();

        lastWeatherUpdate =
            millis();

        lastAQIUpdate =
            millis();
    }


    // -------------------------------------------------------------
    // Initial UI
    // -------------------------------------------------------------

    drawPage();


    lastPageChange =
        millis();
}


// =================================================================
// MAIN LOOP
// =================================================================

void loop() {

    maintainWiFi();

    handleButton();

    updateWeatherIfNeeded();

    updateAirIfNeeded();

    historyIfNeeded();

    clockIfNeeded();

    autoRotateIfNeeded();


    /*
       LVGL must be serviced frequently.
    */

    lv_timer_handler();


    delay(5);
}