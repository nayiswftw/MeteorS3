#pragma once
#include <Arduino.h>
#include <lvgl.h>

struct HourData {
  char time[6] = "--:--";
  float temperature = NAN;
  float apparent = NAN;
  float pressure = NAN;
  float wind = NAN;
  float gust = NAN;
  float direction = NAN;
  float precipitation = NAN;
  int rainChance = 0;
  int weatherCode = -1;
};

struct DayData {
  char date[11] = "";
  float low = NAN;
  float high = NAN;
  float precipitation = NAN;
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
  float pressureDelta6h = NAN;
  float wind = NAN;
  float gust = NAN;
  float direction = NAN;
  float precipitation = NAN;
  float rain = NAN;
  float visibility = NAN;
  float uv = NAN;
  int weatherCode = -1;
  bool isDay = true;
  HourData hourly[24];
  int hourlyCount = 0;
  DayData daily[8];
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
