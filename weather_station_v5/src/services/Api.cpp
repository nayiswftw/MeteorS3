#include "src/services/Api.h"
#include "src/services/Network.h"
#include "src/services/Storage.h"
#include "src/core/State.h"
#include "src/Config.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

namespace svc {

static bool getJson(const String& url, JsonDocument& doc) {
    if (!isWifiConnected()) return false;

    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(20000);
    http.useHTTP10(true);

    if (!http.begin(url)) {
        Serial.println("[http] begin failed");
        return false;
    }

    http.addHeader("User-Agent", "ESP32-WeatherStation-V4");

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[http] error %d\n", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    if (payload.length() == 0) return false;

    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[json] deserialize failed: %s\n", err.c_str());
        return false;
    }

    return true;
}

static String buildWeatherUrl() {
    state::lock();
    double lat = state::config().latitude;
    double lon = state::config().longitude;
    state::unlock();

    String url = "https://api.open-meteo.com/v1/forecast";
    url += "?latitude=" + String(lat, 6);
    url += "&longitude=" + String(lon, 6);


    url += "&current="
           "temperature_2m,"
           "relative_humidity_2m,"
           "apparent_temperature,"
           "dew_point_2m,"
           "pressure_msl,"
           "cloud_cover,"
           "wind_speed_10m,"
           "wind_direction_10m,"
           "wind_gusts_10m,"
           "precipitation,"
           "rain,"
           "weather_code,"
           "visibility,"
           "uv_index,"
           "is_day";

    url += "&hourly="
           "temperature_2m,"
           "apparent_temperature,"
           "relative_humidity_2m,"
           "dew_point_2m,"
           "pressure_msl,"
           "cloud_cover,"
           "visibility,"
           "uv_index,"
           "wind_speed_10m,"
           "wind_direction_10m,"
           "wind_gusts_10m,"
           "precipitation,"
           "rain,"
           "precipitation_probability,"
           "weather_code";

    url += "&daily="
           "weather_code,"
           "temperature_2m_max,"
           "temperature_2m_min,"
           "precipitation_sum,"
           "precipitation_probability_max,"
           "wind_speed_10m_max,"
           "wind_gusts_10m_max,"
           "uv_index_max,"
           "sunrise,"
           "sunset";

    url += "&past_hours=12";
    url += "&forecast_days=10";
    url += "&timezone=auto";

    return url;
}

static int findCurrentHourIndex(JsonArray times, const String& currentTime) {
    if (times.size() == 0 || currentTime.length() < 13) return 0;
    String key = currentTime.substring(0, 13);

    for (int i = 0; i < (int)times.size(); i++) {
        String item = times[i].as<String>();
        if (item.startsWith(key)) return i;
    }
    return 0;
}

bool refreshWeather() {
    Serial.println("[weather] fetching data...");
    JsonDocument doc;

    if (!getJson(buildWeatherUrl(), doc)) {
        Serial.println("[weather] fetch failed");
        return false;
    }

    JsonObject current = doc["current"];
    String currentTime = current["time"] | "";

    WeatherData w;
    w.temperature   = current["temperature_2m"]       | NAN;
    w.apparent      = current["apparent_temperature"]  | NAN;
    w.humidity      = current["relative_humidity_2m"]  | NAN;
    w.dewPoint      = current["dew_point_2m"]          | NAN;
    w.pressure      = current["pressure_msl"]          | NAN;
    w.cloudCover    = current["cloud_cover"]           | NAN;
    w.wind          = current["wind_speed_10m"]        | NAN;
    w.direction     = current["wind_direction_10m"]    | NAN;
    w.gust          = current["wind_gusts_10m"]        | NAN;
    w.precipitation = current["precipitation"]         | NAN;
    w.rain          = current["rain"]                  | NAN;
    w.weatherCode   = current["weather_code"]          | -1;
    w.visibility    = current["visibility"]            | NAN;
    w.uv            = current["uv_index"]              | NAN;
    w.isDay         = (current["is_day"] | 1) == 1;

    JsonObject hourly = doc["hourly"];
    JsonArray times = hourly["time"];
    int nowIndex = findCurrentHourIndex(times, currentTime);

    auto pressureAt = [&](int offset) -> float {
        int idx = nowIndex - offset;
        if (idx < 0) return NAN;
        return hourly["pressure_msl"][idx] | NAN;
    };

    float pNow = hourly["pressure_msl"][nowIndex] | NAN;
    float p3   = pressureAt(3);
    float p6   = pressureAt(6);
    float p12  = pressureAt(12);

    w.pressureDelta3h  = (!isnan(pNow) && !isnan(p3))  ? (pNow - p3)  : NAN;
    w.pressureDelta6h  = (!isnan(pNow) && !isnan(p6))  ? (pNow - p6)  : NAN;
    w.pressureDelta12h = (!isnan(pNow) && !isnan(p12)) ? (pNow - p12) : NAN;

    // Parse Hourly (up to 48 hours forward)
    w.hourlyCount = 0;
    for (int src = nowIndex; src < (int)times.size() && w.hourlyCount < MAX_HOURLY; src++) {
        HourData& h = w.hourly[w.hourlyCount++];
        String ts = times[src].as<String>();
        if (ts.length() >= 16) {
            snprintf(h.time, sizeof(h.time), "%s", ts.substring(11, 16).c_str());
        }

        h.temperature   = hourly["temperature_2m"][src]           | NAN;
        h.apparent      = hourly["apparent_temperature"][src]      | NAN;
        h.humidity      = hourly["relative_humidity_2m"][src]      | NAN;
        h.dewPoint      = hourly["dew_point_2m"][src]              | NAN;
        h.pressure      = hourly["pressure_msl"][src]              | NAN;
        h.cloudCover    = hourly["cloud_cover"][src]               | NAN;
        h.visibility    = hourly["visibility"][src]                | NAN;
        h.uv            = hourly["uv_index"][src]                  | NAN;
        h.wind          = hourly["wind_speed_10m"][src]            | NAN;
        h.direction     = hourly["wind_direction_10m"][src]        | NAN;
        h.gust          = hourly["wind_gusts_10m"][src]            | NAN;
        h.precipitation = hourly["precipitation"][src]             | NAN;
        h.rain          = hourly["rain"][src]                      | NAN;
        h.rainChance    = hourly["precipitation_probability"][src] | 0;
        h.weatherCode   = hourly["weather_code"][src]              | -1;
    }

    // Parse Daily (up to 10 days)
    JsonObject daily = doc["daily"];
    JsonArray dates = daily["time"];
    w.dailyCount = min(MAX_DAILY, (int)dates.size());

    for (int i = 0; i < w.dailyCount; i++) {
        DayData& d = w.daily[i];
        String dt = dates[i].as<String>();
        snprintf(d.date, sizeof(d.date), "%s", dt.c_str());

        d.low           = daily["temperature_2m_min"][i]           | NAN;
        d.high          = daily["temperature_2m_max"][i]           | NAN;
        d.precipitation = daily["precipitation_sum"][i]            | NAN;
        d.rainChance    = daily["precipitation_probability_max"][i]| 0;
        d.windMax       = daily["wind_speed_10m_max"][i]           | NAN;
        d.gustMax       = daily["wind_gusts_10m_max"][i]           | NAN;
        d.uvMax         = daily["uv_index_max"][i]                 | NAN;
        d.weatherCode   = daily["weather_code"][i]                 | -1;

        String sr = daily["sunrise"][i] | "";
        String ss = daily["sunset"][i]  | "";
        if (sr.length() >= 16) {
            snprintf(d.sunrise, sizeof(d.sunrise), "%s", sr.substring(11, 16).c_str());
        }
        if (ss.length() >= 16) {
            snprintf(d.sunset, sizeof(d.sunset), "%s", ss.substring(11, 16).c_str());
        }
    }

    w.valid = true;
    w.fetchedAt = time(nullptr);

    state::setWeather(w);
    state::setLastWeatherUpdate(millis());
    storageSaveCache();

    Serial.printf("[weather] updated: %.1f C | %d hourly | %d daily\n",
                  w.temperature, w.hourlyCount, w.dailyCount);
    return true;
}

