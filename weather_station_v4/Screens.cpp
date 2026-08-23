#include "Screens.h"

#include "UiComponents.h"
#include "UiTheme.h"
#include "Config.h"
#include "AppState.h"
#include "Hardware.h"
#include "DataService.h"

#include <WiFi.h>

static ScreenId currentScreen =
  SCREEN_HOME;

static String tempText(
  float c
) {
  if (isnan(c)) return "--";

  if (temperatureUnit == TEMP_F) {
    return
      String(
        c * 9.0f / 5.0f + 32.0f,
        0
      ) +
      " F";
  }

  return
    String(c, 0) +
    " C";
}

static String windText(
  float kmh
) {
  if (isnan(kmh)) return "--";

  if (windUnit == WIND_MPH) {
    return
      String(
        kmh * 0.621371f,
        0
      ) +
      " mph";
  }

  if (windUnit == WIND_MS) {
    return
      String(
        kmh / 3.6f,
        1
      ) +
      " m/s";
  }

  return
    String(kmh, 0) +
    " km/h";
}

static String pressureText(
  float hpa
) {
  if (isnan(hpa)) return "--";

  if (pressureUnit == PRESS_INHG) {
    return
      String(
        hpa * 0.029529983f,
        2
      ) +
      " inHg";
  }

  return
    String(hpa, 0) +
    " hPa";
}

static String weatherName(
  int code
) {
  if (code == 0) return "Clear";
  if (code == 1) return "Mostly clear";
  if (code == 2) return "Partly cloudy";
  if (code == 3) return "Overcast";

  if (code == 45 ||
      code == 48) {
    return "Fog";
  }

  if (code >= 51 &&
      code <= 57) {
    return "Drizzle";
  }

  if (code >= 61 &&
      code <= 67) {
    return "Rain";
  }

  if (code >= 71 &&
      code <= 77) {
    return "Snow";
  }

  if (code >= 80 &&
      code <= 82) {
    return "Showers";
  }

  if (code >= 95) {
    return "Thunderstorm";
  }

  return "Weather";
}

static String directionText(
  float degrees
) {
  if (isnan(degrees)) {
    return "--";
  }

  static const char *directions[] = {
    "N", "NNE", "NE", "ENE",
    "E", "ESE", "SE", "SSE",
    "S", "SSW", "SW", "WSW",
    "W", "WNW", "NW", "NNW"
  };

  int index =
    (int)(
      (degrees + 11.25f) /
      22.5f
    ) %
    16;

  return directions[index];
}

static String ageText(
  time_t timestamp
) {
  if (timestamp <= 0) {
    return "cached";
  }

  time_t now =
    time(nullptr);

  if (now <= 0 ||
      now < timestamp) {
    return "cached";
  }

  long minutes =
    (long)(
      (now - timestamp) /
      60
    );

  if (minutes < 1) {
    return "live";
  }

  if (minutes < 60) {
    return
      String(minutes) +
      "m ago";
  }

  return
    String(minutes / 60) +
    "h ago";
}

static lv_color_t aqiColor(
  int aqi
) {
  if (aqi < 0) return UI_MUTED;
  if (aqi <= 50) return UI_GREEN;
  if (aqi <= 100) return UI_YELLOW;
  if (aqi <= 150) return UI_ORANGE;
  return UI_RED;
}

static String aqiName(
  int aqi
) {
  if (aqi < 0) return "--";
  if (aqi <= 50) return "Good";
  if (aqi <= 100) return "Moderate";
  if (aqi <= 150) return "Sensitive";
  if (aqi <= 200) return "Unhealthy";
  if (aqi <= 300) return "Very unhealthy";
  return "Hazardous";
}

static void nav() {
  ui::navDots(
    currentScreen,
    SCREEN_COUNT
  );
}

