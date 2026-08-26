#include "src/ui/Animations.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include <math.h>

namespace ui {

constexpr int MAX_PARTICLES = 16;

struct Particle {
    lv_obj_t* obj = nullptr;
    float x = 0;
    float y = 0;
    float vx = 0;
    float vy = 0;
    float size = 0;
};

static Particle        s_particles[MAX_PARTICLES];
static int             s_particleCount = 0;
static WeatherAnimMode s_animMode      = WeatherAnimMode::NONE;
static lv_timer_t*     s_animTimer     = nullptr;
static uint32_t        s_frame         = 0;
static lv_obj_t*       s_sunPulseObj   = nullptr;
static lv_obj_t*       s_lightningObj  = nullptr;

static void animTimerCb(lv_timer_t* timer) {
    s_frame++;

    // 1. Rain & Thunder Particles (60 FPS Fluid Physics)
    if (s_animMode == WeatherAnimMode::RAINY || s_animMode == WeatherAnimMode::THUNDER) {
        for (int i = 0; i < s_particleCount; i++) {
            Particle& p = s_particles[i];
            if (!p.obj) continue;

            p.y += p.vy;
            p.x += p.vx;

            if (p.y > 320 || p.x < 0 || p.x > 240) {
                p.y = (float)(-(rand() % 30));
                p.x = (float)(rand() % 240);
            }
            lv_obj_set_pos(p.obj, (int)p.x, (int)p.y);
        }

        // Thunder flash (every ~240 frames at 60 FPS)
        if (s_animMode == WeatherAnimMode::THUNDER && s_lightningObj) {
            if ((s_frame % 220 == 0) && (rand() % 3 == 0)) {
                lv_obj_set_style_bg_opa(s_lightningObj, LV_OPA_50, 0);
            } else {
                lv_obj_set_style_bg_opa(s_lightningObj, LV_OPA_TRANSP, 0);
            }
        }
    }

    // 2. Snow Particles (60 FPS Gentle sinusoidal drift)
    else if (s_animMode == WeatherAnimMode::SNOWY) {
        for (int i = 0; i < s_particleCount; i++) {
            Particle& p = s_particles[i];
            if (!p.obj) continue;

            p.y += p.vy;
            p.x += sinf((float)s_frame * 0.025f + (float)i) * 0.4f;

            if (p.y > 320) {
                p.y = (float)(-(rand() % 20));
                p.x = (float)(rand() % 240);
            }
            lv_obj_set_pos(p.obj, (int)p.x, (int)p.y);
        }
    }

    // 3. Sun Pulse (60 FPS Smooth halo breathing)
    else if (s_animMode == WeatherAnimMode::SUNNY && s_sunPulseObj) {
        float pulse = (sinf((float)s_frame * 0.035f) + 1.0f) * 0.5f; // 0.0 to 1.0
        int size = 52 + (int)(pulse * 14.0f);
        lv_obj_set_size(s_sunPulseObj, size, size);
        lv_obj_set_pos(s_sunPulseObj, 178 - size / 2, 79 - size / 2);
        lv_obj_set_style_bg_opa(s_sunPulseObj, (lv_opa_t)(LV_OPA_20 + (uint8_t)(pulse * 30.0f)), 0);
    }
}

void animStop() {
    s_animMode = WeatherAnimMode::NONE;
    s_particleCount = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        s_particles[i].obj = nullptr;
    }
    s_sunPulseObj  = nullptr;
    s_lightningObj = nullptr;
}

void animInit() {
    animStop();
    if (!s_animTimer) {
        s_animTimer = lv_timer_create(animTimerCb, 16, nullptr); // 60 FPS Hardware-accelerated timer
    }
}

void animService() {
    // LVGL drives timer automatically
}

void animAttachWeather(lv_obj_t* root, int weatherCode, bool isDay) {
    animStop();

    if (weatherCode < 0 || !root) {
        return;
    }

    // Determine animation mode from Open-Meteo weather code
    if (weatherCode >= 95) {
        s_animMode = WeatherAnimMode::THUNDER;
    } else if ((weatherCode >= 51 && weatherCode <= 67) || (weatherCode >= 80 && weatherCode <= 82)) {
        s_animMode = WeatherAnimMode::RAINY;
    } else if (weatherCode >= 71 && weatherCode <= 77) {
        s_animMode = WeatherAnimMode::SNOWY;
    } else if (weatherCode == 0 && isDay) {
        s_animMode = WeatherAnimMode::SUNNY;
    } else if (weatherCode <= 3) {
        s_animMode = WeatherAnimMode::CLOUDY;
    } else {
        s_animMode = WeatherAnimMode::NONE;
    }

    // 1. Setup Sunny Pulse Halo
    if (s_animMode == WeatherAnimMode::SUNNY) {
        s_sunPulseObj = panel(root, 150, 52, 54, 54, CLR_YELLOW, LV_RADIUS_CIRCLE);
        lv_obj_set_style_bg_opa(s_sunPulseObj, LV_OPA_30, 0);
        lv_obj_clear_flag(s_sunPulseObj, LV_OBJ_FLAG_CLICKABLE);
        return;
    }

    // 2. Setup Thunder Flash Overlay
    if (s_animMode == WeatherAnimMode::THUNDER) {
        s_lightningObj = panel(root, 0, 0, 240, 320, CLR_WHITE, 0);
        lv_obj_set_style_bg_opa(s_lightningObj, LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(s_lightningObj, LV_OBJ_FLAG_CLICKABLE);
    }

    // 3. Setup Rain Particles
    if (s_animMode == WeatherAnimMode::RAINY || s_animMode == WeatherAnimMode::THUNDER) {
        s_particleCount = 16;
        for (int i = 0; i < s_particleCount; i++) {
            Particle& p = s_particles[i];
            p.x = (float)(rand() % 240);
            p.y = (float)(rand() % 320);
            p.vx = -0.5f;
            p.vy = 2.6f + (float)(rand() % 3);

            p.obj = panel(root, (int)p.x, (int)p.y, 2, 8 + (rand() % 5), CLR_CYAN, 1);
            lv_obj_set_style_bg_opa(p.obj, (lv_opa_t)(LV_OPA_40 + (rand() % 40)), 0);
            lv_obj_clear_flag(p.obj, LV_OBJ_FLAG_CLICKABLE);
        }
    }

    // 4. Setup Snow Particles
    else if (s_animMode == WeatherAnimMode::SNOWY) {
        s_particleCount = 14;
        for (int i = 0; i < s_particleCount; i++) {
            Particle& p = s_particles[i];
            p.x = (float)(rand() % 240);
            p.y = (float)(rand() % 320);
            p.vx = 0.0f;
            p.vy = 0.7f + (float)(rand() % 2) * 0.4f;

            int sz = 3 + (rand() % 3);
            p.obj = panel(root, (int)p.x, (int)p.y, sz, sz, CLR_WHITE, LV_RADIUS_CIRCLE);
            lv_obj_set_style_bg_opa(p.obj, (lv_opa_t)(LV_OPA_50 + (rand() % 40)), 0);
            lv_obj_clear_flag(p.obj, LV_OBJ_FLAG_CLICKABLE);
        }
    }
}

void animScreenSlide(lv_obj_t* oldScreen, lv_obj_t* newScreen, bool forward) {
    // LVGL 9 screen loading animation
    if (newScreen) {
        lv_screen_load_anim(newScreen, 
                            forward ? LV_SCR_LOAD_ANIM_MOVE_LEFT : LV_SCR_LOAD_ANIM_MOVE_RIGHT, 
                            220, 0, true);
    }
}

}  // namespace ui
