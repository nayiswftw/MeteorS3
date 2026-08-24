#pragma once

/*
 * Weather Art / Procedural Icons — Drawn using LVGL primitives.
 */

#include <lvgl.h>

namespace ui {

void weatherArt(
    lv_obj_t* root,
    int weatherCode,
    bool isDay,
    int x, int y,
    int scale = 1
);

}  // namespace ui