static void homeScreen() {
  ui::clear();

  ui::gradientBackground(
    weather.isDay
      ? lv_color_hex(0x0A2A39)
      : lv_color_hex(0x14162C),
    UI_BG_BOTTOM
  );

  ui::header();

  if (!weather.valid) {
    ui::label(
      displayRoot(),
      "--",
      10, 83,
      130, 45,
      &lv_font_montserrat_32,
      UI_TEXT
    );

    ui::label(
      displayRoot(),
      "Waiting for weather",
      10, 135,
      220, 20,
      &lv_font_montserrat_14,
      UI_MUTED
    );

    nav();
    return;
  }

  ui::weatherArt(
    weather.weatherCode,
    weather.isDay,
    158, 59,
    1
  );

  ui::label(
    displayRoot(),
    tempText(
      weather.temperature
    ),
    10, 72,
    143, 45,
    &lv_font_montserrat_32,
    UI_TEXT
  );

  ui::label(
    displayRoot(),
    weatherName(
      weather.weatherCode
    ),
    12, 118,
    140, 19,
    &lv_font_montserrat_14,
    UI_TEXT
  );

  String secondary =
    "Feels " +
    tempText(
      weather.apparent
    );

  if (weather.dailyCount > 0) {
    secondary +=
      "  H " +
      tempText(
        weather.daily[0].high
      );

    secondary +=
      "  L " +
      tempText(
        weather.daily[0].low
      );
  }

  ui::label(
    displayRoot(),
    secondary,
    12, 139,
    218, 17,
    &lv_font_montserrat_14,
    UI_MUTED
  );

  lv_obj_t *insight =
    ui::panel(
      displayRoot(),
      10, 164,
      220, 49,
      UI_PANEL_ALT,
      18
    );

  ui::label(
    insight,
    insights.primary,
    12, 8,
    196, 17,
    &lv_font_montserrat_14,
    UI_TEXT
  );

  ui::label(
    insight,
    insights.secondary,
    12, 27,
    196, 15,
    &lv_font_montserrat_14,
    UI_DIM
  );

  int rainChance =
    weather.hourlyCount
      ? weather.hourly[0].rainChance
      : 0;

  ui::metricCard(
    10, 224,
    105, 65,
    "RAIN",
    String(rainChance) + "%",
    insights.nextRainHours < 0
      ? "No strong signal"
      : "Next in " +
        String(
          insights.nextRainHours
        ) +
        "h",
    UI_BLUE
  );

  ui::metricCard(
    125, 224,
    105, 65,
    "AQI",
    air.valid
      ? String(air.usAqi)
      : "--",
    air.valid
      ? aqiName(air.usAqi)
      : ageText(
          weather.fetchedAt
        ),
    air.valid
      ? aqiColor(air.usAqi)
      : UI_MUTED
  );

  nav();
}

static void hourlyScreen() {
  ui::clear();

  ui::gradientBackground(
    lv_color_hex(0x0C2834),
    UI_BG_BOTTOM
  );

  ui::header(
    "48 HOUR FORECAST"
  );

  if (!weather.valid ||
      weather.hourlyCount == 0) {
    nav();
    return;
  }

  float temperature[16];
  float apparent[16];

  int count =
    min(
      16,
      weather.hourlyCount
    );

  for (int i = 0;
       i < count;
       i++) {
    temperature[i] =
      weather.hourly[i].temperature;

    apparent[i] =
      weather.hourly[i].apparent;
  }

  ui::label(
    displayRoot(),
    "Temperature",
    10, 56,
    100, 16,
    &lv_font_montserrat_14,
    UI_MUTED
  );

  ui::chart(
    10, 78,
    220, 85,
    temperature,
    count,
    UI_CYAN
  );

  for (int i = 0;
       i < 4;
       i++) {
    int index =
      i * 4;

    if (index >= count) {
      break;
    }

    ui::label(
      displayRoot(),
      i == 0
        ? "NOW"
        : String(
            weather.hourly[index].time
          ),
      9 + i * 58,
      168,
      52, 16,
      &lv_font_montserrat_14,
      i == 0
        ? UI_CYAN
        : UI_MUTED,
      LV_TEXT_ALIGN_CENTER
    );
  }

  lv_obj_t *strip =
    ui::panel(
      displayRoot(),
      10, 197,
      220, 92,
      UI_PANEL,
      18
    );

  for (int i = 0;
       i < 4;
       i++) {
    int index =
      i * 2;

    if (index >=
        weather.hourlyCount) {
      break;
    }

    HourData &hour =
      weather.hourly[index];

    int x =
      i * 55;

    ui::label(
      strip,
      tempText(
        hour.temperature
      ),
      x, 9,
      55, 17,
      &lv_font_montserrat_14,
      UI_TEXT,
      LV_TEXT_ALIGN_CENTER
    );

    ui::label(
      strip,
      String(
        hour.rainChance
      ) +
      "%",
      x, 34,
      55, 17,
      &lv_font_montserrat_14,
      hour.rainChance >= 50
        ? UI_BLUE
        : UI_DIM,
      LV_TEXT_ALIGN_CENTER
    );

    ui::label(
      strip,
      windText(
        hour.wind
      ),
      x, 59,
      55, 17,
      &lv_font_montserrat_14,
      UI_MUTED,
      LV_TEXT_ALIGN_CENTER
    );
  }

  nav();
}

