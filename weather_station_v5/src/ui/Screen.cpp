#include "src/ui/Screen.h"
#include "src/ui/Widgets.h"
#include "src/ui/Theme.h"
#include "src/ui/Animations.h"
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
    ui::animInit();
    show(SCREEN_HOME);
}

void show(ScreenId id) {
    if (id < 0 || id >= SCREEN_COUNT) id = SCREEN_HOME;
    s_current = id;

    lv_obj_t* root = hal::displayRoot();
    if (!root) return;

    ui::animStop();
    lv_obj_clean(root);

    const ScreenDef& def = s_registry[s_current];

    // Background gradient: dynamically adjust for Home/Sun if needed
    lv_color_t topColor = def.gradientTop;
    state::lock();
    const WeatherData& w = state::weather();
    const char* customAl = state::customAlert();
    const char* locName  = state::config().locationName;
    bool isDay = w.isDay;
    bool isWeatherValid = w.valid;
    int weatherCode = w.weatherCode;
    state::unlock();

    if (s_current == SCREEN_HOME) {
        topColor = (isWeatherValid && !isDay) ? CLR_HOME_NIGHT : CLR_HOME_DAY;
    } else if (s_current == SCREEN_SUN) {
        topColor = (isWeatherValid && !isDay) ? CLR_SUN_NIGHT : CLR_SUN_DAY;
    }

    ui::gradientBackground(root, topColor, CLR_BG_BOTTOM);

    // Prepare header data
    char clockBuf[8];
    fmt::clock(clockBuf, sizeof(clockBuf));

    ui::HeaderInfo hdr;
    hdr.location       = (locName && locName[0] != '\0') ? locName : config::LOCATION_NAME;
    hdr.clockText      = clockBuf;
    hdr.batteryPercent = hal::readBatteryPercent();
    hdr.isOnline       = state::isOnline();

    ui::header(root, hdr, def.title);

    // Call screen-specific content renderer
    if (def.render) {
        def.render(root);
    }

    // Attach dynamic animated weather particles on Home screen
    if (s_current == SCREEN_HOME && isWeatherValid) {
        ui::animAttachWeather(root, weatherCode, isDay);
    }

    // Overlay custom alert banner if present
    if (customAl && customAl[0] != '\0') {
        ui::alertBanner(root, "NOTIFICATION", customAl, CLR_ORANGE);
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

void prev() {
    int prevId = ((int)s_current - 1 + SCREEN_COUNT) % SCREEN_COUNT;
    show((ScreenId)prevId);
}

void home() {
    show(SCREEN_HOME);
}

ScreenId current() {
    return s_current;
}

}  // namespace screen

