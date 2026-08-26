/*
 * LVGL Configuration — Weather Station V5
 *
 * Targets: LVGL 9.x, ESP32-S3 Hardware Acceleration, ST7789 (240x320, 16-bit RGB565)
 *
 * Only settings that differ from LVGL 9 defaults are listed here.
 * Everything else falls through to lv_conf_internal.h defaults.
 */

#if 1  /* Set to 1 to enable this config */
#ifndef LV_CONF_H
#define LV_CONF_H

/* ====================
 *   COLOR SETTINGS
 * ==================== */

/* ST7789 uses 16-bit RGB565. */
#define LV_COLOR_DEPTH 16

/* ====================
 *   MEMORY SETTINGS
 * ==================== */

/* Internal LVGL memory pool (heap_caps DMA buffers are used separately for
   display buffers, so this only covers widgets and styles). */
#define LV_MEM_SIZE (64U * 1024U)

/* ====================
 *   HARDWARE ACCELERATION & OS
 * ==================== */

/* FreeRTOS integration for thread safety and low latency */
#define LV_USE_OS LV_OS_FREERTOS

/* Software/Assembly draw acceleration */
#define LV_USE_DRAW_SW 1
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_CUSTOM
#define LV_DRAW_SW_COMPLEX 1

/* Draw cache optimization for fast rounded cards, circles, and anti-aliasing */
#define LV_DRAW_SW_SHADOW_CACHE_SIZE 16
#define LV_DRAW_SW_CIRCLE_CACHE_SIZE 16

/* ====================
 *   DISPLAY SETTINGS
 * ==================== */

#define LV_DPI_DEF 130

/* ====================
 *   LOGGING
 * ==================== */

/* Disable in production to save flash / CPU. */
#define LV_USE_LOG 0

/* ====================
 *   FONT USAGE
 * ==================== */

#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_18  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_28  1
#define LV_FONT_MONTSERRAT_32  1
#define LV_FONT_MONTSERRAT_48  1
#define LV_FONT_DEFAULT        &lv_font_montserrat_14

/* ====================
 *   WIDGETS
 * ==================== */

/* label, obj, arc, chart, line are all enabled by default
   in LVGL 9.  No explicit overrides needed. */

#endif /* LV_CONF_H */
#endif /* 1 */
