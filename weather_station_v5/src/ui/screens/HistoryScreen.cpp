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
        ui::metricCard(root, 10, 100, layout::CARD_W_FULL, 85, "LOCAL LOGS",
                       sdReady ? "Logging Active" : "SD Inactive",
                       sdReady ? "Collecting telemetry points" : "Insert FAT32 SD to record",
                       CLR_HISTORY_TOP);
        return;
    }

    ui::label(root, "Temperature", 10, 57, 100, 15, &lv_font_montserrat_12, CLR_MUTED);
    ui::chart(root, 10, 75, 220, 84, temps, count, CLR_CYAN);

    ui::label(root, "Pressure", 10, 171, 100, 15, &lv_font_montserrat_12, CLR_MUTED);
    ui::chart(root, 10, 189, 220, 93, press, count, CLR_PURPLE);
}

static const bool s_reg = screen::registerScreen(
    SCREEN_HISTORY,
    { "LOCAL HISTORY", CLR_HISTORY_TOP, render }
);
