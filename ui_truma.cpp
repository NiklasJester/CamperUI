#include "ui_main.h"
#include "http_handler.h"

// ============================================================================
// Truma Combi & CP Plus Control UI
// ============================================================================

// --- Header / Diagnostic Widgets ---
static lv_obj_t *lbl_model = nullptr;
static lv_obj_t *lbl_indoor_temp = nullptr;
static lv_obj_t *badge_online = nullptr;
static lv_obj_t *lbl_online = nullptr;
static lv_obj_t *badge_230v = nullptr;
static lv_obj_t *lbl_230v = nullptr;
static lv_obj_t *badge_error = nullptr;
static lv_obj_t *lbl_error = nullptr;

// --- Room Heating Widgets ---
static lv_obj_t *btn_heat_toggle = nullptr;
static lv_obj_t *lbl_heat_toggle = nullptr;
static lv_obj_t *lbl_target_temp = nullptr;
static lv_obj_t *btn_temp_minus = nullptr;
static lv_obj_t *btn_temp_plus = nullptr;
static lv_obj_t *btn_preset[4] = {nullptr, nullptr, nullptr, nullptr};
static const int PRESET_TEMPS[4] = {18, 20, 22, 24};

// --- Hot Water Boiler Widgets ---
static lv_obj_t *lbl_water_status = nullptr;
static lv_obj_t *btn_water_off = nullptr;
static lv_obj_t *lbl_btn_water_off = nullptr;
static lv_obj_t *btn_water_eco = nullptr;
static lv_obj_t *lbl_btn_water_eco = nullptr;
static lv_obj_t *btn_water_hot = nullptr;
static lv_obj_t *lbl_btn_water_hot = nullptr;

// --- Energy Mix Widgets ---
static lv_obj_t *card_energy = nullptr;
static lv_obj_t *mix_row = nullptr;
static lv_obj_t *el_row = nullptr;
static lv_obj_t *lbl_energy_status = nullptr;
static lv_obj_t *lbl_230v_hint = nullptr;
static lv_obj_t *btn_energy_fuel = nullptr;
static lv_obj_t *lbl_btn_energy_fuel = nullptr;
static lv_obj_t *btn_energy_mix1 = nullptr;
static lv_obj_t *lbl_btn_energy_mix1 = nullptr;
static lv_obj_t *btn_energy_mix2 = nullptr;
static lv_obj_t *lbl_btn_energy_mix2 = nullptr;
static lv_obj_t *btn_energy_el1 = nullptr;
static lv_obj_t *lbl_btn_energy_el1 = nullptr;
static lv_obj_t *btn_energy_el2 = nullptr;
static lv_obj_t *lbl_btn_energy_el2 = nullptr;

// Helper: Button styling for active / inactive states
static void style_segmented_button(lv_obj_t *btn, lv_obj_t *lbl, bool active, uint32_t active_color_hex = UI_COLOR_PRIMARY) {
    if (!btn) return;
    if (active) {
        lv_obj_set_style_bg_color(btn, lv_color_hex(active_color_hex), 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(active_color_hex), 0);
        if (lbl) lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), 0);
    } else {
        lv_obj_set_style_bg_color(btn, ui_theme_card(), 0);
        lv_obj_set_style_border_color(btn, ui_theme_border(), 0);
        if (lbl) lv_obj_set_style_text_color(lbl, ui_theme_text(), 0);
    }
}

// ----------------------------------------------------------------------------
// Event Handlers
// ----------------------------------------------------------------------------

static void heat_toggle_event_cb(lv_event_t *e) {
    (void)e;
    state.truma_hold_until = millis() + 4000;
    if (state.truma_heating_on) {
        state.truma_heating_on = false;
        http_publish_truma_air(0); // Off
    } else {
        state.truma_heating_on = true;
        if (state.truma_target_temp < 5.0f || state.truma_target_temp > 30.0f) {
            state.truma_target_temp = 22.0f;
        }
        http_publish_truma_air((int)(state.truma_target_temp + 0.5f));
    }
    ui_update_truma_tab();
}

static void temp_adjust_event_cb(lv_event_t *e) {
    int delta = (int)(intptr_t)lv_event_get_user_data(e);
    state.truma_hold_until = millis() + 4000;
    
    float new_t = state.truma_target_temp + delta;
    if (new_t < 5.0f) new_t = 5.0f;
    if (new_t > 30.0f) new_t = 30.0f;
    state.truma_target_temp = new_t;

    if (state.truma_heating_on) {
        http_publish_truma_air((int)(state.truma_target_temp + 0.5f));
    }
    ui_update_truma_tab();
}

