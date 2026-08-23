#include "UiComponents.h"

#include "UiTheme.h"
#include "Hardware.h"
#include "DataService.h"
#include "AppState.h"
#include "Config.h"

namespace ui {

static lv_obj_t *root = nullptr;

static void stylePlain(lv_obj_t *object) {
  lv_obj_set_style_border_width(
    object,
    0,
    0
  );

  lv_obj_set_style_pad_all(
    object,
    0,
    0
  );

  lv_obj_clear_flag(
    object,
    LV_OBJ_FLAG_SCROLLABLE
  );
}

void begin() {
  root =
    displayRoot();
}

void clear() {
  if (root) {
    lv_obj_clean(root);
  }
}

lv_obj_t *label(
  lv_obj_t *parent,
  const String &textValue,
  int x, int y,
  int width, int height,
  const lv_font_t *font,
  lv_color_t color,
  lv_text_align_t align
) {
  lv_obj_t *object =
    lv_label_create(parent);

  lv_label_set_text(
    object,
    textValue.c_str()
  );

  lv_obj_set_pos(
    object,
    x,
    y
  );

  lv_obj_set_size(
    object,
    width,
    height
  );

  lv_obj_set_style_text_font(
    object,
    font,
    0
  );

  lv_obj_set_style_text_color(
    object,
    color,
    0
  );

  lv_obj_set_style_text_align(
    object,
    align,
    0
  );

  lv_label_set_long_mode(
    object,
    LV_LABEL_LONG_CLIP
  );

  return object;
}

lv_obj_t *panel(
  lv_obj_t *parent,
  int x, int y,
  int width, int height,
  lv_color_t color,
  int radius
) {
  lv_obj_t *object =
    lv_obj_create(parent);

  lv_obj_set_pos(
    object,
    x,
    y
  );

  lv_obj_set_size(
    object,
    width,
    height
  );

  stylePlain(object);

  lv_obj_set_style_bg_color(
    object,
    color,
    0
  );

  lv_obj_set_style_bg_opa(
    object,
    LV_OPA_COVER,
    0
  );

  lv_obj_set_style_radius(
    object,
    radius,
    0
  );

  return object;
}

void gradientBackground(
  lv_color_t top,
  lv_color_t bottom
) {
  lv_obj_set_style_bg_color(
    root,
    top,
    0
  );

  lv_obj_set_style_bg_grad_color(
    root,
    bottom,
    0
  );

  lv_obj_set_style_bg_grad_dir(
    root,
    LV_GRAD_DIR_VER,
    0
  );

  lv_obj_set_style_bg_opa(
    root,
    LV_OPA_COVER,
    0
  );
}

static void circle(
  int x, int y,
  int diameter,
  lv_color_t color,
  lv_opa_t opacity = LV_OPA_COVER
) {
  lv_obj_t *object =
    panel(
      root,
      x,
      y,
      diameter,
      diameter,
      color,
      LV_RADIUS_CIRCLE
    );

  lv_obj_set_style_bg_opa(
    object,
    opacity,
    0
  );
}

static void line(
  int x1, int y1,
  int x2, int y2,
  lv_color_t color,
  int width = 2
) {
  static lv_point_precise_t segments[48][2];
  static int segmentIndex = 0;

  lv_point_precise_t *points =
    segments[
      segmentIndex++ % 48
    ];

  points[0].x = x1;
  points[0].y = y1;

  points[1].x = x2;
  points[1].y = y2;

  lv_obj_t *object =
    lv_line_create(root);

  lv_line_set_points(
    object,
    points,
    2
  );

  lv_obj_set_style_line_width(
    object,
    width,
    0
  );

  lv_obj_set_style_line_color(
    object,
    color,
    0
  );

  lv_obj_set_style_line_rounded(
    object,
    true,
    0
  );
}

void header(
  const String &title
) {
  label(
    root,
    LOCATION_NAME,
    10, 7,
    108, 20,
    &lv_font_montserrat_14,
    UI_TEXT
  );

  label(
    root,
    localClockText(),
    124, 7,
    54, 20,
    &lv_font_montserrat_14,
    UI_MUTED,
    LV_TEXT_ALIGN_RIGHT
  );

  circle(
    188,
    12,
    7,
    wifiConnected
      ? UI_GREEN
      : UI_RED
  );

  int battery =
    readBatteryPercent();

  label(
    root,
    String(battery) + "%",
    199, 7,
    32, 20,
    &lv_font_montserrat_14,
    battery <= 20
      ? UI_RED
      : UI_MUTED,
    LV_TEXT_ALIGN_RIGHT
  );

  if (title.length()) {
    label(
      root,
      title,
      10, 33,
      220, 16,
      &lv_font_montserrat_14,
      UI_DIM
    );
  }
}

void navDots(
  int current,
  int count
) {
  int spacing = 13;

  int totalWidth =
    (count - 1) *
    spacing;

  int startX =
    (LCD_WIDTH - totalWidth) / 2;

  for (int i = 0;
       i < count;
       i++) {
    int diameter =
      i == current
        ? 6
        : 4;

    circle(
      startX + i * spacing,
      i == current
        ? 305
        : 306,
      diameter,
      i == current
        ? UI_CYAN
        : UI_DIM
    );
  }
}

static void sunArt(
  int centerX,
  int centerY,
  int scale
) {
  int radius =
    11 * scale;

  circle(
    centerX - radius,
    centerY - radius,
    radius * 2,
    UI_YELLOW
  );

  for (int i = 0; i < 8; i++) {
    float angle =
      i * PI / 4.0f;

    int x1 =
      centerX +
      cos(angle) *
      (radius + 4);

    int y1 =
      centerY +
      sin(angle) *
      (radius + 4);

    int x2 =
      centerX +
      cos(angle) *
      (radius + 11);

    int y2 =
      centerY +
      sin(angle) *
      (radius + 11);

    line(
      x1, y1,
      x2, y2,
      UI_YELLOW,
      2
    );
  }
}

static void cloudArt(
  int x,
  int y,
  int scale,
  lv_color_t color
) {
  circle(
    x + 7 * scale,
    y + 10 * scale,
    18 * scale,
    color
  );

  circle(
    x + 20 * scale,
    y + 3 * scale,
    24 * scale,
    color
  );

  circle(
    x + 36 * scale,
    y + 10 * scale,
    18 * scale,
    color
  );

  panel(
    root,
    x + 4 * scale,
    y + 14 * scale,
    42 * scale,
    13 * scale,
    color,
    6 * scale
  );
}

void weatherArt(
  int code,
  bool isDay,
  int x,
  int y,
  int scale
) {
  if (code == 0) {
    sunArt(
      x + 25 * scale,
      y + 25 * scale,
      scale
    );

    return;
  }

  if (code == 1 ||
      code == 2) {
    sunArt(
      x + 15 * scale,
      y + 12 * scale,
      scale
    );

    cloudArt(
      x + 8 * scale,
      y + 18 * scale,
      scale,
      lv_color_hex(0xE6F0F4)
    );

    return;
  }

  if (code <= 3 ||
      code == 45 ||
      code == 48) {
    cloudArt(
      x,
      y + 10 * scale,
      scale,
      lv_color_hex(0xE6F0F4)
    );

    return;
  }

  if ((code >= 51 &&
       code <= 67) ||
      (code >= 80 &&
       code <= 82)) {
    cloudArt(
      x,
      y,
      scale,
      lv_color_hex(0xD9E7ED)
    );

    for (int i = 0; i < 3; i++) {
      int dx =
        x +
        (12 + i * 12) *
        scale;

      line(
        dx,
        y + 32 * scale,
        dx - 3 * scale,
        y + 42 * scale,
        UI_CYAN,
        2
      );
    }

    return;
  }

  cloudArt(
    x,
    y + 10 * scale,
    scale,
    lv_color_hex(0xD9E7ED)
  );
}

void metricCard(
  int x, int y,
  int width, int height,
  const String &title,
  const String &value,
  const String &caption,
  lv_color_t accent
) {
  lv_obj_t *card =
    panel(
      root,
      x, y,
      width, height,
      UI_PANEL,
      16
    );

  lv_obj_t *bar =
    panel(
      card,
      0, 0,
      3, height,
      accent,
      2
    );

  label(
    card,
    title,
    10, 8,
    width - 18, 15,
    &lv_font_montserrat_14,
    UI_MUTED
  );

  label(
    card,
    value,
    10, 28,
    width - 18, 22,
    &lv_font_montserrat_14,
    UI_TEXT
  );

  label(
    card,
    caption,
    10, height - 20,
    width - 18, 15,
    &lv_font_montserrat_14,
    UI_DIM
  );
}

void gauge(
  int x, int y,
  int diameter,
  int value,
  int minimum,
  int maximum,
  const String &center,
  const String &caption,
  lv_color_t color
) {
  lv_obj_t *arc =
    lv_arc_create(root);

  lv_obj_set_pos(
    arc,
    x,
    y
  );

  lv_obj_set_size(
    arc,
    diameter,
    diameter
  );

  lv_arc_set_range(
    arc,
    minimum,
    maximum
  );

  lv_arc_set_value(
    arc,
    constrain(
      value,
      minimum,
      maximum
    )
  );

  lv_arc_set_rotation(
    arc,
    135
  );

  lv_arc_set_bg_angles(
    arc,
    0,
    270
  );

  lv_obj_remove_style(
    arc,
    NULL,
    LV_PART_KNOB
  );

  lv_obj_set_style_arc_width(
    arc,
    7,
    LV_PART_MAIN
  );

  lv_obj_set_style_arc_color(
    arc,
    UI_PANEL_ALT,
    LV_PART_MAIN
  );

  lv_obj_set_style_arc_width(
    arc,
    7,
    LV_PART_INDICATOR
  );

  lv_obj_set_style_arc_color(
    arc,
    color,
    LV_PART_INDICATOR
  );

  label(
    root,
    center,
    x + 7,
    y + diameter / 2 - 13,
    diameter - 14,
    20,
    &lv_font_montserrat_14,
    UI_TEXT,
    LV_TEXT_ALIGN_CENTER
  );

  label(
    root,
    caption,
    x + 4,
    y + diameter - 19,
    diameter - 8,
    16,
    &lv_font_montserrat_14,
    UI_MUTED,
    LV_TEXT_ALIGN_CENTER
  );
}

lv_obj_t *chart(
  int x, int y,
  int width, int height,
  const float *values,
  int count,
  lv_color_t color
) {
  if (count < 2) return nullptr;

  float minimum = 999999;
  float maximum = -999999;

  for (int i = 0; i < count; i++) {
    if (isnan(values[i])) continue;

    minimum =
      min(
        minimum,
        values[i]
      );

    maximum =
      max(
        maximum,
        values[i]
      );
  }

  if (minimum > 900000) {
    return nullptr;
  }

  if (maximum - minimum < 1.0f) {
    minimum -= 0.5f;
    maximum += 0.5f;
  }

  lv_obj_t *object =
    lv_chart_create(root);

  lv_obj_set_pos(
    object,
    x,
    y
  );

  lv_obj_set_size(
    object,
    width,
    height
  );

  lv_chart_set_type(
    object,
    LV_CHART_TYPE_LINE
  );

  lv_chart_set_point_count(
    object,
    count
  );

  lv_chart_set_axis_range(
    object,
    LV_CHART_AXIS_PRIMARY_Y,
    (int32_t)floor(minimum * 10),
    (int32_t)ceil(maximum * 10)
  );

  lv_chart_set_div_line_count(
    object,
    3,
    0
  );

  lv_obj_set_style_bg_opa(
    object,
    LV_OPA_TRANSP,
    LV_PART_MAIN
  );

  lv_obj_set_style_border_width(
    object,
    0,
    LV_PART_MAIN
  );

  lv_obj_set_style_line_color(
    object,
    UI_PANEL_ALT,
    LV_PART_MAIN
  );

  lv_obj_set_style_line_opa(
    object,
    LV_OPA_30,
    LV_PART_MAIN
  );

  lv_obj_set_style_line_width(
    object,
    3,
    LV_PART_ITEMS
  );

  lv_chart_series_t *series =
    lv_chart_add_series(
      object,
      color,
      LV_CHART_AXIS_PRIMARY_Y
    );

  for (int i = 0;
       i < count;
       i++) {
    if (isnan(values[i])) {
      lv_chart_set_next_value(
        object,
        series,
        LV_CHART_POINT_NONE
      );
    } else {
      lv_chart_set_next_value(
        object,
        series,
        (int32_t)round(
          values[i] * 10
        )
      );
    }
  }

  return object;
}

void rangeBar(
  int x, int y,
  int width,
  float low,
  float high,
  float globalLow,
  float globalHigh,
  lv_color_t color
) {
  panel(
    root,
    x, y,
    width, 5,
    UI_PANEL_ALT,
    3
  );

  if (globalHigh <= globalLow) {
    globalHigh =
      globalLow +
      1;
  }

  int lowX =
    x +
    (int)(
      (low - globalLow) /
      (globalHigh - globalLow) *
      width
    );

  int highX =
    x +
    (int)(
      (high - globalLow) /
      (globalHigh - globalLow) *
      width
    );

  panel(
    root,
    lowX,
    y,
    max(
      5,
      highX - lowX
    ),
    5,
    color,
    3
  );
}

lv_color_t alertColor(
  int severity
) {
  switch (severity) {
    case 3:
      return UI_RED;

    case 2:
      return UI_ORANGE;

    case 1:
      return UI_YELLOW;

    default:
      return UI_GREEN;
  }
}

}
