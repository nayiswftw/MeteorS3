#include "services/Storage.h"
#include "core/State.h"
#include "Config.h"

#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>
#include <time.h>

namespace svc {

static Preferences s_prefs;

void storageLoadCache() {
    s_prefs.begin("weather-v4", true);

    if (!s_prefs.getBool("valid", false)) {
        s_prefs.end();
        return;
    }

    WeatherData w;
    w.temperature = s_prefs.getFloat("temp", NAN);
    w.apparent    = s_prefs.getFloat("apparent", NAN);
    w.humidity    = s_prefs.getFloat("humidity", NAN);
    w.dewPoint    = s_prefs.getFloat("dew", NAN);
    w.pressure    = s_prefs.getFloat("pressure", NAN);
    w.wind        = s_prefs.getFloat("wind", NAN);
    w.gust        = s_prefs.getFloat("gust", NAN);
    w.direction   = s_prefs.getFloat("direction", NAN);
    w.uv          = s_prefs.getFloat("uv", NAN);
    w.visibility  = s_prefs.getFloat("visibility", NAN);
    w.weatherCode = s_prefs.getInt("code", -1);
    w.isDay       = s_prefs.getBool("day", true);
    w.fetchedAt   = (time_t)s_prefs.getLong64("time", 0);
    w.valid       = true;

    s_prefs.end();

    state::setWeather(w);
    Serial.println("[cache] restored from NVS");
}

void storageSaveCache() {
    const WeatherData& w = state::weather();
    if (!w.valid) return;

    s_prefs.begin("weather-v4", false);

    s_prefs.putBool("valid", true);
    s_prefs.putFloat("temp", w.temperature);
    s_prefs.putFloat("apparent", w.apparent);
    s_prefs.putFloat("humidity", w.humidity);
    s_prefs.putFloat("dew", w.dewPoint);
    s_prefs.putFloat("pressure", w.pressure);
    s_prefs.putFloat("wind", w.wind);
    s_prefs.putFloat("gust", w.gust);
    s_prefs.putFloat("direction", w.direction);
    s_prefs.putFloat("uv", w.uv);
    s_prefs.putFloat("visibility", w.visibility);
    s_prefs.putInt("code", w.weatherCode);
    s_prefs.putBool("day", w.isDay);
    s_prefs.putLong64("time", (int64_t)w.fetchedAt);

    s_prefs.end();
}

void storageLoadHistory() {
    if (!state::isSdReady()) return;

    File file = SD.open("/weather.csv", FILE_READ);
    if (!file) return;

    // Skip header line
    file.readStringUntil('\n');

    HistoryPoint points[MAX_HISTORY];
    int count = 0;

    char lineBuf[128];
    while (file.available()) {
        size_t len = file.readBytesUntil('\n', lineBuf, sizeof(lineBuf) - 1);
        lineBuf[len] = '\0';

        if (len == 0) continue;

        // Parse: timestamp,temp,humidity,pressure,aqi
        long long ts = 0;
        float temp = NAN, hum = NAN, press = NAN, aqi = NAN;
        int parsed = sscanf(lineBuf, "%lld,%f,%f,%f,%f", &ts, &temp, &hum, &press, &aqi);

        if (parsed >= 4) {
            HistoryPoint pt;
            pt.timestamp   = (time_t)ts;
            pt.temperature = temp;
            pt.humidity    = hum;
            pt.pressure    = press;
            pt.aqi         = (parsed >= 5) ? aqi : NAN;

            if (count < MAX_HISTORY) {
                points[count++] = pt;
            } else {
                // Shift array left to keep the latest MAX_HISTORY
                for (int i = 1; i < MAX_HISTORY; i++) {
                    points[i - 1] = points[i];
                }
                points[MAX_HISTORY - 1] = pt;
            }
        }
    }

    file.close();

    state::loadHistory(points, count);
    Serial.printf("[history] loaded %d points from SD\n", count);
}

void storageLogHistory() {
    if (!state::isSdReady()) return;
    const WeatherData& w = state::weather();
    if (!w.valid) return;

    File file = SD.open("/weather.csv", FILE_APPEND);
    if (!file) return;

    time_t now = time(nullptr);
    const AirData& a = state::air();

    file.printf("%lld,%.2f,%.2f,%.2f,%.2f\n",
                (long long)now,
                w.temperature,
                w.humidity,
                w.pressure,
                a.valid ? (float)a.usAqi : NAN);
    file.close();

    HistoryPoint pt;
    pt.timestamp   = now;
    pt.temperature = w.temperature;
    pt.humidity    = w.humidity;
    pt.pressure    = w.pressure;
    pt.aqi         = a.valid ? (float)a.usAqi : NAN;

    state::pushHistory(pt);
    state::setLastHistoryWrite(millis());
    Serial.println("[history] logged point to SD");
}

void storageInit() {
    storageLoadCache();

    if (!config::ENABLE_SD) {
        Serial.println("[sd] disabled in config");
        state::setSdReady(false);
        return;
    }

    bool sdOk = SD.begin(config::SD_CS);
    state::setSdReady(sdOk);

    if (!sdOk) {
        Serial.println("[sd] mount failed");
        return;
    }

    Serial.println("[sd] mounted successfully");

    if (!SD.exists("/weather.csv")) {
        File file = SD.open("/weather.csv", FILE_WRITE);
        if (file) {
            file.println("time,temp,humidity,pressure,aqi");
            file.close();
        }
    }

    storageLoadHistory();
}

void storageService() {
    if (!state::isSdReady()) return;
    const WeatherData& w = state::weather();
    if (!w.valid) return;

    if (millis() - state::lastHistoryWrite() >= config::HISTORY_INTERVAL_MS) {
        storageLogHistory();
    }
}

}  // namespace svc