static void preset_event_cb(lv_event_t *e) {
    int t = (int)(intptr_t)lv_event_get_user_data(e);
    state.truma_hold_until = millis() + 4000;
    state.truma_target_temp = (float)t;

    if (state.truma_heating_on) {
        http_publish_truma_air(t);
    }
    ui_update_truma_tab();
}

static void water_btn_event_cb(lv_event_t *e) {
    const char *mode = (const char *)lv_event_get_user_data(e);
    state.truma_hold_until = millis() + 4000;
    state.truma_water_mode = mode ? mode : "off";
    http_publish_truma_water(state.truma_water_mode.c_str());
    ui_update_truma_tab();
}

static void energy_btn_event_cb(lv_event_t *e) {
    const char *mode = (const char *)lv_event_get_user_data(e);
    state.truma_hold_until = millis() + 4000;
    state.truma_energy_mode = mode ? mode : "fuel";
    http_publish_truma_energy(state.truma_energy_mode.c_str());
    ui_update_truma_tab();
}

// ----------------------------------------------------------------------------
// Reset
// ----------------------------------------------------------------------------

void ui_truma_reset() {
    lbl_model = nullptr;
    lbl_indoor_temp = nullptr;
    badge_online = nullptr;
    lbl_online = nullptr;
    badge_230v = nullptr;
    lbl_230v = nullptr;
    badge_error = nullptr;
    lbl_error = nullptr;

    btn_heat_toggle = nullptr;
    lbl_heat_toggle = nullptr;
    lbl_target_temp = nullptr;
    btn_temp_minus = nullptr;
    btn_temp_plus = nullptr;
    for (int i = 0; i < 4; i++) btn_preset[i] = nullptr;

    lbl_water_status = nullptr;
    btn_water_off = nullptr;
    lbl_btn_water_off = nullptr;
    btn_water_eco = nullptr;
    lbl_btn_water_eco = nullptr;
    btn_water_hot = nullptr;
    lbl_btn_water_hot = nullptr;

    card_energy = nullptr;
    mix_row = nullptr;
    el_row = nullptr;
    lbl_energy_status = nullptr;
    lbl_230v_hint = nullptr;
    btn_energy_fuel = nullptr;
    lbl_btn_energy_fuel = nullptr;
    btn_energy_mix1 = nullptr;
    lbl_btn_energy_mix1 = nullptr;
    btn_energy_mix2 = nullptr;
    lbl_btn_energy_mix2 = nullptr;
    btn_energy_el1 = nullptr;
    lbl_btn_energy_el1 = nullptr;
    btn_energy_el2 = nullptr;
    lbl_btn_energy_el2 = nullptr;
}

// ----------------------------------------------------------------------------
// Build Truma Tab
// ----------------------------------------------------------------------------