static void weekScreen() {
  ui::clear();

  ui::gradientBackground(
    lv_color_hex(0x102431),
    UI_BG_BOTTOM
  );

  ui::header(
    "10 DAY OUTLOOK"
  );

  if (!weather.valid ||
      weather.dailyCount == 0) {
    nav();
    return;
  }

  int days =
    min(
      7,
      weather.dailyCount
    );

  float globalLow = 999;
  float globalHigh = -999;

  for (int i = 0;
       i < days;
       i++) {
    globalLow =
      min(
        globalLow,
        weather.daily[i].low
      );

    globalHigh =
      max(
        globalHigh,
        weather.daily[i].high
      );
  }

  int y = 57;

  for (int i = 0;
       i < days;
       i++) {
    DayData &day =
      weather.daily[i];

    String date =
      i == 0
        ? "TODAY"
        : String(
            day.date + 5
          );

    ui::label(
      displayRoot(),
      date,
      10, y,
      48, 17,
      &lv_font_montserrat_14,
      i == 0
        ? UI_CYAN
        : UI_MUTED
    );

    ui::label(
      displayRoot(),
      tempText(day.low),
      61, y,
      40, 17,
      &lv_font_montserrat_14,
      UI_MUTED,
      LV_TEXT_ALIGN_RIGHT
    );

    ui::rangeBar(
      111,
      y + 7,
      74,
      day.low,
      day.high,
      globalLow,
      globalHigh,
      i == 0
        ? UI_YELLOW
        : UI_ORANGE
    );

    ui::label(
      displayRoot(),
      tempText(day.high),
      191, y,
      39, 17,
      &lv_font_montserrat_14,
      UI_TEXT,
      LV_TEXT_ALIGN_RIGHT
    );

    ui::label(
      displayRoot(),
      String(day.rainChance) + "%",
      61, y + 18,
      40, 14,
      &lv_font_montserrat_14,
      day.rainChance >= 50
        ? UI_BLUE
        : UI_DIM,
      LV_TEXT_ALIGN_RIGHT
    );

    y += 35;
  }

  nav();
}

