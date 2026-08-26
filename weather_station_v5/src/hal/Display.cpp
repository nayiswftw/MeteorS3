#include "src/hal/Display.h"
#include "src/hal/Backlight.h"
#include "src/Config.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_heap_caps.h>

namespace hal {

static Arduino_DataBus* s_lcdBus = nullptr;
static Arduino_GFX*     s_gfx    = nullptr;
static lv_display_t*    s_disp   = nullptr;
static lv_obj_t*        s_root   = nullptr;

static uint8_t* s_buf1 = nullptr;
static uint8_t* s_buf2 = nullptr;

static void flushDisplay(lv_display_t* disp, const lv_area_t* area, uint8_t* pixelMap) {
    uint32_t width  = area->x2 - area->x1 + 1;
    uint32_t height = area->y2 - area->y1 + 1;

    s_gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)pixelMap, width, height);
    lv_display_flush_ready(disp);
}

static uint32_t lvglTick() {
    return millis();
}

void displayInit() {
    // Backlight initialization with hardware PWM
    backlightInit();

    // Initialize high-speed hardware SPI databus on ESP32-S3 FSPI peripheral
    s_lcdBus = new Arduino_ESP32SPI(
        config::LCD_DC,
        config::LCD_CS,
        config::LCD_SCLK,
        config::LCD_MOSI,
        config::LCD_MISO,
        FSPI,
        true
    );

    s_gfx = new Arduino_ST7789(
        s_lcdBus,
        config::LCD_RST,
        0,
        true,
        config::LCD_WIDTH,
        config::LCD_HEIGHT
    );

    // Initialize ST7789 at 80 MHz SPI bus speed with hardware DMA
    if (!s_gfx->begin(80000000UL)) {
        Serial.println("[display] GFX begin at 80MHz failed, retrying default clock...");
        if (!s_gfx->begin()) {
            Serial.println("[display] GFX begin failed");
        }
    }

    s_gfx->fillScreen(RGB565_BLACK);

    // Initialize LVGL 9
    lv_init();
    lv_tick_set_cb(lvglTick);

    s_disp = lv_display_create(config::LCD_WIDTH, config::LCD_HEIGHT);

    size_t bufSize = config::LCD_WIDTH * config::LVGL_BUFFER_LINES * sizeof(lv_color_t);
    s_buf1 = (uint8_t*)heap_caps_malloc(bufSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    s_buf2 = (uint8_t*)heap_caps_malloc(bufSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

    if (!s_buf1 || !s_buf2) {
        Serial.println("[display] LVGL DMA buffer allocation failed!");
        while (true) delay(1000);
    }

    lv_display_set_buffers(s_disp, s_buf1, s_buf2, bufSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(s_disp, flushDisplay);

    s_root = lv_screen_active();
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);

    Serial.println("[display] ready");
}

void displayService() {
    lv_timer_handler();
}

lv_obj_t* displayRoot() {
    return s_root;
}

}  // namespace hal