void ui_build_truma(lv_obj_t *parent) {
    ui_truma_reset();
    ui_setup_tab_page(parent);

    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(parent, 10, 0);

    // ========================================================================
    // 1. Header Card (Model, Room Temp, Badges)
    // ========================================================================
    lv_obj_t *card_hdr = ui_create_card(parent, 440, 56);
    lv_obj_set_style_pad_all(card_hdr, 10, 0);

    lbl_model = lv_label_create(card_hdr);
    lv_label_set_text(lbl_model, state.truma_model.length() > 0 ? state.truma_model.c_str() : "Truma Combi");
    lv_obj_set_style_text_font(lbl_model, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_model, ui_theme_text(), 0);
    lv_obj_align(lbl_model, LV_ALIGN_TOP_LEFT, 2, 0);

    lbl_indoor_temp = lv_label_create(card_hdr);
    lv_label_set_text_fmt(lbl_indoor_temp, "Innen: %.1f C", state.indoor_temp);
    lv_obj_set_style_text_font(lbl_indoor_temp, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_indoor_temp, ui_theme_muted(), 0);
    lv_obj_align(lbl_indoor_temp, LV_ALIGN_BOTTOM_LEFT, 2, 0);

    // Badges Container (right side)
    lv_obj_t *badges_cont = lv_obj_create(card_hdr);
    lv_obj_set_size(badges_cont, LV_SIZE_CONTENT, 36);
    lv_obj_align(badges_cont, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_opa(badges_cont, 0, 0);
    lv_obj_set_style_border_width(badges_cont, 0, 0);
    lv_obj_set_style_pad_all(badges_cont, 0, 0);
    lv_obj_set_flex_flow(badges_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(badges_cont, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(badges_cont, 6, 0);

    // 230V Badge
    badge_230v = lv_obj_create(badges_cont);
    lv_obj_set_size(badge_230v, LV_SIZE_CONTENT, 28);
    lv_obj_set_style_radius(badge_230v, 8, 0);
    lv_obj_set_style_pad_hor(badge_230v, 8, 0);
    lv_obj_set_style_pad_ver(badge_230v, 4, 0);
    lv_obj_set_style_border_width(badge_230v, 1, 0);
    lbl_230v = lv_label_create(badge_230v);
    lv_label_set_text(lbl_230v, "230V");
    lv_obj_set_style_text_font(lbl_230v, &lv_font_montserrat_12, 0);
    lv_obj_center(lbl_230v);

    // Online Status Badge
    badge_online = lv_obj_create(badges_cont);
    lv_obj_set_size(badge_online, LV_SIZE_CONTENT, 28);
    lv_obj_set_style_radius(badge_online, 8, 0);
    lv_obj_set_style_pad_hor(badge_online, 8, 0);
    lv_obj_set_style_pad_ver(badge_online, 4, 0);
    lv_obj_set_style_border_width(badge_online, 1, 0);
    lbl_online = lv_label_create(badge_online);
    lv_label_set_text(lbl_online, "Online");
    lv_obj_set_style_text_font(lbl_online, &lv_font_montserrat_12, 0);
    lv_obj_center(lbl_online);

    // Error Badge
    badge_error = lv_obj_create(badges_cont);
    lv_obj_set_size(badge_error, LV_SIZE_CONTENT, 28);
    lv_obj_set_style_radius(badge_error, 8, 0);
    lv_obj_set_style_pad_hor(badge_error, 8, 0);
    lv_obj_set_style_pad_ver(badge_error, 4, 0);
    lv_obj_set_style_bg_color(badge_error, lv_color_hex(UI_COLOR_DANGER), 0);
    lv_obj_set_style_border_color(badge_error, lv_color_hex(0xf87171), 0);
    lv_obj_set_style_border_width(badge_error, 1, 0);
    lbl_error = lv_label_create(badge_error);
    lv_label_set_text(lbl_error, "Fehler");
    lv_obj_set_style_text_font(lbl_error, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_error, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_error);
    lv_obj_add_flag(badge_error, LV_OBJ_FLAG_HIDDEN);

    // ========================================================================
    // 2. Raumheizung Card
    // ========================================================================
    lv_obj_t *card_heat = ui_create_card(parent, 440, 185);
    lv_obj_set_style_pad_all(card_heat, 12, 0);

    // Title Row
    lv_obj_t *lbl_heat_title = lv_label_create(card_heat);
    lv_label_set_text(lbl_heat_title, "Raumheizung");
    lv_obj_set_style_text_font(lbl_heat_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_heat_title, ui_theme_text(), 0);
    lv_obj_align(lbl_heat_title, LV_ALIGN_TOP_LEFT, 2, 4);

    // On / Off Toggle Button
    btn_heat_toggle = lv_btn_create(card_heat);
    lv_obj_set_size(btn_heat_toggle, 92, 34);
    lv_obj_align(btn_heat_toggle, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_radius(btn_heat_toggle, 10, 0);
    lv_obj_set_style_border_width(btn_heat_toggle, 1, 0);
    lv_obj_set_style_shadow_width(btn_heat_toggle, 0, 0);
    lv_obj_add_event_cb(btn_heat_toggle, heat_toggle_event_cb, LV_EVENT_CLICKED, nullptr);

    lbl_heat_toggle = lv_label_create(btn_heat_toggle);
    lv_label_set_text(lbl_heat_toggle, "Start");
    lv_obj_set_style_text_font(lbl_heat_toggle, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_heat_toggle);

    // Stepper Row: [-]  [  22 C  ]  [+]
    lv_obj_t *stepper_row = lv_obj_create(card_heat);
    lv_obj_set_size(stepper_row, 416, 68);
    lv_obj_align(stepper_row, LV_ALIGN_TOP_MID, 0, 44);
    lv_obj_set_style_bg_opa(stepper_row, 0, 0);
    lv_obj_set_style_border_width(stepper_row, 0, 0);
    lv_obj_set_style_pad_all(stepper_row, 0, 0);
    lv_obj_set_flex_flow(stepper_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(stepper_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Minus Button
    btn_temp_minus = lv_btn_create(stepper_row);
    lv_obj_set_size(btn_temp_minus, 72, 54);
    lv_obj_set_style_radius(btn_temp_minus, 14, 0);
    lv_obj_set_style_bg_color(btn_temp_minus, ui_theme_card(), 0);
    lv_obj_set_style_border_color(btn_temp_minus, ui_theme_border(), 0);
    lv_obj_set_style_border_width(btn_temp_minus, 1, 0);
    lv_obj_set_style_shadow_width(btn_temp_minus, 0, 0);
    lv_obj_add_event_cb(btn_temp_minus, temp_adjust_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)-1);
    lv_obj_t *lbl_minus = lv_label_create(btn_temp_minus);
    lv_label_set_text(lbl_minus, "-");
    lv_obj_set_style_text_font(lbl_minus, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_color(lbl_minus, ui_theme_text(), 0);
    lv_obj_center(lbl_minus);

    // Target Temp Display Container
    lv_obj_t *temp_disp = lv_obj_create(stepper_row);
    lv_obj_set_size(temp_disp, 220, 64);
    lv_obj_set_style_bg_opa(temp_disp, 0, 0);
    lv_obj_set_style_border_width(temp_disp, 0, 0);
    lv_obj_set_style_pad_all(temp_disp, 0, 0);
    lv_obj_set_flex_flow(temp_disp, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(temp_disp, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lbl_target_temp = lv_label_create(temp_disp);
    lv_label_set_text_fmt(lbl_target_temp, "%.0f C", state.truma_target_temp);
    lv_obj_set_style_text_font(lbl_target_temp, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(lbl_target_temp, ui_theme_text(), 0);

    lv_obj_t *lbl_target_sub = lv_label_create(temp_disp);
    lv_label_set_text(lbl_target_sub, "Zieltemperatur (5 - 30 C)");
    lv_obj_set_style_text_font(lbl_target_sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_target_sub, ui_theme_muted(), 0);

    // Plus Button
    btn_temp_plus = lv_btn_create(stepper_row);
    lv_obj_set_size(btn_temp_plus, 72, 54);
    lv_obj_set_style_radius(btn_temp_plus, 14, 0);
    lv_obj_set_style_bg_color(btn_temp_plus, ui_theme_card(), 0);
    lv_obj_set_style_border_color(btn_temp_plus, ui_theme_border(), 0);
    lv_obj_set_style_border_width(btn_temp_plus, 1, 0);
    lv_obj_set_style_shadow_width(btn_temp_plus, 0, 0);
    lv_obj_add_event_cb(btn_temp_plus, temp_adjust_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)+1);
    lv_obj_t *lbl_plus = lv_label_create(btn_temp_plus);
    lv_label_set_text(lbl_plus, "+");
    lv_obj_set_style_text_font(lbl_plus, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_color(lbl_plus, ui_theme_text(), 0);
    lv_obj_center(lbl_plus);

    // Bottom Preset Row: [18] [20] [22] [24]
    lv_obj_t *preset_row = lv_obj_create(card_heat);
    lv_obj_set_size(preset_row, 416, 40);
    lv_obj_align(preset_row, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(preset_row, 0, 0);
    lv_obj_set_style_border_width(preset_row, 0, 0);
    lv_obj_set_style_pad_all(preset_row, 0, 0);
    lv_obj_set_flex_flow(preset_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(preset_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < 4; i++) {
        btn_preset[i] = lv_btn_create(preset_row);
        lv_obj_set_size(btn_preset[i], 94, 34);
        lv_obj_set_style_radius(btn_preset[i], 8, 0);
        lv_obj_set_style_bg_color(btn_preset[i], ui_theme_track(), 0);
        lv_obj_set_style_border_color(btn_preset[i], ui_theme_border(), 0);
        lv_obj_set_style_border_width(btn_preset[i], 1, 0);
        lv_obj_set_style_shadow_width(btn_preset[i], 0, 0);
        lv_obj_add_event_cb(btn_preset[i], preset_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)PRESET_TEMPS[i]);

        lv_obj_t *lbl_p = lv_label_create(btn_preset[i]);
        lv_label_set_text_fmt(lbl_p, "%d C", PRESET_TEMPS[i]);
        lv_obj_set_style_text_font(lbl_p, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lbl_p, ui_theme_text(), 0);
        lv_obj_center(lbl_p);
    }

    // ========================================================================
    // 3. Warmwasser Boiler Card
    // ========================================================================
    lv_obj_t *card_water = ui_create_card(parent, 440, 110);
    lv_obj_set_style_pad_all(card_water, 12, 0);

    lv_obj_t *lbl_w_title = lv_label_create(card_water);
    lv_label_set_text(lbl_w_title, "Warmwasser Boiler");
    lv_obj_set_style_text_font(lbl_w_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_w_title, ui_theme_text(), 0);
    lv_obj_align(lbl_w_title, LV_ALIGN_TOP_LEFT, 2, 2);

    lbl_water_status = lv_label_create(card_water);
    lv_label_set_text(lbl_water_status, "Aus");
    lv_obj_set_style_text_font(lbl_water_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_water_status, ui_theme_muted(), 0);
    lv_obj_align(lbl_water_status, LV_ALIGN_TOP_RIGHT, -2, 2);

    // 3 Segmented Buttons: [Aus]  [Eco 40C]  [Hot 60C]
    lv_obj_t *water_btn_row = lv_obj_create(card_water);
    lv_obj_set_size(water_btn_row, 416, 46);
    lv_obj_align(water_btn_row, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(water_btn_row, 0, 0);
    lv_obj_set_style_border_width(water_btn_row, 0, 0);
    lv_obj_set_style_pad_all(water_btn_row, 0, 0);
    lv_obj_set_flex_flow(water_btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(water_btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Button 1: Off
    btn_water_off = lv_btn_create(water_btn_row);
    lv_obj_set_size(btn_water_off, 130, 44);
    lv_obj_set_style_radius(btn_water_off, 10, 0);
    lv_obj_set_style_border_width(btn_water_off, 1, 0);
    lv_obj_set_style_shadow_width(btn_water_off, 0, 0);
    lv_obj_add_event_cb(btn_water_off, water_btn_event_cb, LV_EVENT_CLICKED, (void *)"off");
    lbl_btn_water_off = lv_label_create(btn_water_off);
    lv_label_set_text(lbl_btn_water_off, "Aus");
    lv_obj_set_style_text_font(lbl_btn_water_off, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_btn_water_off);

    // Button 2: Eco 40C
    btn_water_eco = lv_btn_create(water_btn_row);
    lv_obj_set_size(btn_water_eco, 130, 44);
    lv_obj_set_style_radius(btn_water_eco, 10, 0);
    lv_obj_set_style_border_width(btn_water_eco, 1, 0);
    lv_obj_set_style_shadow_width(btn_water_eco, 0, 0);
    lv_obj_add_event_cb(btn_water_eco, water_btn_event_cb, LV_EVENT_CLICKED, (void *)"eco");
    lbl_btn_water_eco = lv_label_create(btn_water_eco);
    lv_label_set_text(lbl_btn_water_eco, "Eco (40 C)");
    lv_obj_set_style_text_font(lbl_btn_water_eco, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_btn_water_eco);

    // Button 3: Hot 60C
    btn_water_hot = lv_btn_create(water_btn_row);
    lv_obj_set_size(btn_water_hot, 130, 44);
    lv_obj_set_style_radius(btn_water_hot, 10, 0);
    lv_obj_set_style_border_width(btn_water_hot, 1, 0);
    lv_obj_set_style_shadow_width(btn_water_hot, 0, 0);
    lv_obj_add_event_cb(btn_water_hot, water_btn_event_cb, LV_EVENT_CLICKED, (void *)"hot");
    lbl_btn_water_hot = lv_label_create(btn_water_hot);
    lv_label_set_text(lbl_btn_water_hot, "Hot (60 C)");
    lv_obj_set_style_text_font(lbl_btn_water_hot, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_btn_water_hot);

    // ========================================================================
    // 4. Energiequelle Card
    // ========================================================================
    card_energy = ui_create_card(parent, 440, truma_has_electric(state.heater_type) ? 185 : 88);
    lv_obj_set_style_pad_all(card_energy, 12, 0);

    lv_obj_t *lbl_e_title = lv_label_create(card_energy);
    lv_label_set_text(lbl_e_title, "Energiequelle");
    lv_obj_set_style_text_font(lbl_e_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_e_title, ui_theme_text(), 0);
    lv_obj_align(lbl_e_title, LV_ALIGN_TOP_LEFT, 2, 2);

    lbl_energy_status = lv_label_create(card_energy);
    lv_label_set_text(lbl_energy_status, "Brennstoff");
    lv_obj_set_style_text_font(lbl_energy_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_energy_status, ui_theme_muted(), 0);
    lv_obj_align(lbl_energy_status, LV_ALIGN_TOP_RIGHT, -2, 2);

    lbl_230v_hint = lv_label_create(card_energy);
    lv_label_set_text(lbl_230v_hint, "Kein 230V Landstrom");
    lv_obj_set_style_text_font(lbl_230v_hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_230v_hint, lv_color_hex(UI_COLOR_WARNING), 0);
    lv_obj_align(lbl_230v_hint, LV_ALIGN_TOP_RIGHT, -2, 20);
    lv_obj_add_flag(lbl_230v_hint, LV_OBJ_FLAG_HIDDEN);

    // Row 1: Fuel / Gas (Full width)
    btn_energy_fuel = lv_btn_create(card_energy);
    lv_obj_set_size(btn_energy_fuel, 416, 40);
    lv_obj_align(btn_energy_fuel, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_radius(btn_energy_fuel, 10, 0);
    lv_obj_set_style_border_width(btn_energy_fuel, 1, 0);
    lv_obj_set_style_shadow_width(btn_energy_fuel, 0, 0);
    lv_obj_add_event_cb(btn_energy_fuel, energy_btn_event_cb, LV_EVENT_CLICKED, (void *)"fuel");
    lbl_btn_energy_fuel = lv_label_create(btn_energy_fuel);
    lv_label_set_text(lbl_btn_energy_fuel, "Brennstoff (Gas / Diesel)");
    lv_obj_set_style_text_font(lbl_btn_energy_fuel, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_btn_energy_fuel);

    // Row 2: Mix 1 & Mix 2
    mix_row = lv_obj_create(card_energy);
    lv_obj_set_size(mix_row, 416, 42);
    lv_obj_align(mix_row, LV_ALIGN_TOP_MID, 0, 84);
    lv_obj_set_style_bg_opa(mix_row, 0, 0);
    lv_obj_set_style_border_width(mix_row, 0, 0);
    lv_obj_set_style_pad_all(mix_row, 0, 0);
    lv_obj_set_flex_flow(mix_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mix_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    btn_energy_mix1 = lv_btn_create(mix_row);
    lv_obj_set_size(btn_energy_mix1, 202, 40);
    lv_obj_set_style_radius(btn_energy_mix1, 10, 0);
    lv_obj_set_style_border_width(btn_energy_mix1, 1, 0);
    lv_obj_set_style_shadow_width(btn_energy_mix1, 0, 0);
    lv_obj_add_event_cb(btn_energy_mix1, energy_btn_event_cb, LV_EVENT_CLICKED, (void *)"mix1");
    lbl_btn_energy_mix1 = lv_label_create(btn_energy_mix1);
    lv_label_set_text(lbl_btn_energy_mix1, "Mix 1 (900W)");
    lv_obj_set_style_text_font(lbl_btn_energy_mix1, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_btn_energy_mix1);

    btn_energy_mix2 = lv_btn_create(mix_row);
    lv_obj_set_size(btn_energy_mix2, 202, 40);
    lv_obj_set_style_radius(btn_energy_mix2, 10, 0);
    lv_obj_set_style_border_width(btn_energy_mix2, 1, 0);
    lv_obj_set_style_shadow_width(btn_energy_mix2, 0, 0);
    lv_obj_add_event_cb(btn_energy_mix2, energy_btn_event_cb, LV_EVENT_CLICKED, (void *)"mix2");
    lbl_btn_energy_mix2 = lv_label_create(btn_energy_mix2);
    lv_label_set_text(lbl_btn_energy_mix2, "Mix 2 (1800W)");
    lv_obj_set_style_text_font(lbl_btn_energy_mix2, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_btn_energy_mix2);

    // Row 3: Elektro 1 & Elektro 2
    el_row = lv_obj_create(card_energy);
    lv_obj_set_size(el_row, 416, 42);
    lv_obj_align(el_row, LV_ALIGN_TOP_MID, 0, 132);
    lv_obj_set_style_bg_opa(el_row, 0, 0);
    lv_obj_set_style_border_width(el_row, 0, 0);
    lv_obj_set_style_pad_all(el_row, 0, 0);
    lv_obj_set_flex_flow(el_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(el_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    btn_energy_el1 = lv_btn_create(el_row);
    lv_obj_set_size(btn_energy_el1, 202, 40);
    lv_obj_set_style_radius(btn_energy_el1, 10, 0);
    lv_obj_set_style_border_width(btn_energy_el1, 1, 0);
    lv_obj_set_style_shadow_width(btn_energy_el1, 0, 0);
    lv_obj_add_event_cb(btn_energy_el1, energy_btn_event_cb, LV_EVENT_CLICKED, (void *)"el1");
    lbl_btn_energy_el1 = lv_label_create(btn_energy_el1);
    lv_label_set_text(lbl_btn_energy_el1, "Elektro 1 (900W)");
    lv_obj_set_style_text_font(lbl_btn_energy_el1, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_btn_energy_el1);

    btn_energy_el2 = lv_btn_create(el_row);
    lv_obj_set_size(btn_energy_el2, 202, 40);
    lv_obj_set_style_radius(btn_energy_el2, 10, 0);
    lv_obj_set_style_border_width(btn_energy_el2, 1, 0);
    lv_obj_set_style_shadow_width(btn_energy_el2, 0, 0);
    lv_obj_add_event_cb(btn_energy_el2, energy_btn_event_cb, LV_EVENT_CLICKED, (void *)"el2");
    lbl_btn_energy_el2 = lv_label_create(btn_energy_el2);
    lv_label_set_text(lbl_btn_energy_el2, "Elektro 2 (1800W)");
    lv_obj_set_style_text_font(lbl_btn_energy_el2, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_btn_energy_el2);

    ui_update_truma_tab();
}

// ----------------------------------------------------------------------------
// Periodic / Live UI Update
// ----------------------------------------------------------------------------

void ui_update_truma_tab() {
    if (!lbl_model) return;

    // 1. Header Updates
    if (state.truma_model.length() > 0) {
        ui_label_set_text_if_changed(lbl_model, state.truma_model.c_str());
    } else {
        ui_label_set_text_if_changed(lbl_model, "Truma Combi");
    }
    ui_label_set_float(lbl_indoor_temp, "Innen: %.1f C", state.indoor_temp);

    // Online Status Badge
    if (state.truma_online) {
        ui_label_set_text_if_changed(lbl_online, "Online");
        lv_obj_set_style_bg_color(badge_online, lv_color_hex(UI_COLOR_SUCCESS), 0);
        lv_obj_set_style_border_color(badge_online, lv_color_hex(0x34d399), 0);
        lv_obj_set_style_text_color(lbl_online, lv_color_hex(0xffffff), 0);
    } else {
        ui_label_set_text_if_changed(lbl_online, "Offline");
        lv_obj_set_style_bg_color(badge_online, ui_theme_card(), 0);
        lv_obj_set_style_border_color(badge_online, ui_theme_border(), 0);
        lv_obj_set_style_text_color(lbl_online, ui_theme_muted(), 0);
    }

    // 230V Badge
    if (!truma_has_electric(state.heater_type)) {
        if (badge_230v) lv_obj_add_flag(badge_230v, LV_OBJ_FLAG_HIDDEN);
    } else if (badge_230v) {
        lv_obj_clear_flag(badge_230v, LV_OBJ_FLAG_HIDDEN);
        if (state.truma_230v) {
            ui_label_set_text_if_changed(lbl_230v, "230V");
            lv_obj_set_style_bg_color(badge_230v, lv_color_hex(UI_COLOR_PRIMARY), 0);
            lv_obj_set_style_border_color(badge_230v, lv_color_hex(0x38bdf8), 0);
            lv_obj_set_style_text_color(lbl_230v, lv_color_hex(0xffffff), 0);
        } else {
            ui_label_set_text_if_changed(lbl_230v, "Kein 230V");
            lv_obj_set_style_bg_color(badge_230v, ui_theme_card(), 0);
            lv_obj_set_style_border_color(badge_230v, ui_theme_border(), 0);
            lv_obj_set_style_text_color(lbl_230v, ui_theme_muted(), 0);
        }
    }

    // Error Badge
    bool has_err = (state.truma_error.length() > 0 && 
                    state.truma_error != "none" && 
                    state.truma_error != "no" && 
                    state.truma_error != "0");
    if (has_err) {
        char err_buf[32];
        snprintf(err_buf, sizeof(err_buf), "Fehler: %s", state.truma_error.c_str());
        ui_label_set_text_if_changed(lbl_error, err_buf);
        lv_obj_clear_flag(badge_error, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(badge_error, LV_OBJ_FLAG_HIDDEN);
    }

    // 2. Room Heating Card Updates
    if (btn_heat_toggle && lbl_heat_toggle) {
        if (state.truma_heating_on) {
            lv_obj_set_style_bg_color(btn_heat_toggle, lv_color_hex(UI_COLOR_DANGER), 0);
            lv_obj_set_style_border_color(btn_heat_toggle, lv_color_hex(0xf87171), 0);
            ui_label_set_text_if_changed(lbl_heat_toggle, "Stop");
            lv_obj_set_style_text_color(lbl_heat_toggle, lv_color_hex(0xffffff), 0);
        } else {
            lv_obj_set_style_bg_color(btn_heat_toggle, lv_color_hex(UI_COLOR_SUCCESS), 0);
            lv_obj_set_style_border_color(btn_heat_toggle, lv_color_hex(0x34d399), 0);
            ui_label_set_text_if_changed(lbl_heat_toggle, "Start");
            lv_obj_set_style_text_color(lbl_heat_toggle, lv_color_hex(0xffffff), 0);
        }
    }

    if (lbl_target_temp) {
        ui_label_set_float(lbl_target_temp, "%.0f C", state.truma_target_temp);
    }

    // Preset button highlights
    for (int i = 0; i < 4; i++) {
        if (btn_preset[i]) {
            bool is_cur = ((int)(state.truma_target_temp + 0.5f) == PRESET_TEMPS[i]);
            lv_color_t bg = is_cur ? lv_color_hex(UI_COLOR_PRIMARY) : ui_theme_track();
            lv_obj_set_style_bg_color(btn_preset[i], bg, 0);
            lv_obj_t *lbl = lv_obj_get_child(btn_preset[i], 0);
            if (lbl) {
                lv_obj_set_style_text_color(lbl, is_cur ? lv_color_hex(0xffffff) : ui_theme_text(), 0);
            }
        }
    }

    // 3. Water Boiler Updates
    String wm = state.truma_water_mode; wm.toLowerCase();
    bool w_off = (wm == "off" || wm.length() == 0);
    bool w_eco = (wm == "eco");
    bool w_hot = (wm == "hot" || wm == "boost");

    if (lbl_water_status) {
        if (w_eco) ui_label_set_text_if_changed(lbl_water_status, "Eco (40 C)");
        else if (w_hot) ui_label_set_text_if_changed(lbl_water_status, "Hot (60 C)");
        else ui_label_set_text_if_changed(lbl_water_status, "Aus");

        lv_color_t sc = (w_eco || w_hot) ? lv_color_hex(UI_COLOR_SUCCESS) : ui_theme_muted();
        lv_obj_set_style_text_color(lbl_water_status, sc, 0);
    }

    style_segmented_button(btn_water_off, lbl_btn_water_off, w_off, UI_COLOR_TRACK_DARK);
    style_segmented_button(btn_water_eco, lbl_btn_water_eco, w_eco, UI_COLOR_SUCCESS);
    style_segmented_button(btn_water_hot, lbl_btn_water_hot, w_hot, UI_COLOR_WARNING);

    // 4. Energy Mix Updates
    bool has_el = truma_has_electric(state.heater_type);
    if (!has_el) {
        if (mix_row) lv_obj_add_flag(mix_row, LV_OBJ_FLAG_HIDDEN);
        if (el_row)  lv_obj_add_flag(el_row, LV_OBJ_FLAG_HIDDEN);
        if (lbl_230v_hint) lv_obj_add_flag(lbl_230v_hint, LV_OBJ_FLAG_HIDDEN);
        if (card_energy) lv_obj_set_height(card_energy, 88);
    } else {
        if (mix_row) lv_obj_clear_flag(mix_row, LV_OBJ_FLAG_HIDDEN);
        if (el_row)  lv_obj_clear_flag(el_row, LV_OBJ_FLAG_HIDDEN);
        if (card_energy) lv_obj_set_height(card_energy, 185);
    }

    String em = state.truma_energy_mode; em.toLowerCase();
    bool e_fuel = (em == "fuel" || em == "gas");
    bool e_mix1 = (em == "mix1");
    bool e_mix2 = (em == "mix2");
    bool e_el1  = (em == "el1");
    bool e_el2  = (em == "el2");

    if (lbl_energy_status) {
        if (!has_el || e_fuel) ui_label_set_text_if_changed(lbl_energy_status, "Brennstoff");
        else if (e_mix1) ui_label_set_text_if_changed(lbl_energy_status, "Mix 1 (900W)");
        else if (e_mix2) ui_label_set_text_if_changed(lbl_energy_status, "Mix 2 (1800W)");
        else if (e_el1)  ui_label_set_text_if_changed(lbl_energy_status, "Elektro 1 (900W)");
        else if (e_el2)  ui_label_set_text_if_changed(lbl_energy_status, "Elektro 2 (1800W)");
        else ui_label_set_text_if_changed(lbl_energy_status, em.c_str());
    }

    if (lbl_230v_hint) {
        if (has_el && !state.truma_230v && (e_mix1 || e_mix2 || e_el1 || e_el2)) {
            lv_obj_clear_flag(lbl_230v_hint, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(lbl_230v_hint, LV_OBJ_FLAG_HIDDEN);
        }
    }

    style_segmented_button(btn_energy_fuel, lbl_btn_energy_fuel, (!has_el || e_fuel), UI_COLOR_PRIMARY);
    if (has_el) {
        style_segmented_button(btn_energy_mix1, lbl_btn_energy_mix1, e_mix1, UI_COLOR_WARNING);
        style_segmented_button(btn_energy_mix2, lbl_btn_energy_mix2, e_mix2, UI_COLOR_WARNING);
        style_segmented_button(btn_energy_el1,  lbl_btn_energy_el1,  e_el1,  UI_COLOR_PRIMARY);
        style_segmented_button(btn_energy_el2,  lbl_btn_energy_el2,  e_el2,  UI_COLOR_PRIMARY);
    }
}
