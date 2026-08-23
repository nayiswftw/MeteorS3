#pragma once

/*
 * UI Widgets — Reusable LVGL component builders.
 *
 * Pure, parameterized component functions that take parent objects
 * and explicit parameters without reading global hardware state.
 */

#include <lvgl.h>
#include "core/Types.h"
#include "ui/Theme.h"

namespace ui {

struct HeaderInfo {
    const char* location;
    const char* clockText;
    int         batteryPercent;
    bool        isOnline;
};

lv_obj_t* label(
    lv_obj_t* parent,
    const char* text,
    int x, int y,
    int width, int height,
    const lv_font_t* font = &lv_font_montserrat_14,
    lv_color_t color = CLR_TEXT,
    lv_text_align_t align = LV_TEXT_ALIGN_LEFT
);

lv_obj_t* panel(
    lv_obj_t* parent,
    int x, int y,
    int width, int height,
    lv_color_t color,
    int radius = layout::CARD_RADIUS
);

void circle(
    lv_obj_t* parent,
    int x, int y,
    int diameter,
    lv_color_t color,
    lv_opa_t opacity = LV_OPA_COVER
);

void line(
    lv_obj_t* parent,
    int x1, int y1,
    int x2, int y2,
    lv_color_t color,
    int width = 2
);

void gradientBackground(
    lv_obj_t* root,
    lv_color_t top,
    lv_color_t bottom = CLR_BG_BOTTOM
);

void header(
    lv_obj_t* root,
    const HeaderInfo& info,
    const char* subtitle = nullptr
);

void navDots(
    lv_obj_t* root,
    int current,
    int count
);

void metricCard(
    lv_obj_t* root,
    int x, int y,
    int width, int height,
    const char* title,
    const char* value,
    const char* caption,
    lv_color_t accent
);

void gauge(
    lv_obj_t* root,
    int x, int y,
    int diameter,
    int value,
    int minimum,
    int maximum,
    const char* centerText,
    const char* caption,
    lv_color_t color
);

lv_obj_t* chart(
    lv_obj_t* root,
    int x, int y,
    int width, int height,
    const float* values,
    int count,
    lv_color_t color
);

void rangeBar(
    lv_obj_t* root,
    int x, int y,
    int width,
    float low,
    float high,
    float globalLow,
    float globalHigh,
    lv_color_t color
);

lv_color_t alertColor(AlertSeverity severity);
lv_color_t aqiColor(int aqi);

}  // namespace ui