static String buildAirUrl() {
    state::lock();
    double lat = state::config().latitude;
    double lon = state::config().longitude;
    state::unlock();

    String url = "https://air-quality-api.open-meteo.com/v1/air-quality";
    url += "?latitude=" + String(lat, 6);
    url += "&longitude=" + String(lon, 6);
    url += "&current=pm2_5,pm10,carbon_monoxide,nitrogen_dioxide,sulphur_dioxide,ozone,european_aqi,us_aqi";
    url += "&timezone=auto";
    return url;
}

bool refreshAir() {
    Serial.println("[air] fetching data...");
    JsonDocument doc;

    if (!getJson(buildAirUrl(), doc)) {
        Serial.println("[air] fetch failed");
        return false;
    }

    JsonObject current = doc["current"];
    AirData a;
    a.pm25        = current["pm2_5"]           | NAN;
    a.pm10        = current["pm10"]            | NAN;
    a.co          = current["carbon_monoxide"]  | NAN;
    a.no2         = current["nitrogen_dioxide"] | NAN;
    a.so2         = current["sulphur_dioxide"]  | NAN;
    a.ozone       = current["ozone"]            | NAN;
    a.europeanAqi = current["european_aqi"]     | -1;
    a.usAqi       = current["us_aqi"]           | -1;

    a.valid     = (a.usAqi >= 0 || !isnan(a.pm25));
    a.fetchedAt = time(nullptr);

    state::setAir(a);
    state::setLastAirUpdate(millis());

    Serial.printf("[air] updated: AQI %d | PM2.5 %.1f\n", a.usAqi, a.pm25);
    return a.valid;
}

static volatile bool s_forceRefresh = false;
static TaskHandle_t  s_apiTaskHandle = nullptr;

static void apiWorkerTask(void* parameter) {
    Serial.println("[api-task] background worker running on Core 0");

    while (true) {
        if (isWifiConnected()) {
            bool needWeather = false;
            bool needAir     = false;

            if (s_forceRefresh) {
                needWeather = true;
                needAir     = true;
                s_forceRefresh = false;
            } else {
                state::lock();
                bool weatherValid = state::weather().valid;
                uint32_t lastWeather = state::lastWeatherUpdate();
                bool airValid = state::air().valid;
                uint32_t lastAir = state::lastAirUpdate();
                state::unlock();

                uint32_t now = millis();
                if (!weatherValid || (now - lastWeather >= config::WEATHER_INTERVAL_MS)) {
                    needWeather = true;
                }
                if (!airValid || (now - lastAir >= config::AIR_INTERVAL_MS)) {
                    needAir = true;
                }
            }

            if (needWeather) {
                refreshWeather();
                vTaskDelay(pdMS_TO_TICKS(500));
            }

            if (needAir) {
                refreshAir();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void apiInit() {
    if (!s_apiTaskHandle) {
        xTaskCreatePinnedToCore(
            apiWorkerTask,
            "ApiWorker",
            8192,
            nullptr,
            1,
            &s_apiTaskHandle,
            0 // Core 0 (background core, core 1 runs UI loop)
        );
    }
}

void apiService() {
    // API work is managed in the background FreeRTOS task.
}

void apiForceRefresh() {
    s_forceRefresh = true;
}

}  // namespace svc

