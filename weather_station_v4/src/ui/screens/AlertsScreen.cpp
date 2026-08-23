#include "ui/Screen.h"
#include "ui/Widgets.h"
#include "ui/Theme.h"
#include "core/State.h"
#include <stdio.h>

static void render(lv_obj_t* root) {
    const AlertItem* alerts = state::alerts();
    int alertCount = state::alertCount();

    // Alert count number
    char countBuf[8];
    snprintf(countBuf, sizeof(countBuf), "%d", alertCount);

    bool isAllClear = (alertCount == 1 && alerts[0].severity == AlertSeverity::INFO);
    ui::label(root, countBuf, 10, 57, 55, 39, &lv_font_montserrat_32,
              isAllClear ? CLR_GREEN : CLR_YELLOW);

    ui::label(root, "ACTIVE", 68, 70, 70, 16, &lv_font_montserrat_14, CLR_MUTED);

    // List of up to 5 alerts
    int y = 110;
    int displayCount = min(5, alertCount);

    for (int i = 0; i < displayCount; i++) {
        lv_obj_t* row = ui::panel(root, 10, y, layout::CARD_W_FULL, 35, CLR_PANEL, 15);

        lv_color_t clr = ui::alertColor(alerts[i].severity);
        ui::circle(row, 9, 12, 8, clr);

        ui::label(row, alerts[i].title, 26, 5, 182, 15, &lv_font_montserrat_14, CLR_TEXT);
        ui::label(row, alerts[i].detail, 26, 19, 182, 13, &lv_font_montserrat_14, CLR_DIM);

        y += 42;
    }
}

static const bool s_reg = screen::registerScreen(
    SCREEN_ALERTS,
    { "ALERT CENTER", CLR_ALERTS_TOP, render }
);
