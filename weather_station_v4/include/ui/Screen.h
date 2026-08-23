#pragma once

/*
 * Screen System — Registry, navigation, and screen lifecycle.
 */

#include <lvgl.h>

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

struct ScreenDef {
    const char* title;               // Subtitle text (optional)
    lv_color_t  gradientTop;         // Screen background accent color
    void (*render)(lv_obj_t* root);  // Unique screen content renderer
};

namespace screen {

void init();
void show(ScreenId id);
void refresh();
void next();
void home();
ScreenId current();

bool registerScreen(ScreenId id, const ScreenDef& def);

}  // namespace screen
