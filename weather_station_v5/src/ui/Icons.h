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

/// Procedurally renders high-fidelity lunar phase graphic (phase: 0.0 = New, 0.25 = First Qtr, 0.5 = Full, 0.75 = Last Qtr)
void lunarArt(
    lv_obj_t* root,
    int x, int y,
    int diameter,
    float phase
);

}  // namespace ui

