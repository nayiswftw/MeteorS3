#include "src/ui/Widgets.h"
#include <math.h>
#include <stdio.h>

namespace ui {

static void stylePlain(lv_obj_t* object) {
    lv_obj_set_style_border_width(object, 0, 0);
    lv_obj_set_style_pad_all(object, 0, 0);
    lv_obj_clear_flag(object, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* label(
    lv_obj_t* parent,
    const char* text,
    int x, int y,
    int width, int height,
    const lv_font_t* font,
    lv_color_t color,
    lv_text_align_t align
) {
    lv_obj_t* obj = lv_label_create(parent);
    lv_label_set_text(obj, text ? text : "");
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, width, height);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, color, 0);
    lv_obj_set_style_text_align(obj, align, 0);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
    return obj;
}

lv_obj_t* panel(
    lv_obj_t* parent,
    int x, int y,
    int width, int height,
    lv_color_t color,
    int radius
) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, width, height);
    stylePlain(obj);
    lv_obj_set_style_bg_color(obj, color, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    return obj;
}

void circle(
    lv_obj_t* parent,
    int x, int y,
    int diameter,
    lv_color_t color,
    lv_opa_t opacity
) {
    lv_obj_t* obj = panel(parent, x, y, diameter, diameter, color, LV_RADIUS_CIRCLE);
    lv_obj_set_style_bg_opa(obj, opacity, 0);
}

void line(
    lv_obj_t* parent,
    int x1, int y1,
    int x2, int y2,
    lv_color_t color,
    int width
) {
    constexpr int POOL_SIZE = 64;
    static lv_point_precise_t segments[POOL_SIZE][2];
    static int segmentIdx = 0;

    lv_point_precise_t* pts = segments[segmentIdx++ % POOL_SIZE];
    pts[0].x = x1;
    pts[0].y = y1;
    pts[1].x = x2;
    pts[1].y = y2;

    lv_obj_t* obj = lv_line_create(parent);
    lv_line_set_points(obj, pts, 2);
    lv_obj_set_style_line_width(obj, width, 0);
    lv_obj_set_style_line_color(obj, color, 0);
    lv_obj_set_style_line_rounded(obj, true, 0);
}

void gradientBackground(
    lv_obj_t* root,
    lv_color_t top,
    lv_color_t bottom
) {
    lv_obj_set_style_bg_color(root, top, 0);
    lv_obj_set_style_bg_grad_color(root, bottom, 0);
    lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
}

lv_obj_t* glassCard(
    lv_obj_t* parent,
    int x, int y,
    int width, int height,
    lv_color_t accent,
    int radius
) {
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, width, height);
    stylePlain(obj);
    lv_obj_set_style_bg_color(obj, CLR_PANEL, 0);
    lv_obj_set_style_bg_opa(obj, (lv_opa_t)210, 0);
    lv_obj_set_style_border_color(obj, CLR_PANEL_BORDER, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, radius, 0);
    return obj;
}

void header(
    lv_obj_t* root,
    const HeaderInfo& info,
    const char* subtitle
) {
    // Location name (14px clean)
    label(root, info.location ? info.location : "",
          layout::MARGIN, layout::HEADER_Y,
          108, layout::HEADER_H,
          &lv_font_montserrat_14, CLR_TEXT);

    // Clock (14px)
    label(root, info.clockText ? info.clockText : "--:--",
          124, layout::HEADER_Y,
          54, layout::HEADER_H,
          &lv_font_montserrat_14, CLR_MUTED, LV_TEXT_ALIGN_RIGHT);

    // Online/Offline status dot with subtle ring
    lv_color_t dotColor = info.isOnline ? CLR_GREEN : CLR_RED;
    circle(root, 186, 10, 10, dotColor, LV_OPA_30);
    circle(root, 188, 12, 6, dotColor, LV_OPA_COVER);

    // Battery % / USB Power indicator
    char batBuf[8];
    lv_color_t batColor = CLR_MUTED;
    if (info.batteryPercent < 0) {
        snprintf(batBuf, sizeof(batBuf), "USB");
        batColor = CLR_GREEN;
    } else {
        snprintf(batBuf, sizeof(batBuf), "%d%%", info.batteryPercent);
        batColor = (info.batteryPercent <= 20) ? CLR_RED : CLR_MUTED;
    }
    label(root, batBuf,
          199, layout::HEADER_Y + 1,
          32, layout::HEADER_H,
          &lv_font_montserrat_12,
          batColor,
          LV_TEXT_ALIGN_RIGHT);

    // Optional Subtitle (12px tracking)
    if (subtitle && subtitle[0] != '\0') {
        label(root, subtitle,
              layout::MARGIN, layout::SUBTITLE_Y,
              layout::CONTENT_W, layout::SUBTITLE_H,
              &lv_font_montserrat_12, CLR_CYAN);
    }
}

