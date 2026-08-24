#include "src/ui/Icons.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include <math.h>

namespace ui {

static void sunArt(lv_obj_t* root, int centerX, int centerY, int scale) {
    int radius = 11 * scale;
    circle(root, centerX - radius, centerY - radius, radius * 2, CLR_YELLOW);

    for (int i = 0; i < 8; i++) {
        float angle = i * 3.14159265f / 4.0f;
        int x1 = centerX + (int)(cos(angle) * (radius + 4));
        int y1 = centerY + (int)(sin(angle) * (radius + 4));
        int x2 = centerX + (int)(cos(angle) * (radius + 11));
        int y2 = centerY + (int)(sin(angle) * (radius + 11));

        line(root, x1, y1, x2, y2, CLR_YELLOW, 2);
    }
}

static void cloudArt(lv_obj_t* root, int x, int y, int scale, lv_color_t color) {
    circle(root, x + 7 * scale,  y + 10 * scale, 18 * scale, color);
    circle(root, x + 20 * scale, y + 3 * scale,  24 * scale, color);
    circle(root, x + 36 * scale, y + 10 * scale, 18 * scale, color);
    panel(root,  x + 4 * scale,  y + 14 * scale, 42 * scale, 13 * scale, color, 6 * scale);
}

void weatherArt(
    lv_obj_t* root,
    int code,
    bool isDay,
    int x, int y,
    int scale
) {
    if (code == 0) {
        sunArt(root, x + 25 * scale, y + 25 * scale, scale);
        return;
    }

    if (code == 1 || code == 2) {
        sunArt(root, x + 15 * scale, y + 12 * scale, scale);
        cloudArt(root, x + 8 * scale, y + 18 * scale, scale, CLR_CLOUD_LIGHT);
        return;
    }

    if (code <= 3 || code == 45 || code == 48) {
        cloudArt(root, x, y + 10 * scale, scale, CLR_CLOUD_LIGHT);
        return;
    }

    if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) {
        cloudArt(root, x, y, scale, CLR_CLOUD_DARK);
        for (int i = 0; i < 3; i++) {
            int dx = x + (12 + i * 12) * scale;
            line(root, dx, y + 32 * scale, dx - 3 * scale, y + 42 * scale, CLR_CYAN, 2);
        }
        return;
    }

    cloudArt(root, x, y + 10 * scale, scale, CLR_CLOUD_DARK);
}

}  // namespace ui
