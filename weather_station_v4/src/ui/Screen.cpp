#include "src/ui/Screen.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include "src/hal/Display.h"
#include "src/hal/Battery.h"
#include "src/core/State.h"
#include "src/core/Format.h"
#include "src/Config.h"

namespace screen {

static ScreenId  s_current = SCREEN_HOME;
static ScreenDef s_registry[SCREEN_COUNT];

bool registerScreen(ScreenId id, const ScreenDef& def) {
    if (id >= 0 && id < SCREEN_COUNT) {
        s_registry[id] = def;
        return true;
    }
    return false;
}

void init() {
    show(SCREEN_HOME);
}

void show(ScreenId id) {
    if (id < 0 || id >= SCREEN_COUNT) id = SCREEN_HOME;
    s_current = id;

    lv_obj_t* root = hal::displayRoot();
    if (!root) return;

    lv_obj_clean(root);

    const ScreenDef& def = s_registry[s_current];

    // Background gradient: dynamically adjust for Home/Sun if needed
    lv_color_t topColor = def.gradientTop;
    const WeatherData& w = state::weather();
    if (s_current == SCREEN_HOME) {
        topColor = (w.valid && !w.isDay) ? CLR_HOME_NIGHT : CLR_HOME_DAY;
    } else if (s_current == SCREEN_SUN) {
        topColor = (w.valid && !w.isDay) ? CLR_SUN_NIGHT : CLR_SUN_DAY;
    }

    ui::gradientBackground(root, topColor, CLR_BG_BOTTOM);

    // Prepare header data
    char clockBuf[8];
    fmt::clock(clockBuf, sizeof(clockBuf));

    ui::HeaderInfo hdr;
    hdr.location       = config::LOCATION_NAME;
    hdr.clockText      = clockBuf;
    hdr.batteryPercent = hal::readBatteryPercent();
    hdr.isOnline       = state::isOnline();

    ui::header(root, hdr, def.title);

    // Call screen-specific content renderer
    if (def.render) {
        def.render(root);
    }

    // Navigation dots
    ui::navDots(root, (int)s_current, SCREEN_COUNT);
}

void refresh() {
    show(s_current);
}

void next() {
    int nextId = ((int)s_current + 1) % SCREEN_COUNT;
    show((ScreenId)nextId);
}

void home() {
    show(SCREEN_HOME);
}

ScreenId current() {
    return s_current;
}

}  // namespace screen