void navDots(
    lv_obj_t* root,
    int current,
    int count
) {
    if (count <= 1) return;

    int totalWidth = (count - 1) * layout::NAV_DOT_SPACING;
    int startX     = (layout::SCREEN_W - totalWidth) / 2;

    for (int i = 0; i < count; i++) {
        bool active  = (i == current);
        int diameter = active ? layout::NAV_DOT_ACTIVE : layout::NAV_DOT_IDLE;
        int y        = active ? layout::NAV_Y : (layout::NAV_Y + 1);

        if (active) {
            circle(root, startX + i * layout::NAV_DOT_SPACING - 2, y - 2, diameter + 4, CLR_CYAN, LV_OPA_30);
        }
        circle(root, startX + i * layout::NAV_DOT_SPACING, y, diameter,
               active ? CLR_CYAN : CLR_DIM);
    }
}

void metricCard(
    lv_obj_t* root,
    int x, int y,
    int width, int height,
    const char* title,
    const char* value,
    const char* caption,
    lv_color_t accent
) {
    lv_obj_t* card = glassCard(root, x, y, width, height, accent, layout::CARD_RADIUS);

    bool isCompact = (height < 56);

    // Subtitle label (12px uppercase)
    label(card, title ? title : "", 10, isCompact ? 4 : 6, width - 16, 13,
          &lv_font_montserrat_12, CLR_MUTED);

    // Value (16px bold for regular cards, 18px for large full cards)
    const lv_font_t* valFont = (width >= 180 && !isCompact) ? &lv_font_montserrat_18 : &lv_font_montserrat_16;
    label(card, value ? value : "--", 10, isCompact ? 18 : 22, width - 16, 18,
          valFont, CLR_TEXT);

    // Caption helper (12px dim)
    if (caption && caption[0] != '\0') {
        label(card, caption, 10, height - (isCompact ? 14 : 17), width - 16, 13,
              &lv_font_montserrat_12, CLR_DIM);
    }
}

void alertBanner(
    lv_obj_t* root,
    const char* title,
    const char* detail,
    lv_color_t color
) {
    if (!title || title[0] == '\0') return;

    lv_obj_t* card = glassCard(root, 10, 52, 220, 38, color, 12);
    circle(card, 8, 14, 8, color);
    label(card, title, 22, 4, 190, 15, &lv_font_montserrat_14, CLR_TEXT);
    label(card, detail ? detail : "", 22, 19, 190, 14, &lv_font_montserrat_12, CLR_MUTED);
}


