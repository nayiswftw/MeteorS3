#include "src/services/Storage.h"
#include "src/core/State.h"
#include "src/Config.h"

#include <Arduino.h>
#include <Preferences.h>
#include <SD_MMC.h>
#include <time.h>

namespace svc {

static Preferences s_prefs;

void storageLoadConfig() {
    s_prefs.begin("ws-cfg", true);

    RuntimeConfig cfg;
    String ssid = s_prefs.getString("ssid", config::WIFI_SSID);
    String pass = s_prefs.getString("pass", config::WIFI_PASSWORD);
    snprintf(cfg.wifiSsid, sizeof(cfg.wifiSsid), "%s", ssid.c_str());
    snprintf(cfg.wifiPassword, sizeof(cfg.wifiPassword), "%s", pass.c_str());

    cfg.latitude  = s_prefs.getDouble("lat", config::LATITUDE);
    cfg.longitude = s_prefs.getDouble("lon", config::LONGITUDE);

    String loc = s_prefs.getString("loc", config::LOCATION_NAME);
    String tz  = s_prefs.getString("tz", config::TIMEZONE);
    snprintf(cfg.locationName, sizeof(cfg.locationName), "%s", loc.c_str());
    snprintf(cfg.timezone, sizeof(cfg.timezone), "%s", tz.c_str());

    cfg.tempUnit  = (TempUnit)s_prefs.getUChar("tu", (uint8_t)config::tempUnit);
    cfg.windUnit  = (WindUnit)s_prefs.getUChar("wu", (uint8_t)config::windUnit);
    cfg.pressUnit = (PressUnit)s_prefs.getUChar("pu", (uint8_t)config::pressUnit);

    cfg.backlightBrightness = s_prefs.getUChar("bl", 220);
    cfg.screenTimeoutSec    = s_prefs.getUInt("tout", 60);
    cfg.enableGestures      = s_prefs.getBool("gest", true);
    cfg.gestureSensitivity  = s_prefs.getUChar("gsens", 5);
    cfg.enableAutoPage      = s_prefs.getBool("apg", config::ENABLE_AUTO_PAGE);
    cfg.autoPageSec         = s_prefs.getUInt("apgs", config::AUTO_PAGE_MS / 1000UL);

    // MQTT
    cfg.mqttEnabled         = s_prefs.getBool("mq_en", false);
    String mqSrv = s_prefs.getString("mq_srv", "");
    String mqUser = s_prefs.getString("mq_usr", "");
    String mqPass = s_prefs.getString("mq_pwd", "");
    String mqPfx  = s_prefs.getString("mq_pfx", "weatherstation");
    snprintf(cfg.mqttServer, sizeof(cfg.mqttServer), "%s", mqSrv.c_str());
    snprintf(cfg.mqttUser, sizeof(cfg.mqttUser), "%s", mqUser.c_str());
    snprintf(cfg.mqttPassword, sizeof(cfg.mqttPassword), "%s", mqPass.c_str());
    snprintf(cfg.mqttTopicPrefix, sizeof(cfg.mqttTopicPrefix), "%s", mqPfx.c_str());
    cfg.mqttPort            = s_prefs.getUShort("mq_port", 1883);

    s_prefs.end();
    state::setConfig(cfg);
    Serial.printf("[config] loaded settings (Loc: %s, Lat: %.4f, Lon: %.4f)\n", cfg.locationName, cfg.latitude, cfg.longitude);
}

void storageSaveConfig(const RuntimeConfig& cfg) {
    s_prefs.begin("ws-cfg", false);

    s_prefs.putString("ssid", cfg.wifiSsid);
    s_prefs.putString("pass", cfg.wifiPassword);
    s_prefs.putDouble("lat", cfg.latitude);
    s_prefs.putDouble("lon", cfg.longitude);
    s_prefs.putString("loc", cfg.locationName);
    s_prefs.putString("tz", cfg.timezone);

    s_prefs.putUChar("tu", (uint8_t)cfg.tempUnit);
    s_prefs.putUChar("wu", (uint8_t)cfg.windUnit);
    s_prefs.putUChar("pu", (uint8_t)cfg.pressUnit);

    s_prefs.putUChar("bl", cfg.backlightBrightness);
    s_prefs.putUInt("tout", cfg.screenTimeoutSec);
    s_prefs.putBool("gest", cfg.enableGestures);
    s_prefs.putUChar("gsens", cfg.gestureSensitivity);
    s_prefs.putBool("apg", cfg.enableAutoPage);
    s_prefs.putUInt("apgs", cfg.autoPageSec);

    s_prefs.putBool("mq_en", cfg.mqttEnabled);
    s_prefs.putString("mq_srv", cfg.mqttServer);
    s_prefs.putUShort("mq_port", cfg.mqttPort);
    s_prefs.putString("mq_usr", cfg.mqttUser);
    s_prefs.putString("mq_pwd", cfg.mqttPassword);
    s_prefs.putString("mq_pfx", cfg.mqttTopicPrefix);

    s_prefs.end();
    state::setConfig(cfg);
    Serial.println("[config] runtime settings saved to NVS");
}

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

#include <SD_MMC.h>

void storageLoadHistory() {
    if (!state::isSdReady()) return;

    File file = SD_MMC.open("/weather.csv", FILE_READ);
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

    File file = SD_MMC.open("/weather.csv", FILE_APPEND);
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
    storageLoadConfig();
    storageLoadCache();

    if (!config::ENABLE_SD) {
        state::setSdReady(false);
        return;
    }

    struct SdMmcPinPair { int clk; int cmd; int d0; const char* desc; };
    const SdMmcPinPair pinPairs[] = {
        { 14, 15, 2,  "Waveshare Official LCD-2 (CLK=14, CMD=15, D0=2)" },
        { 14, 15, 4,  "Alternate Pinout 1 (CLK=14, CMD=15, D0=4)" },
        { 14, 15, 21, "Alternate Pinout 2 (CLK=14, CMD=15, D0=21)" },
        { 14, 15, 16, "Alternate Pinout 3 (CLK=14, CMD=15, D0=16)" },
        { 12, 11, 13, "Alternate Pinout 4 (CLK=12, CMD=11, D0=13)" }
    };

    bool sdOk = false;
    for (const auto& p : pinPairs) {
        SD_MMC.end();
        delay(10);

        // Enable internal pull-ups on CMD and D0 lines for reliable high-speed SPI/MMC handshaking
        pinMode(p.cmd, INPUT_PULLUP);
        pinMode(p.d0,  INPUT_PULLUP);

        if (SD_MMC.setPins(p.clk, p.cmd, p.d0)) {
            // Try standard frequency (20MHz) first, then fallback to probing speed (400kHz)
            if (SD_MMC.begin("/sdcard", true, false, SDMMC_FREQ_DEFAULT, 5)) {
                sdOk = true;
            } else if (SD_MMC.begin("/sdcard", true, false, SDMMC_FREQ_PROBING, 5)) {
                sdOk = true;
            }

            if (sdOk) {
                uint8_t cardType = SD_MMC.cardType();
                const char* typeStr = "Unknown";
                if (cardType == CARD_MMC)  typeStr = "MMC";
                else if (cardType == CARD_SD)   typeStr = "SDSC";
                else if (cardType == CARD_SDHC) typeStr = "SDHC/SDXC";

                uint64_t totalBytes = SD_MMC.totalBytes();
                uint64_t cardSize   = SD_MMC.cardSize();

                Serial.printf("[sd] SD_MMC mounted successfully via %s!\n", p.desc);
                Serial.printf("[sd] Card Type: %s | Size: %llu MB | Formatted: %llu MB\n",
                              typeStr,
                              cardSize / (1024ULL * 1024ULL),
                              totalBytes / (1024ULL * 1024ULL));
                break;
            }
        }
    }

    state::setSdReady(sdOk);

    if (!sdOk) {
        Serial.println("[sd] MicroSD card not mounted. Please ensure:");
        Serial.println("     1. The card is formatted as FAT32 (exFAT/NTFS will not work).");
        Serial.println("     2. The card is 32GB or smaller for maximum compatibility.");
        Serial.println("     3. The card is firmly pushed into the slot until it clicks.");
        return;
    }

    if (!SD_MMC.exists("/weather.csv")) {
        File file = SD_MMC.open("/weather.csv", FILE_WRITE);
        if (file) {
            file.println("time,temp,humidity,pressure,aqi");
            file.close();
            Serial.println("[sd] created fresh /weather.csv logging database");
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
