#include "ui_temps.h"
#include "ui_main.h"
#include "temp_history.h"
#include <stdio.h>
#include <math.h>

static lv_obj_t *root_page = nullptr;
static lv_obj_t *chart = nullptr;
static lv_obj_t *chart_card = nullptr;
static lv_chart_series_t *ser_outdoor = nullptr;
static lv_chart_series_t *ser_indoor = nullptr;
static lv_chart_series_t *ser_custom1 = nullptr;
static lv_chart_series_t *ser_custom2 = nullptr;

static lv_obj_t *btn_win_1h = nullptr;
static lv_obj_t *btn_win_6h = nullptr;
static lv_obj_t *btn_win_12h = nullptr;
static lv_obj_t *btn_win_24h = nullptr;

static lv_obj_t *cards[4] = {};
static lv_obj_t *lbl_names[4] = {};
static lv_obj_t *lbl_vals[4] = {};
static lv_obj_t *lbl_max[4] = {};
static lv_obj_t *lbl_min[4] = {};

static lv_obj_t *lbl_scale_top = nullptr;
static lv_obj_t *lbl_scale_bot = nullptr;
static lv_obj_t *lbl_axis_start = nullptr;
static lv_obj_t *lbl_axis_mid = nullptr;

static constexpr int CHART_POINTS = 60;

static const uint32_t TRACK_COLORS_DARK[4] = {
    0x38bdf8, // Aussen: Sky Blue
    0x34d399, // Innen: Emerald Green
    0xfbbf24, // Zusatz 1: Amber
    0xc084fc  // Zusatz 2: Purple
};

static const uint32_t TRACK_COLORS_LIGHT[4] = {
    0x0284c7, // Aussen: Darker Blue
    0x059669, // Innen: Forest Green
    0xd97706, // Zusatz 1: Dark Amber
    0x9333ea  // Zusatz 2: Purple
};

static inline lv_color_t track_color(int i) {
    return lv_color_hex(state.dark_mode ? TRACK_COLORS_DARK[i] : TRACK_COLORS_LIGHT[i]);
}

// Animation state
static bool temps_animating = false;
static int32_t anim_revealed_points = CHART_POINTS;

static void update_win_buttons_style(void) {
    int win = state.temps_history_window;
    auto style_btn = [](lv_obj_t *btn, bool active) {
        if (!btn) return;
        lv_obj_set_style_bg_color(btn, active ? lv_color_hex(UI_COLOR_PRIMARY) : ui_theme_track(), 0);
        lv_obj_set_style_text_color(btn, active ? lv_color_hex(0xffffff) : ui_theme_muted(), 0);
    };
    style_btn(btn_win_1h,  win == 1);
    style_btn(btn_win_6h,  win == 6);
    style_btn(btn_win_12h, win == 12);
    style_btn(btn_win_24h, win == 24);

    if (lbl_axis_start) {
        if (win == 1)       lv_label_set_text(lbl_axis_start, "-1 Std.");
        else if (win == 6)  lv_label_set_text(lbl_axis_start, "-6 Std.");
        else if (win == 12) lv_label_set_text(lbl_axis_start, "-12 Std.");
        else                lv_label_set_text(lbl_axis_start, "-24 Std.");
    }
    if (lbl_axis_mid) {
        if (win == 1)       lv_label_set_text(lbl_axis_mid, "-30 Min.");
        else if (win == 6)  lv_label_set_text(lbl_axis_mid, "-3 Std.");
        else if (win == 12) lv_label_set_text(lbl_axis_mid, "-6 Std.");
        else                lv_label_set_text(lbl_axis_mid, "-12 Std.");
    }
}

static void win_btn_clicked(lv_event_t *e) {
    int win = (int)(intptr_t)lv_event_get_user_data(e);
    state.temps_history_window = win;
    update_win_buttons_style();
    ui_trigger_temps_anim();
}

static void temps_anim_cb(void *var, int32_t val) {
    anim_revealed_points = val;
    StateLockGuard lock;
    ui_update_temps_tab();
}

static void temps_anim_ready_cb(lv_anim_t *a) {
    temps_animating = false;
    anim_revealed_points = CHART_POINTS;
    StateLockGuard lock;
    ui_update_temps_tab();
}

