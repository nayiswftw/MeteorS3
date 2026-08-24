#include "src/ui/Icons.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include <math.h>

namespace ui {

static void sunArt(lv_obj_t* root, int centerX, int centerY, int scale) {
    int radius = 12 * scale;
    // Outer glowing halo
    circle(root, centerX - radius - 3 * scale, centerY - radius - 3 * scale, (radius + 3 * scale) * 2, CLR_ORANGE, LV_OPA_30);
    // Core sun
    circle(root, centerX - radius, centerY - radius, radius * 2, CLR_YELLOW);

    // 8 Radial rays
    for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159265f / 4.0f;
        int x1 = centerX + (int)(cosf(angle) * (radius + 4 * scale));
        int y1 = centerY + (int)(sinf(angle) * (radius + 4 * scale));
        int x2 = centerX + (int)(cosf(angle) * (radius + 10 * scale));
        int y2 = centerY + (int)(sinf(angle) * (radius + 10 * scale));

        line(root, x1, y1, x2, y2, CLR_YELLOW, 2 * scale);
    }
}

static void moonArt(lv_obj_t* root, int centerX, int centerY, int scale) {
    int radius = 12 * scale;
    // Moon main body
    circle(root, centerX - radius, centerY - radius, radius * 2, CLR_YELLOW, LV_OPA_80);
    // Cutout shadow to make crescent
    circle(root, centerX - radius + 6 * scale, centerY - radius - 4 * scale, radius * 2, CLR_BG_TOP);

    // Subtle star dots
    panel(root, centerX - 14 * scale, centerY - 8 * scale, 2 * scale, 2 * scale, CLR_WHITE, 1);
    panel(root, centerX + 12 * scale, centerY + 10 * scale, 2 * scale, 2 * scale, CLR_CYAN, 1);
}

static void cloudArt(lv_obj_t* root, int x, int y, int scale, lv_color_t color) {
    // Back shadow puff
    circle(root, x + 16 * scale, y + 1 * scale,  22 * scale, CLR_PANEL_BORDER, LV_OPA_40);
    // Main cloud puffs
    circle(root, x + 7 * scale,  y + 10 * scale, 18 * scale, color);
    circle(root, x + 20 * scale, y + 3 * scale,  24 * scale, color);
    circle(root, x + 36 * scale, y + 10 * scale, 18 * scale, color);
    panel(root,  x + 4 * scale,  y + 14 * scale, 42 * scale, 14 * scale, color, 6 * scale);
}

static void lightningBolt(lv_obj_t* root, int x, int y, int scale) {
    line(root, x + 18 * scale, y + 28 * scale, x + 12 * scale, y + 40 * scale, CLR_YELLOW, 3 * scale);
    line(root, x + 12 * scale, y + 40 * scale, x + 18 * scale, y + 40 * scale, CLR_YELLOW, 3 * scale);
    line(root, x + 18 * scale, y + 40 * scale, x + 10 * scale, y + 54 * scale, CLR_YELLOW, 3 * scale);
}

void weatherArt(
    lv_obj_t* root,
    int code,
    bool isDay,
    int x, int y,
    int scale
) {
    if (code == 0) { // Clear sky
        if (isDay) {
            sunArt(root, x + 25 * scale, y + 25 * scale, scale);
        } else {
            moonArt(root, x + 25 * scale, y + 25 * scale, scale);
        }
        return;
    }

    if (code == 1 || code == 2) { // Mainly clear / partly cloudy
        if (isDay) {
            sunArt(root, x + 14 * scale, y + 12 * scale, scale);
        } else {
            moonArt(root, x + 14 * scale, y + 12 * scale, scale);
        }
        cloudArt(root, x + 8 * scale, y + 18 * scale, scale, CLR_CLOUD_LIGHT);
        return;
    }

    if (code == 3 || code == 45 || code == 48) { // Overcast / Fog
        cloudArt(root, x, y + 6 * scale, scale, CLR_CLOUD_LIGHT);
        if (code == 45 || code == 48) {
            line(root, x + 4 * scale, y + 36 * scale, x + 44 * scale, y + 36 * scale, CLR_MUTED, 2 * scale);
            line(root, x + 8 * scale, y + 42 * scale, x + 40 * scale, y + 42 * scale, CLR_DIM, 2 * scale);
        }
        return;
    }

    if (code >= 95) { // Thunderstorm
        cloudArt(root, x, y + 2 * scale, scale, CLR_CLOUD_DARK);
        lightningBolt(root, x, y, scale);
        return;
    }

    if (code >= 71 && code <= 77) { // Snow
        cloudArt(root, x, y + 4 * scale, scale, CLR_CLOUD_LIGHT);
        for (int i = 0; i < 3; i++) {
            int dx = x + (12 + i * 12) * scale;
            circle(root, dx, y + 34 * scale, 4 * scale, CLR_WHITE);
        }
        return;
    }

    if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) { // Rain / Showers
        cloudArt(root, x, y + 2 * scale, scale, CLR_CLOUD_DARK);
        for (int i = 0; i < 3; i++) {
            int dx = x + (12 + i * 12) * scale;
            line(root, dx, y + 32 * scale, dx - 4 * scale, y + 44 * scale, CLR_CYAN, 2 * scale);
        }
        return;
    }

    cloudArt(root, x, y + 8 * scale, scale, CLR_CLOUD_DARK);
}

void lunarArt(
    lv_obj_t* root,
    int x, int y,
    int diameter,
    float phase
) {
    int radius = diameter / 2;
    int centerX = x + radius;
    int centerY = y + radius;

    // Dark lunar background circle
    circle(root, x, y, diameter, CLR_PANEL_ALT);

    // Illuminated portion
    if (phase >= 0.05f && phase <= 0.95f) {
        // Bright moon disc
        circle(root, x, y, diameter, CLR_YELLOW, LV_OPA_90);

        // Calculate shadow offset
        float shadowOffset = (phase <= 0.5f) 
                             ? (1.0f - (phase / 0.5f)) * (float)diameter 
                             : ((phase - 0.5f) / 0.5f) * (float)diameter;

        int sx = (phase <= 0.5f) ? (x + (int)shadowOffset) : (x - (int)shadowOffset);
        circle(root, sx, y, diameter, CLR_PANEL_ALT);
    }

    // Subtle lunar crater textures
    circle(root, centerX - radius / 3, centerY - radius / 4, diameter / 6, CLR_DIM, LV_OPA_30);
    circle(root, centerX + radius / 4, centerY + radius / 3, diameter / 5, CLR_DIM, LV_OPA_30);
    circle(root, centerX - radius / 5, centerY + radius / 3, diameter / 8, CLR_DIM, LV_OPA_30);
}

}  // namespace ui

