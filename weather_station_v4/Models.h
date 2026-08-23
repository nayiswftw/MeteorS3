#pragma once
#include <Arduino.h>

struct HourData {
  char time[6] = "--:--";
  float temperature = NAN;
  float apparent = NAN;
  float humidity = NAN;
  float dewPoint = NAN;
  float pressure = NAN;
  float cloudCover = NAN;
  float visibility = NAN;
  float uv = NAN;
  float wind = NAN;
  float gust = NAN;
  float direction = NAN;
  float precipitation = NAN;
  float rain = NAN;
  int rainChance = 0;
  int weatherCode = -1;
};

struct DayData {
  char date[11] = "";
  float low = NAN;
  float high = NAN;
  float precipitation = NAN;
  float windMax = NAN;
  float gustMax = NAN;
  float uvMax = NAN;
  int rainChance = 0;
  int weatherCode = -1;
  char sunrise[6] = "--:--";
  char sunset[6] = "--:--";
};

struct WeatherData {
  bool valid = false;

  float temperature = NAN;
  float apparent = NAN;
  float humidity = NAN;
  float dewPoint = NAN;
  float pressure = NAN;
  float pressureDelta3h = NAN;
  float pressureDelta6h = NAN;
  float pressureDelta12h = NAN;
  float wind = NAN;
  float gust = NAN;
  float direction = NAN;
  float precipitation = NAN;
  float rain = NAN;
  float visibility = NAN;
  float cloudCover = NAN;
  float uv = NAN;

  int weatherCode = -1;
  bool isDay = true;

  HourData hourly[48];
  int hourlyCount = 0;

  DayData daily[10];
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

struct HistoryPoint {
  time_t timestamp = 0;
  float temperature = NAN;
  float humidity = NAN;
  float pressure = NAN;
  float aqi = NAN;
};

struct InsightData {
  String primary;
  String secondary;

  int nextRainHours = -1;
  int wettestHour = -1;
  int hottestHour = -1;
  int coldestHour = -1;
  int strongestWindHour = -1;

  float hottestTemperature = NAN;
  float coldestTemperature = NAN;
  float strongestGust = NAN;
};

enum AlertSeverity {
  ALERT_INFO,
  ALERT_CAUTION,
  ALERT_WARNING,
  ALERT_DANGER
};

struct AlertItem {
  String title;
  String detail;
  AlertSeverity severity = ALERT_INFO;
};