void ui_trigger_temps_anim(void) {
    if (!chart) return;
    lv_anim_del(chart, temps_anim_cb);
    temps_animating = true;
    anim_revealed_points = 2;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, chart);
    lv_anim_set_exec_cb(&a, temps_anim_cb);
    lv_anim_set_ready_cb(&a, temps_anim_ready_cb);
    lv_anim_set_values(&a, 2, CHART_POINTS);
    lv_anim_set_time(&a, 750);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static int last_layout_custom_count = -1;
static int last_layout_c1 = -1;
static int last_layout_c2 = -1;
static uint32_t last_rendered_hist_ver = 0xFFFFFFFF;
static int last_rendered_win = -1;
static int last_rendered_points_to_show = -1;

void ui_build_temps(lv_obj_t *parent) {
    last_layout_custom_count = -1;
    last_layout_c1 = -1;
    last_layout_c2 = -1;
    last_rendered_hist_ver = 0xFFFFFFFF;
    last_rendered_win = -1;
    last_rendered_points_to_show = -1;

    root_page = parent;
    ui_setup_tab_page(parent);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // 1. Header Row (y = 0, h = 36px): Title & Large Touch Buttons (1h, 6h, 12h, 24h)
    // ==========================================
    lv_obj_t *title_box = lv_obj_create(parent);
    lv_obj_remove_style_all(title_box);
    lv_obj_set_size(title_box, 155, 36);
    lv_obj_align(title_box, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_flex_flow(title_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(title_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(title_box, 6, 0);

    lv_obj_t *lbl_icon = lv_label_create(title_box);
    lv_label_set_text(lbl_icon, MDI_THERMOMETER);
    lv_obj_set_style_text_font(lbl_icon, &ui_font_mdi_18, 0);
    lv_obj_set_style_text_color(lbl_icon, lv_color_hex(UI_COLOR_PRIMARY), 0);

    lv_obj_t *lbl_title = lv_label_create(title_box);
    lv_label_set_text(lbl_title, "Temperaturen");
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, lv_color_hex(UI_COLOR_PRIMARY), 0);

    // Generous Touch Button Container (Right aligned)
    lv_obj_t *pill_box = lv_obj_create(parent);
    lv_obj_remove_style_all(pill_box);
    lv_obj_set_size(pill_box, 275, 36);
    lv_obj_align(pill_box, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_flex_flow(pill_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pill_box, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(pill_box, 6, 0);

    auto create_pill = [&](const char *txt, int win) -> lv_obj_t* {
        lv_obj_t *b = lv_btn_create(pill_box);
        lv_obj_set_size(b, 62, 34);
        lv_obj_set_style_radius(b, 8, 0);
        lv_obj_set_style_border_width(b, 1, 0);
        lv_obj_set_style_border_color(b, ui_theme_border(), 0);
        lv_obj_add_event_cb(b, win_btn_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)win);
        lv_obj_t *l = lv_label_create(b);
        lv_label_set_text(l, txt);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
        lv_obj_center(l);
        return b;
    };

    btn_win_1h  = create_pill("1h", 1);
    btn_win_6h  = create_pill("6h", 6);
    btn_win_12h = create_pill("12h", 12);
    btn_win_24h = create_pill("24h", 24);
    update_win_buttons_style();

    // ==========================================
    // 2. Metric Cards (52px height, 26px prominent temperature readout)
    // ==========================================
    for (int i = 0; i < 4; ++i) {
        cards[i] = lv_obj_create(parent);
        lv_obj_set_size(cards[i], 216, 52);
        lv_obj_set_style_bg_color(cards[i], ui_theme_card(), 0);
        lv_obj_set_style_border_color(cards[i], ui_theme_border(), 0);
        lv_obj_set_style_border_width(cards[i], 1, 0);
        lv_obj_set_style_radius(cards[i], 12, 0);
        lv_obj_set_style_pad_all(cards[i], 0, 0);
        lv_obj_clear_flag(cards[i], LV_OBJ_FLAG_SCROLLABLE);

        // Colored accent bar
        lv_obj_t *accent = lv_obj_create(cards[i]);
        lv_obj_remove_style_all(accent);
        lv_obj_set_size(accent, 4, 36);
        lv_obj_set_pos(accent, 4, 8);
        lv_obj_set_style_radius(accent, 2, 0);
        lv_obj_set_style_bg_color(accent, track_color(i), 0);
        lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);

        // Sensor name (Top-left, 14px font)
        lbl_names[i] = lv_label_create(cards[i]);
        lv_obj_set_pos(lbl_names[i], 16, 5);
        lv_obj_set_width(lbl_names[i], 110);
        lv_label_set_long_mode(lbl_names[i], LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_font(lbl_names[i], &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_names[i], ui_theme_text(), 0);

        // Current value (Bottom-left: prominent 26px font highlighted in track color)
        lbl_vals[i] = lv_label_create(cards[i]);
        lv_obj_set_pos(lbl_vals[i], 16, 21);
        lv_obj_set_style_text_font(lbl_vals[i], &lv_font_montserrat_26, 0);
        lv_obj_set_style_text_color(lbl_vals[i], track_color(i), 0);
        lv_label_set_text(lbl_vals[i], "--");

        // Max sub-label (Top-right, 12px font)
        lbl_max[i] = lv_label_create(cards[i]);
        lv_obj_align(lbl_max[i], LV_ALIGN_TOP_RIGHT, -10, 6);
        lv_obj_set_style_text_font(lbl_max[i], &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(lbl_max[i], ui_theme_muted(), 0);
        lv_label_set_text(lbl_max[i], "Max: --");

        // Min sub-label (Bottom-right, 12px font)
        lbl_min[i] = lv_label_create(cards[i]);
        lv_obj_align(lbl_min[i], LV_ALIGN_BOTTOM_RIGHT, -10, -6);
        lv_obj_set_style_text_font(lbl_min[i], &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(lbl_min[i], ui_theme_muted(), 0);
        lv_label_set_text(lbl_min[i], "Min: --");
    }

    lv_label_set_text(lbl_names[0], "Außen");
    lv_label_set_text(lbl_names[1], "Innen");
    lv_label_set_text(lbl_names[2], "Zusatz 1");
    lv_label_set_text(lbl_names[3], "Zusatz 2");

    // ==========================================
    // 3. Chart Card (y = 152, h = 206px, w = 440px)
    // ==========================================
    chart_card = lv_obj_create(parent);
    lv_obj_set_pos(chart_card, 0, 152);
    lv_obj_set_size(chart_card, 440, 206);
    lv_obj_set_style_bg_color(chart_card, ui_theme_card(), 0);
    lv_obj_set_style_border_color(chart_card, ui_theme_border(), 0);
    lv_obj_set_style_border_width(chart_card, 1, 0);
    lv_obj_set_style_radius(chart_card, 12, 0);
    lv_obj_set_style_pad_all(chart_card, 6, 0);
    lv_obj_clear_flag(chart_card, LV_OBJ_FLAG_SCROLLABLE);

    // Scale header labels (12px font for readability)
    lbl_scale_top = lv_label_create(chart_card);
    lv_obj_align(lbl_scale_top, LV_ALIGN_TOP_LEFT, 6, 2);
    lv_obj_set_style_text_font(lbl_scale_top, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_scale_top, ui_theme_muted(), 0);
    lv_label_set_text(lbl_scale_top, "Max: -- C");

    lbl_scale_bot = lv_label_create(chart_card);
    lv_obj_align(lbl_scale_bot, LV_ALIGN_TOP_RIGHT, -6, 2);
    lv_obj_set_style_text_font(lbl_scale_bot, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_scale_bot, ui_theme_muted(), 0);
    lv_label_set_text(lbl_scale_bot, "Min: -- C");

    // The Line Chart
    chart = lv_chart_create(chart_card);
    lv_obj_set_pos(chart, 4, 20);
    lv_obj_set_size(chart, 418, 154);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, CHART_POINTS);

    // Clean modern chart styling
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_chart_set_div_line_count(chart, 4, 0);
    lv_obj_set_style_line_color(chart, ui_theme_border(), LV_PART_MAIN);
    lv_obj_set_style_line_opa(chart, LV_OPA_40, LV_PART_MAIN);

    // 3px wide curves for bold, distinct curves
    lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);

    // Add 4 Series with track colors
    ser_outdoor = lv_chart_add_series(chart, track_color(0), LV_CHART_AXIS_PRIMARY_Y);
    ser_indoor  = lv_chart_add_series(chart, track_color(1), LV_CHART_AXIS_PRIMARY_Y);
    ser_custom1 = lv_chart_add_series(chart, track_color(2), LV_CHART_AXIS_PRIMARY_Y);
    ser_custom2 = lv_chart_add_series(chart, track_color(3), LV_CHART_AXIS_PRIMARY_Y);

    // Time Axis Footer Labels (12px font)
    lbl_axis_start = lv_label_create(chart_card);
    lv_obj_align(lbl_axis_start, LV_ALIGN_BOTTOM_LEFT, 6, -2);
    lv_obj_set_style_text_font(lbl_axis_start, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_axis_start, ui_theme_muted(), 0);
    lv_label_set_text(lbl_axis_start, "-1 Std.");

    lbl_axis_mid = lv_label_create(chart_card);
    lv_obj_align(lbl_axis_mid, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_text_font(lbl_axis_mid, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_axis_mid, ui_theme_muted(), 0);
    lv_label_set_text(lbl_axis_mid, "-30 Min.");

    lv_obj_t *lbl_axis_now = lv_label_create(chart_card);
    lv_obj_align(lbl_axis_now, LV_ALIGN_BOTTOM_RIGHT, -6, -2);
    lv_obj_set_style_text_font(lbl_axis_now, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_axis_now, ui_theme_muted(), 0);
    lv_label_set_text(lbl_axis_now, "Jetzt");

    // Initial render and launch reveal animation
    ui_update_temps_tab();
    ui_trigger_temps_anim();
}

void ui_update_temps_tab(void) {
    if (!root_page || !chart || !chart_card) return;

    bool c1_active = (state.temps_custom_sensor[0] >= 0 && state.temps_custom_sensor[0] < TEMP_SOURCE_COUNT);
    bool c2_active = (state.temps_custom_sensor[1] >= 0 && state.temps_custom_sensor[1] < TEMP_SOURCE_COUNT);
    int custom_count = (c1_active ? 1 : 0) + (c2_active ? 1 : 0);

    // Dynamic card & chart layout depending on number of active sensors
    if (custom_count != last_layout_custom_count ||
        state.temps_custom_sensor[0] != last_layout_c1 ||
        state.temps_custom_sensor[1] != last_layout_c2) {
        last_layout_custom_count = custom_count;
        last_layout_c1 = state.temps_custom_sensor[0];
        last_layout_c2 = state.temps_custom_sensor[1];

        if (custom_count == 0) {
            // Only 2 cards: Outdoor and Indoor side-by-side
            lv_obj_clear_flag(cards[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(cards[0], 0, 42);
            lv_obj_set_size(cards[0], 216, 52);
            lv_obj_set_width(lbl_names[0], 110);
            lv_obj_align(lbl_max[0], LV_ALIGN_TOP_RIGHT, -10, 6);
            lv_obj_align(lbl_min[0], LV_ALIGN_BOTTOM_RIGHT, -10, -6);

            lv_obj_clear_flag(cards[1], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(cards[1], 224, 42);
            lv_obj_set_size(cards[1], 216, 52);
            lv_obj_set_width(lbl_names[1], 110);
            lv_obj_align(lbl_max[1], LV_ALIGN_TOP_RIGHT, -10, 6);
            lv_obj_align(lbl_min[1], LV_ALIGN_BOTTOM_RIGHT, -10, -6);

            lv_obj_add_flag(cards[2], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cards[3], LV_OBJ_FLAG_HIDDEN);

            // Expand chart card to 256px height
            lv_obj_set_pos(chart_card, 0, 102);
            lv_obj_set_size(chart_card, 440, 256);
            lv_obj_set_pos(chart, 4, 22);
            lv_obj_set_size(chart, 418, 202);
        } else if (custom_count == 1) {
            // 3 cards: Outdoor and Indoor on row 1, single custom sensor on row 2 (full width)
            lv_obj_clear_flag(cards[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(cards[0], 0, 40);
            lv_obj_set_size(cards[0], 216, 52);
            lv_obj_set_width(lbl_names[0], 110);
            lv_obj_align(lbl_max[0], LV_ALIGN_TOP_RIGHT, -10, 6);
            lv_obj_align(lbl_min[0], LV_ALIGN_BOTTOM_RIGHT, -10, -6);

            lv_obj_clear_flag(cards[1], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(cards[1], 224, 40);
            lv_obj_set_size(cards[1], 216, 52);
            lv_obj_set_width(lbl_names[1], 110);
            lv_obj_align(lbl_max[1], LV_ALIGN_TOP_RIGHT, -10, 6);
            lv_obj_align(lbl_min[1], LV_ALIGN_BOTTOM_RIGHT, -10, -6);

            int active_idx = c1_active ? 2 : 3;
            int hidden_idx = c1_active ? 3 : 2;

            lv_obj_add_flag(cards[hidden_idx], LV_OBJ_FLAG_HIDDEN);

            lv_obj_clear_flag(cards[active_idx], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(cards[active_idx], 0, 98);
            lv_obj_set_size(cards[active_idx], 440, 52);
            lv_obj_set_width(lbl_names[active_idx], 280);
            lv_obj_align(lbl_max[active_idx], LV_ALIGN_TOP_RIGHT, -12, 6);
            lv_obj_align(lbl_min[active_idx], LV_ALIGN_BOTTOM_RIGHT, -12, -6);

            lv_obj_set_pos(chart_card, 0, 156);
            lv_obj_set_size(chart_card, 440, 202);
            lv_obj_set_pos(chart, 4, 20);
            lv_obj_set_size(chart, 418, 150);
        } else {
            // 4 cards: 2x2 grid
            lv_obj_clear_flag(cards[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(cards[0], 0, 40);
            lv_obj_set_size(cards[0], 216, 52);
            lv_obj_set_width(lbl_names[0], 110);
            lv_obj_align(lbl_max[0], LV_ALIGN_TOP_RIGHT, -10, 6);
            lv_obj_align(lbl_min[0], LV_ALIGN_BOTTOM_RIGHT, -10, -6);

            lv_obj_clear_flag(cards[1], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(cards[1], 224, 40);
            lv_obj_set_size(cards[1], 216, 52);
            lv_obj_set_width(lbl_names[1], 110);
            lv_obj_align(lbl_max[1], LV_ALIGN_TOP_RIGHT, -10, 6);
            lv_obj_align(lbl_min[1], LV_ALIGN_BOTTOM_RIGHT, -10, -6);

            lv_obj_clear_flag(cards[2], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(cards[2], 0, 98);
            lv_obj_set_size(cards[2], 216, 52);
            lv_obj_set_width(lbl_names[2], 110);
            lv_obj_align(lbl_max[2], LV_ALIGN_TOP_RIGHT, -10, 6);
            lv_obj_align(lbl_min[2], LV_ALIGN_BOTTOM_RIGHT, -10, -6);

            lv_obj_clear_flag(cards[3], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(cards[3], 224, 98);
            lv_obj_set_size(cards[3], 216, 52);
            lv_obj_set_width(lbl_names[3], 110);
            lv_obj_align(lbl_max[3], LV_ALIGN_TOP_RIGHT, -10, 6);
            lv_obj_align(lbl_min[3], LV_ALIGN_BOTTOM_RIGHT, -10, -6);

            lv_obj_set_pos(chart_card, 0, 156);
            lv_obj_set_size(chart_card, 440, 202);
            lv_obj_set_pos(chart, 4, 20);
            lv_obj_set_size(chart, 418, 150);
        }
    }

    char buf[48];

    // 1. Live Current Values & Names (updated if changed, zero overhead if identical)
    if (isfinite(state.outdoor_temp) && state.outdoor_temp > -40.0f && state.outdoor_temp < 80.0f) {
        snprintf(buf, sizeof(buf), "%.1f C", state.outdoor_temp);
        ui_label_set_text_if_changed(lbl_vals[0], buf);
    } else {
        ui_label_set_text_if_changed(lbl_vals[0], "--");
    }
    ui_label_set_text_if_changed(lbl_names[0], "Außen");

    if (isfinite(state.indoor_temp) && state.indoor_temp > -40.0f && state.indoor_temp < 80.0f) {
        snprintf(buf, sizeof(buf), "%.1f C", state.indoor_temp);
        ui_label_set_text_if_changed(lbl_vals[1], buf);
    } else {
        ui_label_set_text_if_changed(lbl_vals[1], "--");
    }
    ui_label_set_text_if_changed(lbl_names[1], "Innen");

    int c1_idx = state.temps_custom_sensor[0];
    if (c1_active) {
        float c1_val = state.temp_sensors[c1_idx];
        if (isfinite(c1_val) && c1_val > -40.0f && c1_val < 80.0f) {
            snprintf(buf, sizeof(buf), "%.1f C", c1_val);
            ui_label_set_text_if_changed(lbl_vals[2], buf);
        } else {
            ui_label_set_text_if_changed(lbl_vals[2], "--");
        }
        const char *nm = state.temp_sensor_names[c1_idx].length() ? state.temp_sensor_names[c1_idx].c_str() : (c1_idx < 4 ? "Temp 3" : "Ruuvi");
        ui_label_set_text_if_changed(lbl_names[2], nm);
    } else {
        ui_label_set_text_if_changed(lbl_vals[2], "--");
    }

    int c2_idx = state.temps_custom_sensor[1];
    if (c2_active) {
        float c2_val = state.temp_sensors[c2_idx];
        if (isfinite(c2_val) && c2_val > -40.0f && c2_val < 80.0f) {
            snprintf(buf, sizeof(buf), "%.1f C", c2_val);
            ui_label_set_text_if_changed(lbl_vals[3], buf);
        } else {
            ui_label_set_text_if_changed(lbl_vals[3], "--");
        }
        const char *nm = state.temp_sensor_names[c2_idx].length() ? state.temp_sensor_names[c2_idx].c_str() : (c2_idx < 4 ? "Temp 4" : "Ruuvi");
        ui_label_set_text_if_changed(lbl_names[3], nm);
    } else {
        ui_label_set_text_if_changed(lbl_vals[3], "--");
    }

    // Ensure colors reflect active theme
    for (int i = 0; i < 4; ++i) {
        ui_text_color_if_changed(lbl_vals[i], track_color(i));
    }

    // 2. Chart History Throttling: only recalculate curves when a new minute-sample arrived,
    // window changed, or reveal animation is in flight.
    int win = constrain(state.temps_history_window, 1, 24);
    uint32_t hist_ver = temp_history_get_version();
    int points_to_show = temps_animating ? anim_revealed_points : CHART_POINTS;
    if (points_to_show < 2) points_to_show = 2;
    if (points_to_show > CHART_POINTS) points_to_show = CHART_POINTS;

    bool chart_needs_update = temps_animating ||
                              (win != last_rendered_win) ||
                              (hist_ver != last_rendered_hist_ver) ||
                              (points_to_show != last_rendered_points_to_show);

    if (!chart_needs_update) return;

    last_rendered_win = win;
    last_rendered_hist_ver = hist_ver;
    last_rendered_points_to_show = points_to_show;

    lv_coord_t points[CHART_POINTS];
    float min_val = 0.0f, max_val = 0.0f;
    float overall_min = 999.0f;
    float overall_max = -999.0f;
    bool has_any_series = false;

    // Track 0: Außen
    if (temp_history_get_series(0, win, points, CHART_POINTS, &min_val, &max_val)) {
        for (int i = 0; i < CHART_POINTS; ++i) {
            if (i < points_to_show) {
                lv_chart_set_value_by_id(chart, ser_outdoor, i, points[i]);
            } else {
                lv_chart_set_value_by_id(chart, ser_outdoor, i, LV_CHART_POINT_NONE);
            }
        }
        if (min_val < overall_min) overall_min = min_val;
        if (max_val > overall_max) overall_max = max_val;
        has_any_series = true;

        snprintf(buf, sizeof(buf), "Max: %.1f C", max_val);
        ui_label_set_text_if_changed(lbl_max[0], buf);
        snprintf(buf, sizeof(buf), "Min: %.1f C", min_val);
        ui_label_set_text_if_changed(lbl_min[0], buf);
    } else {
        for (int i = 0; i < CHART_POINTS; ++i) lv_chart_set_value_by_id(chart, ser_outdoor, i, LV_CHART_POINT_NONE);
        ui_label_set_text_if_changed(lbl_max[0], "Max: --");
        ui_label_set_text_if_changed(lbl_min[0], "Min: --");
    }

    // Track 1: Innen
    if (temp_history_get_series(1, win, points, CHART_POINTS, &min_val, &max_val)) {
        for (int i = 0; i < CHART_POINTS; ++i) {
            if (i < points_to_show) {
                lv_chart_set_value_by_id(chart, ser_indoor, i, points[i]);
            } else {
                lv_chart_set_value_by_id(chart, ser_indoor, i, LV_CHART_POINT_NONE);
            }
        }
        if (min_val < overall_min) overall_min = min_val;
        if (max_val > overall_max) overall_max = max_val;
        has_any_series = true;

        snprintf(buf, sizeof(buf), "Max: %.1f C", max_val);
        ui_label_set_text_if_changed(lbl_max[1], buf);
        snprintf(buf, sizeof(buf), "Min: %.1f C", min_val);
        ui_label_set_text_if_changed(lbl_min[1], buf);
    } else {
        for (int i = 0; i < CHART_POINTS; ++i) lv_chart_set_value_by_id(chart, ser_indoor, i, LV_CHART_POINT_NONE);
        ui_label_set_text_if_changed(lbl_max[1], "Max: --");
        ui_label_set_text_if_changed(lbl_min[1], "Min: --");
    }

    // Track 2: Zusatz 1
    if (c1_active && temp_history_get_series(2, win, points, CHART_POINTS, &min_val, &max_val)) {
        for (int i = 0; i < CHART_POINTS; ++i) {
            if (i < points_to_show) {
                lv_chart_set_value_by_id(chart, ser_custom1, i, points[i]);
            } else {
                lv_chart_set_value_by_id(chart, ser_custom1, i, LV_CHART_POINT_NONE);
            }
        }
        if (min_val < overall_min) overall_min = min_val;
        if (max_val > overall_max) overall_max = max_val;
        has_any_series = true;

        snprintf(buf, sizeof(buf), "Max: %.1f C", max_val);
        ui_label_set_text_if_changed(lbl_max[2], buf);
        snprintf(buf, sizeof(buf), "Min: %.1f C", min_val);
        ui_label_set_text_if_changed(lbl_min[2], buf);
    } else {
        for (int i = 0; i < CHART_POINTS; ++i) lv_chart_set_value_by_id(chart, ser_custom1, i, LV_CHART_POINT_NONE);
        ui_label_set_text_if_changed(lbl_max[2], "Max: --");
        ui_label_set_text_if_changed(lbl_min[2], "Min: --");
    }

    // Track 3: Zusatz 2
    if (c2_active && temp_history_get_series(3, win, points, CHART_POINTS, &min_val, &max_val)) {
        for (int i = 0; i < CHART_POINTS; ++i) {
            if (i < points_to_show) {
                lv_chart_set_value_by_id(chart, ser_custom2, i, points[i]);
            } else {
                lv_chart_set_value_by_id(chart, ser_custom2, i, LV_CHART_POINT_NONE);
            }
        }
        if (min_val < overall_min) overall_min = min_val;
        if (max_val > overall_max) overall_max = max_val;
        has_any_series = true;

        snprintf(buf, sizeof(buf), "Max: %.1f C", max_val);
        ui_label_set_text_if_changed(lbl_max[3], buf);
        snprintf(buf, sizeof(buf), "Min: %.1f C", min_val);
        ui_label_set_text_if_changed(lbl_min[3], buf);
    } else {
        for (int i = 0; i < CHART_POINTS; ++i) lv_chart_set_value_by_id(chart, ser_custom2, i, LV_CHART_POINT_NONE);
        ui_label_set_text_if_changed(lbl_max[3], "Max: --");
        ui_label_set_text_if_changed(lbl_min[3], "Min: --");
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
        snprintf(buf, sizeof(buf), "Max: %.1f C", overall_max);
        ui_label_set_text_if_changed(lbl_scale_top, buf);
    }
    if (lbl_scale_bot) {
        snprintf(buf, sizeof(buf), "Min: %.1f C", overall_min);
        ui_label_set_text_if_changed(lbl_scale_bot, buf);
    }

    lv_chart_refresh(chart);
}