static void rainScreen() {
  ui::clear();

  ui::gradientBackground(
    lv_color_hex(0x0A1E31),
    UI_BG_BOTTOM
  );

  ui::header(
    "RAIN CENTER"
  );

  if (!weather.valid ||
      weather.hourlyCount == 0) {
    nav();
    return;
  }

  int peakChance = 0;
  float total12h = 0;

  int peakHour = 0;

  float rainValues[12];

  int count =
    min(
      12,
      weather.hourlyCount
    );

  for (int i = 0;
       i < count;
       i++) {
    HourData &hour =
      weather.hourly[i];

    rainValues[i] =
      hour.precipitation;

    total12h +=
      isnan(hour.precipitation)
        ? 0
        : hour.precipitation;

    if (hour.rainChance >
        peakChance) {
      peakChance =
        hour.rainChance;

      peakHour =
        i;
    }
  }

  ui::metricCard(
    10, 57,
    105, 71,
    "NEXT RAIN",
    insights.nextRainHours < 0
      ? "None"
      : String(
          insights.nextRainHours
        ) +
        "h",
    peakChance
      ? "Peak " +
        String(peakChance) +
        "%"
      : "No strong signal",
    UI_BLUE
  );

  ui::metricCard(
    125, 57,
    105, 71,
    "12H TOTAL",
    String(
      total12h,
      1
    ) +
    " mm",
    peakChance
      ? String(
          weather.hourly[peakHour].time
        ) +
        " wettest"
      : "Dry",
    UI_CYAN
  );

  ui::label(
    displayRoot(),
    "Precipitation",
    10, 143,
    120, 16,
    &lv_font_montserrat_14,
    UI_MUTED
  );

  ui::chart(
    10, 166,
    220, 92,
    rainValues,
    count,
    UI_BLUE
  );

  ui::label(
    displayRoot(),
    "Probability now " +
    String(
      weather.hourly[0].rainChance
    ) +
    "%",
    10, 268,
    220, 18,
    &lv_font_montserrat_14,
    weather.hourly[0].rainChance >= 60
      ? UI_BLUE
      : UI_MUTED,
    LV_TEXT_ALIGN_CENTER
  );

  nav();
}

static void windScreen() {
  ui::clear();

  ui::gradientBackground(
    lv_color_hex(0x0C272A),
    UI_BG_BOTTOM
  );

  ui::header(
    "WIND CENTER"
  );

  if (!weather.valid) {
    nav();
    return;
  }

  ui::gauge(
    10, 57,
    100,
    (int)constrain(
      weather.wind,
      0.0f,
      100.0f
    ),
    0, 100,
    windText(
      weather.wind
    ),
    directionText(
      weather.direction
    ),
    UI_GREEN
  );

  ui::gauge(
    130, 57,
    100,
    (int)constrain(
      weather.gust,
      0.0f,
      120.0f
    ),
    0, 120,
    windText(
      weather.gust
    ),
    "Gust",
    weather.gust >= 50
      ? UI_ORANGE
      : UI_CYAN
  );

  float gusts[12];

  int count =
    min(
      12,
      weather.hourlyCount
    );

  for (int i = 0;
       i < count;
       i++) {
    gusts[i] =
      weather.hourly[i].gust;
  }

  ui::label(
    displayRoot(),
    "Gust trend",
    10, 174,
    100, 16,
    &lv_font_montserrat_14,
    UI_MUTED
  );

  ui::chart(
    10, 196,
    220, 89,
    gusts,
    count,
    UI_GREEN
  );

  nav();
}

static void atmosphereScreen() {
  ui::clear();

  ui::gradientBackground(
    lv_color_hex(0x12202B),
    UI_BG_BOTTOM
  );

  ui::header(
    "ATMOSPHERE"
  );

  if (!weather.valid) {
    nav();
    return;
  }

  ui::metricCard(
    10, 57,
    105, 73,
    "HUMIDITY",
    String(
      weather.humidity,
      0
    ) +
    "%",
    "Dew " +
    tempText(
      weather.dewPoint
    ),
    UI_BLUE
  );

  ui::metricCard(
    125, 57,
    105, 73,
    "CLOUDS",
    String(
      weather.cloudCover,
      0
    ) +
    "%",
    isnan(
      weather.visibility
    )
      ? "Vis --"
      : "Vis " +
        String(
          weather.visibility /
          1000.0f,
          1
        ) +
        " km",
    UI_CYAN
  );

  String trend =
    weather.pressureDelta6h > 2
      ? "Rising"
      : weather.pressureDelta6h < -2
        ? "Falling"
        : "Stable";

  ui::metricCard(
    10, 141,
    220, 66,
    "PRESSURE",
    pressureText(
      weather.pressure
    ),
    trend +
    "  3h " +
    String(
      weather.pressureDelta3h,
      1
    ) +
    "  6h " +
    String(
      weather.pressureDelta6h,
      1
    ),
    UI_PURPLE
  );

  float pressureValues[12];

  int count =
    min(
      12,
      weather.hourlyCount
    );

  for (int i = 0;
       i < count;
       i++) {
    pressureValues[i] =
      weather.hourly[i].pressure;
  }

  ui::chart(
    10, 220,
    220, 66,
    pressureValues,
    count,
    UI_PURPLE
  );

  nav();
}

