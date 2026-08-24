#include "src/ui/Screen.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include "src/core/State.h"

static void render(lv_obj_t* root) {
    constexpr int COUNT = 24;
    float temps[COUNT];
    float press[COUNT];
    int count = 0;
    bool sdReady = false;

    state::lock();
    sdReady = state::isSdReady();
    const auto& hist = state::history();
    if (hist.size() >= 2) {
        count = min(COUNT, hist.size());
        int start = hist.size() - count;
        for (int i = 0; i < count; i++) {
            const HistoryPoint& pt = hist.at(start + i);
            temps[i] = pt.temperature;
            press[i] = pt.pressure;
        }
    }
    state::unlock();

    if (count < 2) {
        ui::label(root, sdReady ? "Collecting history..." : "SD unavailable",
                  10, 130, 220, 20, &lv_font_montserrat_14, CLR_MUTED, LV_TEXT_ALIGN_CENTER);
        return;
    }

    ui::label(root, "Temperature", 10, 57, 100, 16, &lv_font_montserrat_14, CLR_MUTED);
    ui::chart(root, 10, 76, 220, 83, temps, count, CLR_CYAN);

    ui::label(root, "Pressure", 10, 172, 100, 16, &lv_font_montserrat_14, CLR_MUTED);
    ui::chart(root, 10, 191, 220, 91, press, count, CLR_PURPLE);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_HISTORY,
    { "LOCAL HISTORY", CLR_HISTORY_TOP, render }
);
