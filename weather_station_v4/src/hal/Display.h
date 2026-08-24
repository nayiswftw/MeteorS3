#pragma once

/*
 * Display HAL — GFX initialization & LVGL 9 display driver.
 */

#include <lvgl.h>

namespace hal {

/// Initialize ST7789 display controller, backlight, and LVGL buffers.
void displayInit();

/// Call in the main loop to drive LVGL timers and rendering.
void displayService();

/// Get the active LVGL screen object.
lv_obj_t* displayRoot();

}  // namespace hal
