WEATHER STATION V4 (PlatformIO Edition)
========================================

Hardware: Waveshare ESP32-S3-LCD-2 (ESP32-S3R8, 16MB Flash, 8MB PSRAM, ST7789T3 240x320 IPS)

BUILD & FLASH
-------------
1. Open this directory in VS Code with PlatformIO extension installed (or use PlatformIO CLI).
2. Edit `include/Config.h` with your WiFi credentials, coordinates, and units.
3. Build & Upload:
   - Command palette / UI: "PlatformIO: Upload"
   - CLI: `pio run -t upload`
   - Monitor: `pio device monitor -b 115200`

DIRECTORY STRUCTURE
-------------------
include/
  Config.h               User configuration (WiFi, GPS, units, pin definitions)
  lv_conf.h              LVGL 9.x lightweight configuration
  core/
    Types.h              Plain data structures, limits, ring buffer template
    State.h              Centralized state accessor API (const-ref reads, guarded writes)
    Weather.h            Weather intelligence & alert derivation engine
    Format.h             Zero-allocation string formatting utilities
  hal/
    Display.h            ST7789 display initialization & LVGL 9 display driver
    Input.h              Button debounce & press classification (short / long)
    Battery.h            ADC voltage measurement & SoC percentage mapping
    Imu.h                QMI8658 6-axis IMU driver (I2C)
  services/
    Network.h            WiFi FSM with non-blocking exponential backoff
    TimeSync.h           NTP time synchronization
    Api.h                Open-Meteo REST client for Weather & Air Quality
    Storage.h            NVS offline cache & SD card CSV logging
  ui/
    Theme.h              Color design tokens and layout geometry constants
    Icons.h              Procedural weather icons (sun, clouds, rain)
    Widgets.h            Reusable LVGL widgets (panels, labels, cards, gauges, charts)
    Screen.h             Screen manager & template system

src/
  main.cpp               Main application lifecycle orchestration
  core/                  State, Format, Weather implementations
  hal/                   Display, Input, Battery, Imu implementations
  services/              Network, TimeSync, Api, Storage implementations
  ui/                    Widgets, Icons, Screen implementation
  ui/screens/            Individual screen components (12 files)
    HomeScreen.cpp
    HourlyScreen.cpp
    WeekScreen.cpp
    RainScreen.cpp
    WindScreen.cpp
    AtmosphereScreen.cpp
    AirScreen.cpp
    UvScreen.cpp
    SunScreen.cpp
    HistoryScreen.cpp
    AlertsScreen.cpp
    SystemScreen.cpp

KEY IMPROVEMENTS OVER LEGACY CODE
---------------------------------
1. Zero Naked Globals: State is managed through a clean read-only accessor API.
2. Zero Heap Allocations in Data Flow: Stack buffers and fixed-size char arrays.
3. Decoupled Architecture: HAL (hardware), Services (I/O), Core (data), UI (presentation).
4. O(1) History Ring Buffer: Eliminates array shifts on every history log point.
5. Non-Blocking WiFi FSM: Graceful exponential backoff without blocking the loop.
6. Unified Screen Registry: Each screen is a clean modular file that plugs into the framework.