static void airScreen() {
  ui::clear();

  ui::gradientBackground(
    lv_color_hex(0x12241D),
    UI_BG_BOTTOM
  );

  ui::header(
    "AIR QUALITY"
  );

  if (!air.valid) {
    nav();
    return;
  }

  lv_color_t color =
    aqiColor(
      air.usAqi
    );

  ui::gauge(
    60, 52,
    120,
    constrain(
      air.usAqi,
      0,
      300
    ),
    0, 300,
    String(
      air.usAqi
    ),
    aqiName(
      air.usAqi
    ),
    color
  );

  ui::metricCard(
    10, 185,
    105, 48,
    "PM2.5",
    String(air.pm25, 1),
    "ug/m3",
    color
  );

  ui::metricCard(
    125, 185,
    105, 48,
    "PM10",
    String(air.pm10, 1),
    "ug/m3",
    UI_ORANGE
  );

  ui::metricCard(
    10, 242,
    105, 48,
    "OZONE",
    String(air.ozone, 0),
    "ug/m3",
    UI_CYAN
  );

  ui::metricCard(
    125, 242,
    105, 48,
    "NO2",
    String(air.no2, 0),
    "ug/m3",
    UI_PURPLE
  );

  nav();
}

static void uvScreen() {
  ui::clear();

  ui::gradientBackground(
    lv_color_hex(0x30261A),
    UI_BG_BOTTOM
  );

  ui::header(
    "UV CENTER"
  );

  if (!weather.valid) {
    nav();
    return;
  }

  int gaugeValue =
    (int)constrain(
      weather.uv * 10.0f,
      0.0f,
      110.0f
    );

  ui::gauge(
    60, 52,
    120,
    gaugeValue,
    0, 110,
    String(
      weather.uv,
      1
    ),
    weather.uv >= 8
      ? "Very high"
      : weather.uv >= 6
        ? "High"
        : weather.uv >= 3
          ? "Moderate"
          : "Low",
    weather.uv >= 6
      ? UI_YELLOW
      : UI_GREEN
  );

  float uvValues[12];

  int count =
    min(
      12,
      weather.hourlyCount
    );

  float peak = -1;
  int peakIndex = 0;

  for (int i = 0;
       i < count;
       i++) {
    uvValues[i] =
      weather.hourly[i].uv;

    if (uvValues[i] > peak) {
      peak =
        uvValues[i];

      peakIndex =
        i;
    }
  }

  ui::label(
    displayRoot(),
    "Next 12 hours",
    10, 187,
    120, 16,
    &lv_font_montserrat_14,
    UI_MUTED
  );

  ui::chart(
    10, 208,
    220, 64,
    uvValues,
    count,
    UI_YELLOW
  );

  ui::label(
    displayRoot(),
    peak >= 0
      ? "Peak " +
        String(
          peak,
          1
        ) +
        " at " +
        String(
          weather.hourly[peakIndex].time
        )
      : "No UV forecast",
    10, 278,
    220, 17,
    &lv_font_montserrat_14,
    UI_MUTED,
    LV_TEXT_ALIGN_CENTER
  );

  nav();
}

