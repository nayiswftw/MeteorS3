#pragma once
#include <Arduino.h>
#include <lvgl.h>

namespace ui {

void begin();
void clear();

lv_obj_t *label(
  lv_obj_t *parent,
  const String &text,
  int x, int y,
  int width, int height,
  const lv_font_t *font = &lv_font_montserrat_14,
  lv_color_t color = lv_color_white(),
  lv_text_align_t align = LV_TEXT_ALIGN_LEFT
);

lv_obj_t *panel(
  lv_obj_t *parent,
  int x, int y,
  int width, int height,
  lv_color_t color,
  int radius = 16
);

void gradientBackground(
  lv_color_t top,
  lv_color_t bottom
);

void header(
  const String &title = ""
);

void navDots(
  int current,
  int count
);

void weatherArt(
  int weatherCode,
  bool isDay,
  int x,
  int y,
  int scale = 1
);

void metricCard(
  int x, int y,
  int width, int height,
  const String &title,
  const String &value,
  const String &caption,
  lv_color_t accent
);

void gauge(
  int x, int y,
  int diameter,
  int value,
  int minimum,
  int maximum,
  const String &center,
  const String &caption,
  lv_color_t color
);

lv_obj_t *chart(
  int x, int y,
  int width, int height,
  const float *values,
  int count,
  lv_color_t color
);

void rangeBar(
  int x, int y,
  int width,
  float low,
  float high,
  float globalLow,
  float globalHigh,
  lv_color_t color
);

lv_color_t alertColor(int severity);
}
