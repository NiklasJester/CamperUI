#include "ui_temps.h"
#include "ui_main.h"
#include "temp_history.h"
#include <stdio.h>
#include <math.h>

static lv_obj_t *root_page = nullptr;
static lv_obj_t *chart = nullptr;
static lv_chart_series_t *ser_outdoor = nullptr;
static lv_chart_series_t *ser_indoor = nullptr;
static lv_chart_series_t *ser_custom1 = nullptr;
static lv_chart_series_t *ser_custom2 = nullptr;

static lv_obj_t *btn_win_1h = nullptr;
static lv_obj_t *btn_win_6h = nullptr;
static lv_obj_t *btn_win_12h = nullptr;

static lv_obj_t *cards[4] = {};
static lv_obj_t *lbl_names[4] = {};
static lv_obj_t *lbl_vals[4] = {};
static lv_obj_t *lbl_minmax[4] = {};

static lv_obj_t *lbl_scale_top = nullptr;
static lv_obj_t *lbl_scale_bot = nullptr;
static lv_obj_t *lbl_axis_start = nullptr;
static lv_obj_t *lbl_axis_mid = nullptr;

static constexpr int CHART_POINTS = 60;

static const uint32_t TRACK_COLORS[4] = {
    0x38bdf8, // Aussen: Sky Blue
    0x34d399, // Innen: Emerald Green
    0xfbbf24, // Zusatz 1: Amber
    0xc084fc  // Zusatz 2: Purple
};

static void update_win_buttons_style(void) {
    int win = state.temps_history_window;
    auto style_btn = [](lv_obj_t *btn, bool active) {
        if (!btn) return;
        lv_obj_set_style_bg_color(btn, active ? lv_color_hex(UI_COLOR_PRIMARY) : ui_theme_track(), 0);
        lv_obj_set_style_text_color(btn, active ? lv_color_hex(0xffffff) : ui_theme_muted(), 0);
    };
    style_btn(btn_win_1h, win == 1);
    style_btn(btn_win_6h, win == 6);
    style_btn(btn_win_12h, win == 12);

    if (lbl_axis_start) {
        if (win == 1) lv_label_set_text(lbl_axis_start, "-1 Std.");
        else if (win == 6) lv_label_set_text(lbl_axis_start, "-6 Std.");
        else lv_label_set_text(lbl_axis_start, "-12 Std.");
    }
    if (lbl_axis_mid) {
        if (win == 1) lv_label_set_text(lbl_axis_mid, "-30 Min.");
        else if (win == 6) lv_label_set_text(lbl_axis_mid, "-3 Std.");
        else lv_label_set_text(lbl_axis_mid, "-6 Std.");
    }
}

static void win_btn_clicked(lv_event_t *e) {
    int win = (int)(intptr_t)lv_event_get_user_data(e);
    state.temps_history_window = win;
    state_save();
    update_win_buttons_style();
    ui_update_temps_tab();
}

