#include "DataService.h"

#include "Config.h"
#include "Models.h"
#include "AppState.h"
#include "StorageService.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

static uint32_t lastWifiAttempt = 0;

String localClockText() {
  struct tm now;

  if (!getLocalTime(&now, 20)) return "--:--";

  char value[6];
  strftime(value, sizeof(value), "%H:%M", &now);

  return String(value);
}

String localDateText() {
  struct tm now;

  if (!getLocalTime(&now, 20)) return "--- -- ---";

  char value[20];
  strftime(value, sizeof(value), "%a, %d %b", &now);

  return String(value);
}

static bool getJson(const String &url,
                    JsonDocument &document) {
  if (!wifiConnected) return false;

  HTTPClient http;

  http.setConnectTimeout(10000);
  http.setTimeout(20000);
  http.useHTTP10(true);

  if (!http.begin(url)) {
    Serial.println("[http] begin failed");
    return false;
  }

  http.addHeader(
    "User-Agent",
    "ESP32-WeatherStation-V4"
  );

  int responseCode = http.GET();

  if (responseCode != HTTP_CODE_OK) {
    Serial.printf("[http] error %d\n", responseCode);
    http.end();
    return false;
  }

  // Full-payload read is intentional. It fixed the weather
  // IncompleteInput issue seen on this board/core combination.
  String payload = http.getString();

  http.end();

  Serial.printf(
    "[http] received %u bytes\n",
    payload.length()
  );

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

static String weatherURL() {
  String url =
    "https://api.open-meteo.com/v1/forecast";

  url += "?latitude=" + String(LATITUDE, 6);
  url += "&longitude=" + String(LONGITUDE, 6);

  url +=
    "&current="
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

  url +=
    "&hourly="
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

  url +=
    "&daily="
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

  // Enough history for 3/6/12-hour pressure tendencies.
  url += "&past_hours=12";

  // We display up to 10 days when available.
  url += "&forecast_days=10";
  url += "&timezone=auto";

  return url;
}

static int findCurrentHour(JsonArray times,
                           const String &currentTime) {
  if (times.size() == 0 ||
      currentTime.length() < 13) {
    return 0;
  }

  String key =
    currentTime.substring(0, 13);

  for (int i = 0; i < (int)times.size(); i++) {
    String item =
      times[i].as<String>();

    if (item.startsWith(key)) return i;
  }

  return 0;
}

bool refreshWeather() {
  Serial.println("[weather] updating");

  JsonDocument document;

  if (!getJson(weatherURL(), document)) {
    Serial.println("[weather] failed");
    return false;
  }

  JsonObject current =
    document["current"];

  String currentTime =
    current["time"] | "";

  weather.temperature =
    current["temperature_2m"] | NAN;

  weather.apparent =
    current["apparent_temperature"] | NAN;

  weather.humidity =
    current["relative_humidity_2m"] | NAN;

  weather.dewPoint =
    current["dew_point_2m"] | NAN;

  weather.pressure =
    current["pressure_msl"] | NAN;

  weather.cloudCover =
    current["cloud_cover"] | NAN;

  weather.wind =
    current["wind_speed_10m"] | NAN;

  weather.direction =
    current["wind_direction_10m"] | NAN;

  weather.gust =
    current["wind_gusts_10m"] | NAN;

  weather.precipitation =
    current["precipitation"] | NAN;

  weather.rain =
    current["rain"] | NAN;

  weather.weatherCode =
    current["weather_code"] | -1;

  weather.visibility =
    current["visibility"] | NAN;

  weather.uv =
    current["uv_index"] | NAN;

  weather.isDay =
    (current["is_day"] | 1) == 1;

  JsonObject hourly =
    document["hourly"];

  JsonArray times =
    hourly["time"];

  int nowIndex =
    findCurrentHour(
      times,
      currentTime
    );

  auto pressureAt =
    [&](int offset) -> float {
      int index = nowIndex - offset;

      if (index < 0) return NAN;

      return
        hourly["pressure_msl"][index] | NAN;
    };

  float pressureNow =
    hourly["pressure_msl"][nowIndex] | NAN;

  float p3 = pressureAt(3);
  float p6 = pressureAt(6);
  float p12 = pressureAt(12);

  weather.pressureDelta3h =
    (!isnan(pressureNow) && !isnan(p3))
      ? pressureNow - p3
      : NAN;

  weather.pressureDelta6h =
    (!isnan(pressureNow) && !isnan(p6))
      ? pressureNow - p6
      : NAN;

  weather.pressureDelta12h =
    (!isnan(pressureNow) && !isnan(p12))
      ? pressureNow - p12
      : NAN;

  weather.hourlyCount = 0;

  for (int source = nowIndex;
       source < (int)times.size() &&
       weather.hourlyCount < 48;
       source++) {
    HourData &hour =
      weather.hourly[weather.hourlyCount++];

    String timestamp =
      times[source].as<String>();

    if (timestamp.length() >= 16) {
      snprintf(
        hour.time,
        sizeof(hour.time),
        "%s",
        timestamp.substring(11, 16).c_str()
      );
    }

    hour.temperature =
      hourly["temperature_2m"][source] | NAN;

    hour.apparent =
      hourly["apparent_temperature"][source] | NAN;

    hour.humidity =
      hourly["relative_humidity_2m"][source] | NAN;

    hour.dewPoint =
      hourly["dew_point_2m"][source] | NAN;

    hour.pressure =
      hourly["pressure_msl"][source] | NAN;

    hour.cloudCover =
      hourly["cloud_cover"][source] | NAN;

    hour.visibility =
      hourly["visibility"][source] | NAN;

    hour.uv =
      hourly["uv_index"][source] | NAN;

    hour.wind =
      hourly["wind_speed_10m"][source] | NAN;

    hour.direction =
      hourly["wind_direction_10m"][source] | NAN;

    hour.gust =
      hourly["wind_gusts_10m"][source] | NAN;

    hour.precipitation =
      hourly["precipitation"][source] | NAN;

    hour.rain =
      hourly["rain"][source] | NAN;

    hour.rainChance =
      hourly["precipitation_probability"][source] | 0;

    hour.weatherCode =
      hourly["weather_code"][source] | -1;
  }

  JsonObject daily =
    document["daily"];

  JsonArray dates =
    daily["time"];

  weather.dailyCount =
    min(10, (int)dates.size());

  for (int i = 0;
       i < weather.dailyCount;
       i++) {
    DayData &day =
      weather.daily[i];

    String date =
      dates[i].as<String>();

    snprintf(
      day.date,
      sizeof(day.date),
      "%s",
      date.c_str()
    );

    day.low =
      daily["temperature_2m_min"][i] | NAN;

    day.high =
      daily["temperature_2m_max"][i] | NAN;

    day.precipitation =
      daily["precipitation_sum"][i] | NAN;

    day.rainChance =
      daily["precipitation_probability_max"][i] | 0;

    day.windMax =
      daily["wind_speed_10m_max"][i] | NAN;

    day.gustMax =
      daily["wind_gusts_10m_max"][i] | NAN;

    day.uvMax =
      daily["uv_index_max"][i] | NAN;

    day.weatherCode =
      daily["weather_code"][i] | -1;

    String sunrise =
      daily["sunrise"][i] | "";

    String sunset =
      daily["sunset"][i] | "";

    if (sunrise.length() >= 16) {
      snprintf(
        day.sunrise,
        sizeof(day.sunrise),
        "%s",
        sunrise.substring(11, 16).c_str()
      );
    }

    if (sunset.length() >= 16) {
      snprintf(
        day.sunset,
        sizeof(day.sunset),
        "%s",
        sunset.substring(11, 16).c_str()
      );
    }
  }

  weather.valid = true;
  weather.fetchedAt = time(nullptr);

  lastWeatherUpdate = millis();

  rebuildInsights();
  rebuildAlerts();
  storageSaveCache();

  Serial.printf(
    "[weather] %.1f C | %d hourly | %d daily\n",
    weather.temperature,
    weather.hourlyCount,
    weather.dailyCount
  );

  return true;
}

static String airURL() {
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

bool refreshAir() {
  Serial.println("[air] updating");

  JsonDocument document;

  if (!getJson(airURL(), document)) {
    Serial.println("[air] failed");
    return false;
  }

  JsonObject current =
    document["current"];

  air.pm25 =
    current["pm2_5"] | NAN;

  air.pm10 =
    current["pm10"] | NAN;

  air.co =
    current["carbon_monoxide"] | NAN;

  air.no2 =
    current["nitrogen_dioxide"] | NAN;

  air.so2 =
    current["sulphur_dioxide"] | NAN;

  air.ozone =
    current["ozone"] | NAN;

  air.europeanAqi =
    current["european_aqi"] | -1;

  air.usAqi =
    current["us_aqi"] | -1;

  air.valid =
    air.usAqi >= 0 ||
    !isnan(air.pm25);

  air.fetchedAt =
    time(nullptr);

  lastAirUpdate =
    millis();

  rebuildInsights();
  rebuildAlerts();

  Serial.printf(
    "[air] AQI %d | PM2.5 %.1f\n",
    air.usAqi,
    air.pm25
  );

  return air.valid;
}

void dataBegin() {
  configTzTime(
    TIMEZONE,
    "pool.ntp.org",
    "time.google.com",
    "time.cloudflare.com"
  );

  if (strlen(WIFI_SSID) == 0 ||
      strcmp(WIFI_SSID, "YOUR_WIFI_NAME") == 0) {
    Serial.println("[wifi] credentials not configured");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  Serial.printf(
    "[wifi] connecting to %s\n",
    WIFI_SSID
  );

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  lastWifiAttempt =
    millis();
}

void dataService() {
  bool connected =
    WiFi.status() ==
    WL_CONNECTED;

  if (connected != wifiConnected) {
    wifiConnected = connected;

    if (wifiConnected) {
      Serial.print("[wifi] connected: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("[wifi] disconnected");
    }
  }

  if (!wifiConnected &&
      millis() - lastWifiAttempt >= WIFI_RETRY_MS) {
    WiFi.disconnect();

    WiFi.begin(
      WIFI_SSID,
      WIFI_PASSWORD
    );

    lastWifiAttempt =
      millis();
  }

  if (!wifiConnected) return;

  if (!weather.valid ||
      millis() - lastWeatherUpdate >= WEATHER_REFRESH_MS) {
    refreshWeather();
  }

  if (!air.valid ||
      millis() - lastAirUpdate >= AIR_REFRESH_MS) {
    refreshAir();
  }
}