static String moonPhase() {
  struct tm now;

  if (!getLocalTime(
        &now,
        20
      )) {
    return "Unknown";
  }

  int year =
    now.tm_year +
    1900;

  int month =
    now.tm_mon +
    1;

  int day =
    now.tm_mday;

  if (month < 3) {
    year--;
    month += 12;
  }

  long days =
    365L * year +
    year / 4 -
    year / 100 +
    year / 400 +
    (153L * (month + 1)) / 5 +
    day -
    730551L;

  double phase =
    fmod(
      days + 4.867,
      29.530588853
    );

  if (phase < 0) {
    phase +=
      29.530588853;
  }

  double fraction =
    phase /
    29.530588853;

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

static void sunScreen() {
  ui::clear();

  ui::gradientBackground(
    weather.isDay
      ? lv_color_hex(0x34291A)
      : lv_color_hex(0x17172D),
    UI_BG_BOTTOM
  );

  ui::header(
    "SUN & MOON"
  );

  if (!weather.valid) {
    nav();
    return;
  }

  ui::weatherArt(
    0,
    weather.isDay,
    90, 54,
    1
  );

  String sunrise =
    weather.dailyCount
      ? String(
          weather.daily[0].sunrise
        )
      : "--:--";

  String sunset =
    weather.dailyCount
      ? String(
          weather.daily[0].sunset
        )
      : "--:--";

  ui::metricCard(
    10, 143,
    105, 67,
    "SUNRISE",
    sunrise,
    weather.isDay
      ? "Today"
      : "Next morning",
    UI_YELLOW
  );

  ui::metricCard(
    125, 143,
    105, 67,
    "SUNSET",
    sunset,
    weather.isDay
      ? "Today"
      : "Passed",
    UI_ORANGE
  );

  ui::metricCard(
    10, 221,
    220, 68,
    "MOON",
    moonPhase(),
    weather.isDay
      ? "Tonight"
      : "Current night",
    UI_PURPLE
  );

  nav();
}

static void historyScreen() {
  ui::clear();

  ui::gradientBackground(
    lv_color_hex(0x111E2B),
    UI_BG_BOTTOM
  );

  ui::header(
    "LOCAL HISTORY"
  );

  if (historyCount < 2) {
    ui::label(
      displayRoot(),
      sdAvailable
        ? "Collecting history..."
        : "SD unavailable",
      10, 130,
      220, 20,
      &lv_font_montserrat_14,
      UI_MUTED,
      LV_TEXT_ALIGN_CENTER
    );

    nav();
    return;
  }

  int count =
    min(
      24,
      historyCount
    );

  float temperature[24];
  float pressure[24];

  int start =
    historyCount -
    count;

  for (int i = 0;
       i < count;
       i++) {
    temperature[i] =
      historyPoints[
        start + i
      ].temperature;

    pressure[i] =
      historyPoints[
        start + i
      ].pressure;
  }

  ui::label(
    displayRoot(),
    "Temperature",
    10, 57,
    100, 16,
    &lv_font_montserrat_14,
    UI_MUTED
  );

  ui::chart(
    10, 76,
    220, 83,
    temperature,
    count,
    UI_CYAN
  );

  ui::label(
    displayRoot(),
    "Pressure",
    10, 172,
    100, 16,
    &lv_font_montserrat_14,
    UI_MUTED
  );

  ui::chart(
    10, 191,
    220, 91,
    pressure,
    count,
    UI_PURPLE
  );

  nav();
}

static void alertsScreen() {
  ui::clear();

  ui::gradientBackground(
    lv_color_hex(0x251A20),
    UI_BG_BOTTOM
  );

  ui::header(
    "ALERT CENTER"
  );

  ui::label(
    displayRoot(),
    String(
      alertCount
    ),
    10, 57,
    55, 39,
    &lv_font_montserrat_32,
    alertCount == 1 &&
    alerts[0].severity == ALERT_INFO
      ? UI_GREEN
      : UI_YELLOW
  );

  ui::label(
    displayRoot(),
    "ACTIVE",
    68, 70,
    70, 16,
    &lv_font_montserrat_14,
    UI_MUTED
  );

  int y = 110;

  for (int i = 0;
       i < alertCount &&
       i < 5;
       i++) {
    lv_obj_t *row =
      ui::panel(
        displayRoot(),
        10, y,
        220, 35,
        UI_PANEL,
        15
      );

    lv_color_t color =
      ui::alertColor(
        alerts[i].severity
      );

    ui::panel(
      row,
      9, 12,
      8, 8,
      color,
      LV_RADIUS_CIRCLE
    );

    ui::label(
      row,
      alerts[i].title,
      26, 5,
      182, 15,
      &lv_font_montserrat_14,
      UI_TEXT
    );

    ui::label(
      row,
      alerts[i].detail,
      26, 19,
      182, 13,
      &lv_font_montserrat_14,
      UI_DIM
    );

    y += 42;
  }

  nav();
}

static void systemScreen() {
  ui::clear();

  ui::gradientBackground(
    lv_color_hex(0x101D27),
    UI_BG_BOTTOM
  );

  ui::header(
    "SYSTEM"
  );

  float voltage =
    readBatteryVoltage();

  ui::gauge(
    10, 57,
    98,
    readBatteryPercent(),
    0, 100,
    String(
      readBatteryPercent()
    ) +
    "%",
    "Battery",
    readBatteryPercent() <= 20
      ? UI_RED
      : UI_GREEN
  );

  int signal =
    wifiConnected
      ? constrain(
          WiFi.RSSI() +
          100,
          0,
          70
        )
      : 0;

  ui::gauge(
    132, 57,
    98,
    signal,
    0, 70,
    wifiConnected
      ? String(
          WiFi.RSSI()
        )
      : "--",
    "WiFi dBm",
    wifiConnected
      ? UI_CYAN
      : UI_DIM
  );

  ui::metricCard(
    10, 172,
    105, 54,
    "HEAP",
    String(
      ESP.getFreeHeap() /
      1024
    ) +
    " KB",
    "Free",
    UI_CYAN
  );

  ui::metricCard(
    125, 172,
    105, 54,
    "PSRAM",
    psramFound()
      ? String(
          ESP.getFreePsram() /
          1024
        ) +
        " KB"
      : "--",
    psramFound()
      ? "Free"
      : "Not found",
    UI_PURPLE
  );

  ui::metricCard(
    10, 237,
    220, 52,
    "STATUS",
    wifiConnected
      ? "ONLINE"
      : "OFFLINE",
    String(
      voltage,
      2
    ) +
    " V   SD " +
    (
      sdAvailable
        ? "ready"
        : "offline"
    ),
    wifiConnected
      ? UI_GREEN
      : UI_RED
  );

  nav();
}

static void renderCurrent() {
  switch (currentScreen) {
    case SCREEN_HOME:
      homeScreen();
      break;

    case SCREEN_HOURLY:
      hourlyScreen();
      break;

    case SCREEN_WEEK:
      weekScreen();
      break;

    case SCREEN_RAIN:
      rainScreen();
      break;

    case SCREEN_WIND:
      windScreen();
      break;

    case SCREEN_ATMOSPHERE:
      atmosphereScreen();
      break;

    case SCREEN_AIR:
      airScreen();
      break;

    case SCREEN_UV:
      uvScreen();
      break;

    case SCREEN_SUN:
      sunScreen();
      break;

    case SCREEN_HISTORY:
      historyScreen();
      break;

    case SCREEN_ALERTS:
      alertsScreen();
      break;

    case SCREEN_SYSTEM:
      systemScreen();
      break;

    default:
      currentScreen =
        SCREEN_HOME;

      homeScreen();
      break;
  }
}

void screensBegin() {
  ui::begin();

  rebuildInsights();
  rebuildAlerts();

  renderCurrent();
}

void screensShow(
  ScreenId screen
) {
  currentScreen =
    screen;

  renderCurrent();
}

void screensNext() {
  currentScreen =
    (ScreenId)(
      (
        (int)currentScreen +
        1
      ) %
      SCREEN_COUNT
    );

  renderCurrent();
}

void screensHome() {
  currentScreen =
    SCREEN_HOME;

  renderCurrent();
}

ScreenId screensCurrent() {
  return currentScreen;
}
