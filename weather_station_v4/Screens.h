#pragma once

enum ScreenId {
  SCREEN_HOME,
  SCREEN_HOURLY,
  SCREEN_WEEK,
  SCREEN_RAIN,
  SCREEN_WIND,
  SCREEN_ATMOSPHERE,
  SCREEN_AIR,
  SCREEN_UV,
  SCREEN_SUN,
  SCREEN_HISTORY,
  SCREEN_ALERTS,
  SCREEN_SYSTEM,
  SCREEN_COUNT
};

void screensBegin();
void screensShow(ScreenId screen);
void screensNext();
void screensHome();
ScreenId screensCurrent();
