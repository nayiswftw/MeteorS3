#pragma once

/*
 * Weather Station V4 — Visual Theme
 *
 * All color definitions and layout constants live here.
 * Screen code references these tokens — never raw hex or pixel values.
 */

#include <lvgl.h>

// ================================================================
//  Color Palette
// ================================================================

// Background gradient
#define CLR_BG_TOP       lv_color_hex(0x0A2431)
#define CLR_BG_BOTTOM    lv_color_hex(0x04090F)

// Panel surfaces (Glassmorphic dark surfaces)
#define CLR_PANEL        lv_color_hex(0x0F2230)
#define CLR_PANEL_ALT    lv_color_hex(0x163143)
#define CLR_PANEL_BORDER lv_color_hex(0x22485E)
#define CLR_PANEL_GLOW   lv_color_hex(0x1A3D54)

// Typography
#define CLR_TEXT         lv_color_hex(0xF8FAFC)
#define CLR_MUTED        lv_color_hex(0x94A8B8)
#define CLR_DIM          lv_color_hex(0x566B78)

// Accent colors
#define CLR_CYAN         lv_color_hex(0x68DAEA)
#define CLR_BLUE         lv_color_hex(0x78AFFF)
#define CLR_GREEN        lv_color_hex(0x75DBA0)
#define CLR_YELLOW       lv_color_hex(0xFFD57A)
#define CLR_ORANGE       lv_color_hex(0xFFAA67)
#define CLR_RED          lv_color_hex(0xFF7F83)
#define CLR_PURPLE       lv_color_hex(0xB9A3FF)
#define CLR_WHITE        lv_color_hex(0xFFFFFF)

// Cloud art (procedural icon fill colors)
#define CLR_CLOUD_LIGHT  lv_color_hex(0xEAF3F7)
#define CLR_CLOUD_DARK   lv_color_hex(0xD0E2EB)

// Screen-specific gradient tops
#define CLR_HOME_DAY     lv_color_hex(0x0C2E40)
#define CLR_HOME_NIGHT   lv_color_hex(0x13172E)
#define CLR_HOURLY_TOP   lv_color_hex(0x0D2B3A)
#define CLR_WEEK_TOP     lv_color_hex(0x112735)
#define CLR_RAIN_TOP     lv_color_hex(0x0A2238)
#define CLR_WIND_TOP     lv_color_hex(0x0B2B2F)
#define CLR_ATMOS_TOP    lv_color_hex(0x142330)
#define CLR_AIR_TOP      lv_color_hex(0x102820)
#define CLR_UV_TOP       lv_color_hex(0x352818)
#define CLR_SUN_DAY      lv_color_hex(0x3B2D18)
#define CLR_SUN_NIGHT    lv_color_hex(0x191832)
#define CLR_HISTORY_TOP  lv_color_hex(0x122230)
#define CLR_ALERTS_TOP   lv_color_hex(0x2E1D24)
#define CLR_SYSTEM_TOP   lv_color_hex(0x11202C)


// ================================================================
//  Layout Constants (pixels)
// ================================================================

namespace layout {

// Screen dimensions
constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 320;

// Global margins
constexpr int MARGIN = 10;

// Header bar
constexpr int HEADER_Y  = 7;
constexpr int HEADER_H  = 20;

// Subtitle (below header)
constexpr int SUBTITLE_Y = 33;
constexpr int SUBTITLE_H = 16;

// Content area starts below subtitle
constexpr int CONTENT_Y = 57;

// Navigation dots
constexpr int NAV_Y           = 305;
constexpr int NAV_DOT_SPACING = 13;
constexpr int NAV_DOT_ACTIVE  = 6;
constexpr int NAV_DOT_IDLE    = 4;

// Card system
constexpr int CARD_GAP    = 10;
constexpr int CARD_W_HALF = 105;   // two cards side-by-side
constexpr int CARD_W_FULL = 220;   // single full-width card
constexpr int CARD_RADIUS = 16;

// Usable content width
constexpr int CONTENT_W = SCREEN_W - 2 * MARGIN;  // 220

}  // namespace layout
