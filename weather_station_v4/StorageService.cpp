#include "StorageService.h"

#include "Config.h"
#include "AppState.h"

#include <Preferences.h>
#include <SD.h>

static Preferences preferences;

void storageLoadCache() {
  preferences.begin(
    "weather-v4",
    true
  );

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

  weather.direction =
    preferences.getFloat("direction", NAN);

  weather.uv =
    preferences.getFloat("uv", NAN);

  weather.visibility =
    preferences.getFloat("visibility", NAN);

  weather.weatherCode =
    preferences.getInt("code", -1);

  weather.isDay =
    preferences.getBool("day", true);

  weather.fetchedAt =
    (time_t)preferences.getLong64("time", 0);

  weather.valid = true;

  preferences.end();

  rebuildInsights();
  rebuildAlerts();

  Serial.println("[cache] restored");
}

void storageSaveCache() {
  if (!weather.valid) return;

  preferences.begin(
    "weather-v4",
    false
  );

  preferences.putBool(
    "valid",
    true
  );

  preferences.putFloat(
    "temp",
    weather.temperature
  );

  preferences.putFloat(
    "apparent",
    weather.apparent
  );

  preferences.putFloat(
    "humidity",
    weather.humidity
  );

  preferences.putFloat(
    "dew",
    weather.dewPoint
  );

  preferences.putFloat(
    "pressure",
    weather.pressure
  );

  preferences.putFloat(
    "wind",
    weather.wind
  );

  preferences.putFloat(
    "gust",
    weather.gust
  );

  preferences.putFloat(
    "direction",
    weather.direction
  );

  preferences.putFloat(
    "uv",
    weather.uv
  );

  preferences.putFloat(
    "visibility",
    weather.visibility
  );

  preferences.putInt(
    "code",
    weather.weatherCode
  );

  preferences.putBool(
    "day",
    weather.isDay
  );

  preferences.putLong64(
    "time",
    (int64_t)weather.fetchedAt
  );

  preferences.end();
}

void storageLoadHistory() {
  historyCount = 0;

  if (!sdAvailable) return;

  File file =
    SD.open(
      "/weather.csv",
      FILE_READ
    );

  if (!file) return;

  // Skip header.
  file.readStringUntil('\n');

  while (file.available()) {
    String line =
      file.readStringUntil('\n');

    line.trim();

    if (!line.length()) continue;

    HistoryPoint point;

    int field = 0;
    int start = 0;

    String values[5];

    for (int i = 0; i <= (int)line.length(); i++) {
      if (i == (int)line.length() || line[i] == ',') {
        if (field < 5) {
          values[field] =
            line.substring(start, i);
        }

        field++;
        start = i + 1;
      }
    }

    if (field < 5) continue;

    point.timestamp =
      (time_t)values[0].toInt();

    point.temperature =
      values[1].toFloat();

    point.humidity =
      values[2].toFloat();

    point.pressure =
      values[3].toFloat();

    point.aqi =
      values[4].toFloat();

    if (historyCount < 96) {
      historyPoints[historyCount++] =
        point;
    } else {
      for (int i = 1; i < 96; i++) {
        historyPoints[i - 1] =
          historyPoints[i];
      }

      historyPoints[95] =
        point;
    }
  }

  file.close();

  Serial.printf(
    "[history] loaded %d points\n",
    historyCount
  );
}

void storageLogHistory() {
  if (!sdAvailable ||
      !weather.valid) {
    return;
  }

  File file =
    SD.open(
      "/weather.csv",
      FILE_APPEND
    );

  if (!file) return;

  time_t now =
    time(nullptr);

  file.printf(
    "%lld,%.2f,%.2f,%.2f,%.2f\n",
    (long long)now,
    weather.temperature,
    weather.humidity,
    weather.pressure,
    air.valid ? (float)air.usAqi : NAN
  );

  file.close();

  HistoryPoint point;

  point.timestamp = now;
  point.temperature = weather.temperature;
  point.humidity = weather.humidity;
  point.pressure = weather.pressure;
  point.aqi = air.valid ? (float)air.usAqi : NAN;

  if (historyCount < 96) {
    historyPoints[historyCount++] =
      point;
  } else {
    for (int i = 1; i < 96; i++) {
      historyPoints[i - 1] =
        historyPoints[i];
    }

    historyPoints[95] =
      point;
  }

  lastHistoryWrite =
    millis();

  Serial.println(
    "[history] logged"
  );
}

void storageBegin() {
  storageLoadCache();

  if (!ENABLE_SD_LOGGING) {
    Serial.println("[sd] disabled");
    return;
  }

  sdAvailable =
    SD.begin(
      SD_CS
    );

  if (!sdAvailable) {
    Serial.println("[sd] not mounted");
    return;
  }

  Serial.println("[sd] ready");

  if (!SD.exists("/weather.csv")) {
    File file =
      SD.open(
        "/weather.csv",
        FILE_WRITE
      );

    if (file) {
      file.println(
        "time,temp,humidity,pressure,aqi"
      );

      file.close();
    }
  }

  storageLoadHistory();
}

void storageService() {
  if (!sdAvailable ||
      !weather.valid) {
    return;
  }

  if (millis() -
      lastHistoryWrite >=
      HISTORY_LOG_MS) {
    storageLogHistory();
  }
}