void gauge(
    lv_obj_t* root,
    int x, int y,
    int diameter,
    int value,
    int minimum,
    int maximum,
    const char* centerText,
    const char* caption,
    lv_color_t color
) {
    if (maximum <= minimum) maximum = minimum + 1;

    lv_obj_t* arc = lv_arc_create(root);
    lv_obj_set_pos(arc, x, y);
    lv_obj_set_size(arc, diameter, diameter);
    lv_arc_set_range(arc, minimum, maximum);
    lv_arc_set_value(arc, constrain(value, minimum, maximum));
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);

    // Hide knob transparently in LVGL
    lv_obj_set_style_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, 0, LV_PART_KNOB);

    lv_obj_set_style_arc_width(arc, 6, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, CLR_PANEL_ALT, LV_PART_MAIN);

    lv_obj_set_style_arc_width(arc, 6, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);

    // Center Value (18px bold — perfectly proportioned inside gauge)
    label(root, centerText ? centerText : "",
          x + 4, y + diameter / 2 - 12, diameter - 8, 20,
          &lv_font_montserrat_18, CLR_TEXT, LV_TEXT_ALIGN_CENTER);

    // Bottom Caption (12px)
    label(root, caption ? caption : "",
          x + 4, y + diameter - 18, diameter - 8, 14,
          &lv_font_montserrat_12, CLR_MUTED, LV_TEXT_ALIGN_CENTER);
}

lv_obj_t* chart(
    lv_obj_t* root,
    int x, int y,
    int width, int height,
    const float* values,
    int count,
    lv_color_t color
) {
    if (!values || count < 2) return nullptr;

    float minVal = 999999.0f;
    float maxVal = -999999.0f;

    for (int i = 0; i < count; i++) {
        if (isnan(values[i])) continue;
        if (values[i] < minVal) minVal = values[i];
        if (values[i] > maxVal) maxVal = values[i];
    }

    if (minVal > 900000.0f) return nullptr;

    if (maxVal - minVal < 1.0f) {
        minVal -= 0.5f;
        maxVal += 0.5f;
    }

    int32_t rangeMin = (int32_t)floor(minVal * 10);
    int32_t rangeMax = (int32_t)ceil(maxVal * 10);
    if (rangeMax <= rangeMin) rangeMax = rangeMin + 10;

    lv_obj_t* obj = lv_chart_create(root);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, width, height);
    lv_chart_set_type(obj, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(obj, count);
    lv_chart_set_axis_range(obj, LV_CHART_AXIS_PRIMARY_Y, rangeMin, rangeMax);
    lv_chart_set_div_line_count(obj, 3, 2);

    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_line_color(obj, CLR_PANEL_ALT, LV_PART_MAIN);
    lv_obj_set_style_line_opa(obj, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_line_width(obj, 3, LV_PART_ITEMS);

    lv_chart_series_t* series = lv_chart_add_series(obj, color, LV_CHART_AXIS_PRIMARY_Y);

    for (int i = 0; i < count; i++) {
        if (isnan(values[i])) {
            lv_chart_set_next_value(obj, series, LV_CHART_POINT_NONE);
        } else {
            lv_chart_set_next_value(obj, series, (int32_t)round(values[i] * 10.0f));
        }
    }

    return obj;
}

void rangeBar(
    lv_obj_t* root,
    int x, int y,
    int width,
    float low,
    float high,
    float globalLow,
    float globalHigh,
    lv_color_t color
) {
    panel(root, x, y, width, 5, CLR_PANEL_ALT, 3);

    if (globalHigh <= globalLow) {
        globalHigh = globalLow + 1.0f;
    }

    int lowX  = x + (int)((low - globalLow) / (globalHigh - globalLow) * width);
    int highX = x + (int)((high - globalLow) / (globalHigh - globalLow) * width);

    panel(root, lowX, y, max(5, highX - lowX), 5, color, 3);
}

lv_color_t alertColor(AlertSeverity severity) {
    switch (severity) {
        case AlertSeverity::DANGER:  return CLR_RED;
        case AlertSeverity::WARNING: return CLR_ORANGE;
        case AlertSeverity::CAUTION: return CLR_YELLOW;
        default:                     return CLR_GREEN;
    }
}

lv_color_t aqiColor(int aqi) {
    if (aqi < 0)    return CLR_MUTED;
    if (aqi <= 50)  return CLR_GREEN;
    if (aqi <= 100) return CLR_YELLOW;
    if (aqi <= 150) return CLR_ORANGE;
    return CLR_RED;
}

}  // namespace ui
