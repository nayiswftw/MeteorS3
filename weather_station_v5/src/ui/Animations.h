#pragma once

#include <lvgl.h>
#include "src/core/Types.h"

namespace ui {

enum class WeatherAnimMode : uint8_t {
    NONE = 0,
    SUNNY,
    CLOUDY,
    RAINY,
    SNOWY,
    THUNDER
};

void animInit();
void animStop();
void animService();

/// Attach dynamic animated weather particles to the active root screen
void animAttachWeather(lv_obj_t* root, int weatherCode, bool isDay);

/// Trigger a smooth visual screen transition animation
void animScreenSlide(lv_obj_t* oldScreen, lv_obj_t* newScreen, bool forward);

}  // namespace ui