void ui_build_temps(lv_obj_t *parent) {
    root_page = parent;
    ui_setup_tab_page(parent);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // 1. Header Row (y = 0, h = 34px): Title & Window Buttons
    // ==========================================
    lv_obj_t *lbl_title = lv_label_create(parent);
    lv_label_set_text(lbl_title, MDI_THERMOMETER " Temperaturen");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 0, 4);

    // Pill Button Container (Right aligned)
    lv_obj_t *pill_box = lv_obj_create(parent);
    lv_obj_remove_style_all(pill_box);
    lv_obj_set_size(pill_box, 190, 32);
    lv_obj_align(pill_box, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_flex_flow(pill_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pill_box, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(pill_box, 6, 0);

    auto create_pill = [&](const char *txt, int win) -> lv_obj_t* {
        lv_obj_t *b = lv_btn_create(pill_box);
        lv_obj_set_size(b, 56, 30);
        lv_obj_set_style_radius(b, 6, 0);
        lv_obj_set_style_border_width(b, 1, 0);
        lv_obj_set_style_border_color(b, ui_theme_border(), 0);
        lv_obj_add_event_cb(b, win_btn_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)win);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, txt);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
        lv_obj_center(l);
        return b;
    };

    btn_win_1h = create_pill("1h", 1);
    btn_win_6h = create_pill("6h", 6);
    btn_win_12h = create_pill("12h", 12);
    update_win_buttons_style();

    // ==========================================
    // 2. Metric Cards (y = 38, h = 98px): 2x2 Grid
    // ==========================================
    for (int i = 0; i < 4; ++i) {
        lv_coord_t cx = (i % 2 == 0) ? 0 : 224;
        lv_coord_t cy = (i < 2) ? 38 : 88;

        cards[i] = lv_obj_create(parent);
        lv_obj_set_pos(cards[i], cx, cy);
        lv_obj_set_size(cards[i], 216, 46);
        lv_obj_set_style_bg_color(cards[i], ui_theme_card(), 0);
        lv_obj_set_style_border_color(cards[i], ui_theme_border(), 0);
        lv_obj_set_style_border_width(cards[i], 1, 0);
        lv_obj_set_style_radius(cards[i], 10, 0);
        lv_obj_set_style_pad_all(cards[i], 4, 0);
        lv_obj_clear_flag(cards[i], LV_OBJ_FLAG_SCROLLABLE);

        // Colored accent bar
        lv_obj_t *accent = lv_obj_create(cards[i]);
        lv_obj_remove_style_all(accent);
        lv_obj_set_size(accent, 4, 28);
        lv_obj_set_pos(accent, 2, 5);
        lv_obj_set_style_radius(accent, 2, 0);
        lv_obj_set_style_bg_color(accent, lv_color_hex(TRACK_COLORS[i]), 0);
        lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);

        // Sensor name
        lbl_names[i] = lv_label_create(cards[i]);
        lv_obj_set_pos(lbl_names[i], 10, 2);
        lv_obj_set_width(lbl_names[i], 120);
        lv_label_set_long_mode(lbl_names[i], LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(lbl_names[i], &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(lbl_names[i], ui_theme_muted(), 0);

        // Min/Max sub-label
        lbl_minmax[i] = lv_label_create(cards[i]);
        lv_obj_set_pos(lbl_minmax[i], 10, 20);
        lv_obj_set_width(lbl_minmax[i], 120);
        lv_obj_set_style_text_font(lbl_minmax[i], &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(lbl_minmax[i], ui_theme_muted(), 0);
        lv_label_set_text(lbl_minmax[i], "--");

        // Current value (aligned right)
        lbl_vals[i] = lv_label_create(cards[i]);
        lv_obj_set_style_text_font(lbl_vals[i], &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(lbl_vals[i], ui_theme_text(), 0);
        lv_obj_align(lbl_vals[i], LV_ALIGN_RIGHT_MID, -6, 0);
        lv_label_set_text(lbl_vals[i], "--");
    }

    // Default card names
    lv_label_set_text(lbl_names[0], "Außen");
    lv_label_set_text(lbl_names[1], "Innen");
    lv_label_set_text(lbl_names[2], "Zusatz 1");
    lv_label_set_text(lbl_names[3], "Zusatz 2");

    // ==========================================
    // 3. Chart Card (y = 140, h = 216px, w = 440px)
    // ==========================================
    lv_obj_t *chart_card = lv_obj_create(parent);
    lv_obj_set_pos(chart_card, 0, 140);
    lv_obj_set_size(chart_card, 440, 216);
    lv_obj_set_style_bg_color(chart_card, ui_theme_card(), 0);
    lv_obj_set_style_border_color(chart_card, ui_theme_border(), 0);
    lv_obj_set_style_border_width(chart_card, 1, 0);
    lv_obj_set_style_radius(chart_card, 12, 0);
    lv_obj_set_style_pad_all(chart_card, 6, 0);
    lv_obj_clear_flag(chart_card, LV_OBJ_FLAG_SCROLLABLE);

    // Scale header labels
    lbl_scale_top = lv_label_create(chart_card);
    lv_obj_align(lbl_scale_top, LV_ALIGN_TOP_LEFT, 6, 2);
    lv_obj_set_style_text_font(lbl_scale_top, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_scale_top, ui_theme_muted(), 0);
    lv_label_set_text(lbl_scale_top, "↑ -- °C");

    lbl_scale_bot = lv_label_create(chart_card);
    lv_obj_align(lbl_scale_bot, LV_ALIGN_TOP_RIGHT, -6, 2);
    lv_obj_set_style_text_font(lbl_scale_bot, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_scale_bot, ui_theme_muted(), 0);
    lv_label_set_text(lbl_scale_bot, "↓ -- °C");

    // The Line Chart
    chart = lv_chart_create(chart_card);
    lv_obj_set_pos(chart, 4, 18);
    lv_obj_set_size(chart, 418, 160);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, CHART_POINTS);

    // Chart styling: clean background, subtle grid lines, 0-radius line dots
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_chart_set_div_line_count(chart, 4, 0);
    lv_obj_set_style_line_color(chart, ui_theme_border(), LV_PART_MAIN);
    lv_obj_set_style_line_opa(chart, LV_OPA_40, LV_PART_MAIN);

    // Hide heavy points on lines for clean modern curves
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);

    // Add 4 Series
    ser_outdoor = lv_chart_add_series(chart, lv_color_hex(TRACK_COLORS[0]), LV_CHART_AXIS_PRIMARY_Y);
    ser_indoor  = lv_chart_add_series(chart, lv_color_hex(TRACK_COLORS[1]), LV_CHART_AXIS_PRIMARY_Y);
    ser_custom1 = lv_chart_add_series(chart, lv_color_hex(TRACK_COLORS[2]), LV_CHART_AXIS_PRIMARY_Y);
    ser_custom2 = lv_chart_add_series(chart, lv_color_hex(TRACK_COLORS[3]), LV_CHART_AXIS_PRIMARY_Y);

    // Time Axis Footer Labels
    lbl_axis_start = lv_label_create(chart_card);
    lv_obj_align(lbl_axis_start, LV_ALIGN_BOTTOM_LEFT, 6, -2);
    lv_obj_set_style_text_font(lbl_axis_start, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_axis_start, ui_theme_muted(), 0);
    lv_label_set_text(lbl_axis_start, "-1 Std.");

    lbl_axis_mid = lv_label_create(chart_card);
    lv_obj_align(lbl_axis_mid, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_text_font(lbl_axis_mid, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_axis_mid, ui_theme_muted(), 0);
    lv_label_set_text(lbl_axis_mid, "-30 Min.");

    lv_obj_t *lbl_axis_now = lv_label_create(chart_card);
    lv_obj_align(lbl_axis_now, LV_ALIGN_BOTTOM_RIGHT, -6, -2);
    lv_obj_set_style_text_font(lbl_axis_now, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_axis_now, ui_theme_muted(), 0);
    lv_label_set_text(lbl_axis_now, "Jetzt");

    ui_update_temps_tab();
}

void ui_update_temps_tab(void) {
    if (!root_page || !chart) return;

    int win = constrain(state.temps_history_window, 1, 12);
    lv_coord_t points[CHART_POINTS];
    float min_val = 0.0f, max_val = 0.0f;

    float overall_min = 999.0f;
    float overall_max = -999.0f;
    bool has_any_series = false;

    char buf[48];

    // Track 0: Außen
    if (temp_history_get_series(0, win, points, CHART_POINTS, &min_val, &max_val)) {
        for (int i = 0; i < CHART_POINTS; ++i) {
            lv_chart_set_value_by_id(chart, ser_outdoor, i, points[i]);
        }
        if (min_val < overall_min) overall_min = min_val;
        if (max_val > overall_max) overall_max = max_val;
        has_any_series = true;

        if (isfinite(state.outdoor_temp) && state.outdoor_temp > -40.0f && state.outdoor_temp < 80.0f) {
            snprintf(buf, sizeof(buf), "%.1f °C", state.outdoor_temp);
            ui_label_set_text_if_changed(lbl_vals[0], buf);
        } else {
            ui_label_set_text_if_changed(lbl_vals[0], "--");
        }
        snprintf(buf, sizeof(buf), "↓%.1f°  ↑%.1f°", min_val, max_val);
        ui_label_set_text_if_changed(lbl_minmax[0], buf);
    } else {
        for (int i = 0; i < CHART_POINTS; ++i) lv_chart_set_value_by_id(chart, ser_outdoor, i, LV_CHART_POINT_NONE);
        ui_label_set_text_if_changed(lbl_vals[0], "--");
        ui_label_set_text_if_changed(lbl_minmax[0], "--");
    }
    // Update name
    int out_idx = constrain(state.outdoor_temp_sensor, 0, TEMP_SOURCE_COUNT - 1);
    if (state.temp_sensor_names[out_idx].length() > 0) {
        snprintf(buf, sizeof(buf), "Außen (%s)", state.temp_sensor_names[out_idx].c_str());
        ui_label_set_text_if_changed(lbl_names[0], buf);
    } else {
        ui_label_set_text_if_changed(lbl_names[0], "Außentemperatur");
    }

    // Track 1: Innen
    if (temp_history_get_series(1, win, points, CHART_POINTS, &min_val, &max_val)) {
        for (int i = 0; i < CHART_POINTS; ++i) {
            lv_chart_set_value_by_id(chart, ser_indoor, i, points[i]);
        }
        if (min_val < overall_min) overall_min = min_val;
        if (max_val > overall_max) overall_max = max_val;
        has_any_series = true;

        if (isfinite(state.indoor_temp) && state.indoor_temp > -40.0f && state.indoor_temp < 80.0f) {
            snprintf(buf, sizeof(buf), "%.1f °C", state.indoor_temp);
            ui_label_set_text_if_changed(lbl_vals[1], buf);
        } else {
            ui_label_set_text_if_changed(lbl_vals[1], "--");
        }
        snprintf(buf, sizeof(buf), "↓%.1f°  ↑%.1f°", min_val, max_val);
        ui_label_set_text_if_changed(lbl_minmax[1], buf);
    } else {
        for (int i = 0; i < CHART_POINTS; ++i) lv_chart_set_value_by_id(chart, ser_indoor, i, LV_CHART_POINT_NONE);
        ui_label_set_text_if_changed(lbl_vals[1], "--");
        ui_label_set_text_if_changed(lbl_minmax[1], "--");
    }
    int in_idx = constrain(state.indoor_temp_sensor, 0, TEMP_SOURCE_COUNT - 1);
    if (state.temp_sensor_names[in_idx].length() > 0) {
        snprintf(buf, sizeof(buf), "Innen (%s)", state.temp_sensor_names[in_idx].c_str());
        ui_label_set_text_if_changed(lbl_names[1], buf);
    } else {
        ui_label_set_text_if_changed(lbl_names[1], "Innentemperatur");
    }

    // Track 2: Zusatz 1
    int c1_idx = state.temps_custom_sensor[0];
    if (c1_idx >= 0 && c1_idx < TEMP_SOURCE_COUNT && temp_history_get_series(2, win, points, CHART_POINTS, &min_val, &max_val)) {
        lv_obj_clear_flag(cards[2], LV_OBJ_FLAG_HIDDEN);
        for (int i = 0; i < CHART_POINTS; ++i) {
            lv_chart_set_value_by_id(chart, ser_custom1, i, points[i]);
        }
        if (min_val < overall_min) overall_min = min_val;
        if (max_val > overall_max) overall_max = max_val;
        has_any_series = true;

        float c1_val = state.temp_sensors[c1_idx];
        if (isfinite(c1_val) && c1_val > -40.0f && c1_val < 80.0f) {
            snprintf(buf, sizeof(buf), "%.1f °C", c1_val);
            ui_label_set_text_if_changed(lbl_vals[2], buf);
        } else {
            ui_label_set_text_if_changed(lbl_vals[2], "--");
        }
        snprintf(buf, sizeof(buf), "↓%.1f°  ↑%.1f°", min_val, max_val);
        ui_label_set_text_if_changed(lbl_minmax[2], buf);
        const char *nm = state.temp_sensor_names[c1_idx].length() ? state.temp_sensor_names[c1_idx].c_str() : "Sensor 3";
        ui_label_set_text_if_changed(lbl_names[2], nm);
    } else {
        for (int i = 0; i < CHART_POINTS; ++i) lv_chart_set_value_by_id(chart, ser_custom1, i, LV_CHART_POINT_NONE);
        ui_label_set_text_if_changed(lbl_names[2], "Zusatz 1 (Aus)");
        ui_label_set_text_if_changed(lbl_vals[2], "--");
        ui_label_set_text_if_changed(lbl_minmax[2], "Deaktiviert");
    }

    // Track 3: Zusatz 2
    int c2_idx = state.temps_custom_sensor[1];
    if (c2_idx >= 0 && c2_idx < TEMP_SOURCE_COUNT && temp_history_get_series(3, win, points, CHART_POINTS, &min_val, &max_val)) {
        lv_obj_clear_flag(cards[3], LV_OBJ_FLAG_HIDDEN);
        for (int i = 0; i < CHART_POINTS; ++i) {
            lv_chart_set_value_by_id(chart, ser_custom2, i, points[i]);
        }
        if (min_val < overall_min) overall_min = min_val;
        if (max_val > overall_max) overall_max = max_val;
        has_any_series = true;

        float c2_val = state.temp_sensors[c2_idx];
        if (isfinite(c2_val) && c2_val > -40.0f && c2_val < 80.0f) {
            snprintf(buf, sizeof(buf), "%.1f °C", c2_val);
            ui_label_set_text_if_changed(lbl_vals[3], buf);
        } else {
            ui_label_set_text_if_changed(lbl_vals[3], "--");
        }
        snprintf(buf, sizeof(buf), "↓%.1f°  ↑%.1f°", min_val, max_val);
        ui_label_set_text_if_changed(lbl_minmax[3], buf);
        const char *nm = state.temp_sensor_names[c2_idx].length() ? state.temp_sensor_names[c2_idx].c_str() : "Sensor 4";
        ui_label_set_text_if_changed(lbl_names[3], nm);
    } else {
        for (int i = 0; i < CHART_POINTS; ++i) lv_chart_set_value_by_id(chart, ser_custom2, i, LV_CHART_POINT_NONE);
        ui_label_set_text_if_changed(lbl_names[3], "Zusatz 2 (Aus)");
        ui_label_set_text_if_changed(lbl_vals[3], "--");
        ui_label_set_text_if_changed(lbl_minmax[3], "Deaktiviert");
    }

    // Auto-scale Y-axis range
    if (has_any_series && overall_max > overall_min) {
        float span = overall_max - overall_min;
        if (span < 4.0f) {
            float mid = (overall_max + overall_min) * 0.5f;
            overall_min = mid - 2.0f;
            overall_max = mid + 2.0f;
        } else {
            overall_min = floorf(overall_min - 0.5f);
            overall_max = ceilf(overall_max + 0.5f);
        }
    } else {
        overall_min = 10.0f;
        overall_max = 30.0f;
    }

    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, (lv_coord_t)roundf(overall_min * 10.0f), (lv_coord_t)roundf(overall_max * 10.0f));

    if (lbl_scale_top) {
        snprintf(buf, sizeof(buf), "Max: %.1f °C", overall_max);
        ui_label_set_text_if_changed(lbl_scale_top, buf);
    }
    if (lbl_scale_bot) {
        snprintf(buf, sizeof(buf), "Min: %.1f °C", overall_min);
        ui_label_set_text_if_changed(lbl_scale_bot, buf);
    }

    lv_chart_refresh(chart);
}
