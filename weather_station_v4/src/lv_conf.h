/*
 * LVGL Configuration — Weather Station V4
 *
 * Targets: LVGL 9.x, ESP32-S3, ST7789T3 (240x320, 16-bit RGB565)
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

/* Internal LVGL memory pool (heap_caps is used separately for
   display buffers, so this only covers widgets and styles). */
#define LV_MEM_SIZE (48U * 1024U)

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

#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_32  1
#define LV_FONT_DEFAULT        &lv_font_montserrat_14

/* ====================
 *   WIDGETS
 * ==================== */

/* label, obj, arc, chart, line are all enabled by default
   in LVGL 9.  No explicit overrides needed. */

#endif /* LV_CONF_H */
#endif /* 1 */
