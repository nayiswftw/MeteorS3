#include "src/ui/Screen.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include "src/core/State.h"
#include <stdio.h>

static void render(lv_obj_t* root) {
    AlertItem alerts[5];
    int alertCount = 0;
    int displayCount = 0;

    state::lock();
    alertCount = state::alertCount();
    displayCount = min(5, alertCount);
    for (int i = 0; i < displayCount; i++) {
        alerts[i] = state::alerts()[i];
    }
    state::unlock();

    // Alert count number (28px)
    char countBuf[8];
    snprintf(countBuf, sizeof(countBuf), "%d", alertCount);

    bool isAllClear = (alertCount == 1 && alerts[0].severity == AlertSeverity::INFO);
    ui::label(root, countBuf, 10, 56, 45, 34, &lv_font_montserrat_28,
              isAllClear ? CLR_GREEN : CLR_YELLOW);

    ui::label(root, "ACTIVE ALERTS", 60, 68, 170, 16, &lv_font_montserrat_12, CLR_MUTED);

    // List of up to 5 alerts
    int y = 100;

    for (int i = 0; i < displayCount; i++) {
        lv_obj_t* row = ui::panel(root, 10, y, layout::CARD_W_FULL, 36, CLR_PANEL, 14);

        lv_color_t clr = ui::alertColor(alerts[i].severity);
        ui::circle(row, 9, 13, 7, clr);

        ui::label(row, alerts[i].title, 24, 4, 184, 15, &lv_font_montserrat_14, CLR_TEXT);
        ui::label(row, alerts[i].detail, 24, 18, 184, 13, &lv_font_montserrat_12, CLR_DIM);

        y += 40;
    }
}

static const bool s_reg = screen::registerScreen(
    SCREEN_ALERTS,
    { "ALERT CENTER", CLR_ALERTS_TOP, render }
);
