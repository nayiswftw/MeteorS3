#include "Hardware.h"
#include "Config.h"

#include <Arduino_GFX_Library.h>

static Arduino_DataBus *lcdBus =
  new Arduino_ESP32SPI(
    LCD_DC,
    LCD_CS,
    LCD_SCLK,
    LCD_MOSI,
    LCD_MISO
  );

static Arduino_GFX *gfx =
  new Arduino_ST7789(
    lcdBus,
    LCD_RST,
    0,
    true,
    LCD_WIDTH,
    LCD_HEIGHT
  );

static lv_display_t *display = nullptr;
static lv_obj_t *root = nullptr;

static uint8_t *buffer1 = nullptr;
static uint8_t *buffer2 = nullptr;

constexpr int BUFFER_LINES = 40;

static bool previousButton = HIGH;
static uint32_t buttonDownAt = 0;
static bool shortEvent = false;
static bool longEvent = false;
static bool longHandled = false;

static void flushDisplay(lv_display_t *disp,
                         const lv_area_t *area,
                         uint8_t *pixelMap) {
  uint32_t width = area->x2 - area->x1 + 1;
  uint32_t height = area->y2 - area->y1 + 1;

  gfx->draw16bitRGBBitmap(
    area->x1,
    area->y1,
    (uint16_t *)pixelMap,
    width,
    height
  );

  lv_display_flush_ready(disp);
}

static uint32_t lvglTick() {
  return millis();
}

void hardwareBegin() {
  pinMode(USER_BUTTON, INPUT_PULLUP);

  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, HIGH);

  analogReadResolution(12);
}

void displayBegin() {
  if (!gfx->begin()) {
    Serial.println("[display] gfx begin failed");
  }

  gfx->fillScreen(RGB565_BLACK);

  lv_init();
  lv_tick_set_cb(lvglTick);

  display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);

  size_t bufferSize =
    LCD_WIDTH * BUFFER_LINES * sizeof(lv_color_t);

  buffer1 = (uint8_t *)heap_caps_malloc(
    bufferSize,
    MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL
  );

  buffer2 = (uint8_t *)heap_caps_malloc(
    bufferSize,
    MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL
  );

  if (!buffer1 || !buffer2) {
    Serial.println("[display] LVGL buffers failed");
    while (true) delay(1000);
  }

  lv_display_set_buffers(
    display,
    buffer1,
    buffer2,
    bufferSize,
    LV_DISPLAY_RENDER_MODE_PARTIAL
  );

  lv_display_set_flush_cb(
    display,
    flushDisplay
  );

  root = lv_screen_active();

  lv_obj_clear_flag(
    root,
    LV_OBJ_FLAG_SCROLLABLE
  );

  Serial.println("[display] ready");
}

void displayService() {
  bool current = digitalRead(USER_BUTTON);

  if (previousButton == HIGH && current == LOW) {
    buttonDownAt = millis();
    longHandled = false;
  }

  if (current == LOW &&
      !longHandled &&
      millis() - buttonDownAt >= 900) {
    longHandled = true;
    longEvent = true;
  }

  if (previousButton == LOW && current == HIGH) {
    uint32_t duration = millis() - buttonDownAt;

    if (!longHandled && duration >= 30) {
      shortEvent = true;
    }
  }

  previousButton = current;

  lv_timer_handler();
}

bool buttonShortPressed() {
  if (!shortEvent) return false;

  shortEvent = false;
  return true;
}

bool buttonLongPressed() {
  if (!longEvent) return false;

  longEvent = false;
  return true;
}

float readBatteryVoltage() {
  uint32_t total = 0;

  for (int i = 0; i < 8; i++) {
    total += analogRead(BATTERY_ADC);
    delayMicroseconds(80);
  }

  float raw = total / 8.0f;

  // Same divider assumption as the known-working V1 code.
  return (raw * 3.3f / 4095.0f) * 2.0f;
}

int readBatteryPercent() {
  float v = readBatteryVoltage();

  if (v >= 4.20f) return 100;
  if (v >= 4.00f) return map((int)(v * 1000), 4000, 4200, 80, 100);
  if (v >= 3.80f) return map((int)(v * 1000), 3800, 4000, 45, 80);
  if (v >= 3.65f) return map((int)(v * 1000), 3650, 3800, 15, 45);
  if (v >= 3.40f) return map((int)(v * 1000), 3400, 3650, 0, 15);

  return 0;
}

lv_obj_t *displayRoot() {
  return root;
}
