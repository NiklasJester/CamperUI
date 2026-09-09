#include "ui_main.h"

static lv_obj_t *arc_battery;
static lv_obj_t *arc_solar;
static lv_obj_t *lbl_bat_pct;
static lv_obj_t *lbl_bat_volt;
static lv_obj_t *lbl_bat_curr;
static lv_obj_t *lbl_bat_ttg;
static lv_obj_t *lbl_bat_charge_icon;

static lv_obj_t *lbl_solar_watt;
static lv_obj_t *lbl_solar_curr;

// Visual Energy Flow Elements
static lv_obj_t *lbl_flow_solar_val;
static lv_obj_t *lbl_flow_arr1;
static lv_obj_t *lbl_flow_bat_val;
static lv_obj_t *lbl_flow_arr2;
static lv_obj_t *lbl_flow_load_val;

static int last_soc = -1;
static int last_solar = -1;

static void set_arc_val_cb(void * var, int32_t v) {
    lv_arc_set_value((lv_obj_t*)var, v);
}

void ui_trigger_power_anim() {
    last_soc = -1;
    last_solar = -1;
    if (arc_battery) lv_arc_set_value(arc_battery, 0);
    if (arc_solar) lv_arc_set_value(arc_solar, 0);
}

void ui_update_power_tab() {
    if (!arc_battery) return;
    
    // Battery SOC
    if (state.bat_soc != last_soc) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, arc_battery);
        lv_anim_set_exec_cb(&a, set_arc_val_cb);
        lv_anim_set_time(&a, 1000);
        lv_anim_set_values(&a, lv_arc_get_value(arc_battery), state.bat_soc);
        lv_anim_start(&a);
        last_soc = state.bat_soc;
        lv_label_set_text_fmt(lbl_bat_pct, "%d%%", state.bat_soc);
        
        // Dynamic arc color based on battery health
        if (state.bat_soc <= state.warn_bat_soc) {
            lv_obj_set_style_arc_color(arc_battery, lv_color_hex(UI_COLOR_DANGER), LV_PART_INDICATOR);
            lv_obj_set_style_text_color(lbl_bat_pct, lv_color_hex(UI_COLOR_DANGER), 0);
        } else if (state.bat_soc <= 40) {
            lv_obj_set_style_arc_color(arc_battery, lv_color_hex(UI_COLOR_WARNING), LV_PART_INDICATOR);
            lv_obj_set_style_text_color(lbl_bat_pct, lv_color_hex(UI_COLOR_WARNING), 0);
        } else {
            lv_obj_set_style_arc_color(arc_battery, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_INDICATOR);
            lv_obj_set_style_text_color(lbl_bat_pct, ui_theme_text(), 0);
        }
    }
    
    // Voltage / Current
    ui_label_set_float(lbl_bat_volt, "%.1f V", state.bat_voltage);
    ui_label_set_float(lbl_bat_curr, "%+.1f A", state.bat_current);
    if (state.bat_current > 0.05f) {
        lv_obj_set_style_text_color(lbl_bat_curr, lv_color_hex(UI_COLOR_SUCCESS), 0);
    } else if (state.bat_current < -0.05f) {
        lv_obj_set_style_text_color(lbl_bat_curr, lv_color_hex(UI_COLOR_WARNING), 0);
    } else {
        lv_obj_set_style_text_color(lbl_bat_curr, ui_theme_muted(), 0);
    }
    
    // TTG estimation
    if (state.bat_current < -0.1f) {
        float capacity_ah = state.bat_capacity_ah > 0 ? state.bat_capacity_ah : 100.0f;
        float rem_ah = (state.bat_soc / 100.0f) * capacity_ah;
        float hrs = rem_ah / (-state.bat_current);
        if (hrs > 99.0f) hrs = 99.0f;
        ui_label_set_float(lbl_bat_ttg, "Restzeit: %.1f h", hrs);
    } else if (state.bat_current > 0.1f) {
        lv_label_set_text(lbl_bat_ttg, "Wird geladen");
    } else {
        lv_label_set_text(lbl_bat_ttg, "Standby");
    }
    
    // Charge icon
    if (state.bat_current > 0.05f) {
        lv_obj_clear_flag(lbl_bat_charge_icon, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(lbl_bat_charge_icon, LV_OBJ_FLAG_HIDDEN);
    }
                          
    // Solar
    int solar_w = (int)state.solar_power;
    int s_max_upd = (int)state.solar_max_w;
    if (s_max_upd <= 0) s_max_upd = 200;
    lv_arc_set_range(arc_solar, 0, s_max_upd);
    
    if (solar_w != last_solar) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, arc_solar);
        lv_anim_set_exec_cb(&a, set_arc_val_cb);
        lv_anim_set_time(&a, 1000);
        lv_anim_set_values(&a, lv_arc_get_value(arc_solar), solar_w);
        lv_anim_start(&a);
        last_solar = solar_w;
        lv_label_set_text_fmt(lbl_solar_watt, "%d W", solar_w);
    }
    ui_label_set_float(lbl_solar_curr, "%.1f A", state.solar_current);

    // ==========================================
    // Visual Energy Flow Calculation & Update
    // ==========================================
    if (lbl_flow_solar_val) {
        float p_solar = state.solar_power;
        float p_bat = state.bat_voltage * state.bat_current;
        float p_load = p_solar - p_bat;
        if (p_load < 0.0f) p_load = 0.0f;

        // Solar node
        ui_label_set_float(lbl_flow_solar_val, "%.0f W", p_solar);

        // Arrow 1 & Battery node
        if (p_bat > 1.5f) {
            ui_label_set_float(lbl_flow_bat_val, "+%.0f W", p_bat);
            lv_obj_set_style_text_color(lbl_flow_bat_val, lv_color_hex(UI_COLOR_SUCCESS), 0);
            lv_label_set_text(lbl_flow_arr1, ">");
            lv_obj_set_style_text_color(lbl_flow_arr1, lv_color_hex(UI_COLOR_SUCCESS), 0);
        } else if (p_bat < -1.5f) {
            ui_label_set_float(lbl_flow_bat_val, "%.0f W", p_bat);
            lv_obj_set_style_text_color(lbl_flow_bat_val, lv_color_hex(UI_COLOR_WARNING), 0);
            lv_label_set_text(lbl_flow_arr1, "-");
            lv_obj_set_style_text_color(lbl_flow_arr1, ui_theme_muted(), 0);
        } else {
            lv_label_set_text(lbl_flow_bat_val, "0 W");
            lv_obj_set_style_text_color(lbl_flow_bat_val, ui_theme_muted(), 0);
            lv_label_set_text(lbl_flow_arr1, "-");
            lv_obj_set_style_text_color(lbl_flow_arr1, ui_theme_muted(), 0);
        }

        // Arrow 2 & Load node
        if (p_load > 1.5f) {
            lv_label_set_text(lbl_flow_arr2, ">");
            lv_obj_set_style_text_color(lbl_flow_arr2, lv_color_hex(UI_COLOR_PRIMARY), 0);
        } else {
            lv_label_set_text(lbl_flow_arr2, "-");
            lv_obj_set_style_text_color(lbl_flow_arr2, ui_theme_muted(), 0);
        }
        ui_label_set_float(lbl_flow_load_val, "%.0f W", p_load);
    }
}

void ui_build_power(lv_obj_t *parent) {
    ui_setup_tab_page(parent);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // 1. Battery Card (Top: Height 144px)
    // ==========================================
    lv_obj_t *card_bat = ui_create_card(parent, 440, 144);
    lv_obj_align(card_bat, LV_ALIGN_TOP_MID, 0, 0);

    // Battery Arc Gauge (Left)
    arc_battery = lv_arc_create(card_bat);
    lv_obj_set_size(arc_battery, 118, 118);
    lv_arc_set_rotation(arc_battery, 135);
    lv_arc_set_bg_angles(arc_battery, 0, 270);
    lv_arc_set_value(arc_battery, 0);
    lv_obj_clear_flag(arc_battery, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(arc_battery, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_set_style_arc_color(arc_battery, lv_color_hex(UI_COLOR_SUCCESS), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_battery, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_battery, ui_theme_track(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_battery, 12, LV_PART_MAIN);
    lv_obj_remove_style(arc_battery, NULL, LV_PART_KNOB);

    lbl_bat_pct = lv_label_create(arc_battery);
    lv_obj_set_style_text_font(lbl_bat_pct, &lv_font_montserrat_28, 0);
    lv_label_set_text(lbl_bat_pct, "0%");
    lv_obj_center(lbl_bat_pct);

    // Battery Info (Right)
    lv_obj_t *l_battitle = lv_label_create(card_bat);
    lv_label_set_text_fmt(l_battitle, "%s Batterie", LV_SYMBOL_BATTERY_3);
    lv_obj_set_style_text_font(l_battitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l_battitle, ui_theme_muted(), 0);
    lv_obj_align(l_battitle, LV_ALIGN_TOP_LEFT, 145, 4);

    lbl_bat_volt = lv_label_create(card_bat);
    lv_obj_set_style_text_font(lbl_bat_volt, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_bat_volt, ui_theme_text(), 0);
    lv_label_set_text(lbl_bat_volt, "-- V");
    lv_obj_align(lbl_bat_volt, LV_ALIGN_TOP_LEFT, 145, 28);

    lbl_bat_curr = lv_label_create(card_bat);
    lv_obj_set_style_text_font(lbl_bat_curr, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_bat_curr, ui_theme_muted(), 0);
    lv_label_set_text(lbl_bat_curr, "-- A");
    lv_obj_align(lbl_bat_curr, LV_ALIGN_TOP_LEFT, 145, 62);

    lbl_bat_ttg = lv_label_create(card_bat);
    lv_obj_set_style_text_font(lbl_bat_ttg, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_bat_ttg, ui_theme_muted(), 0);
    lv_label_set_text(lbl_bat_ttg, "Standby");
    lv_obj_align(lbl_bat_ttg, LV_ALIGN_TOP_LEFT, 145, 94);

    lbl_bat_charge_icon = lv_label_create(card_bat);
    lv_label_set_text(lbl_bat_charge_icon, LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_font(lbl_bat_charge_icon, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_bat_charge_icon, lv_color_hex(UI_COLOR_SUCCESS), 0);
    lv_obj_align(lbl_bat_charge_icon, LV_ALIGN_TOP_RIGHT, 0, 4);
    lv_obj_add_flag(lbl_bat_charge_icon, LV_OBJ_FLAG_HIDDEN);

    // ==========================================
    // 2. Visual Energy Flow Bar (Middle: Height 56px)
    // ==========================================
    lv_obj_t *card_flow = ui_create_card(parent, 440, 56);
    lv_obj_align(card_flow, LV_ALIGN_TOP_MID, 0, 152);
    lv_obj_set_style_pad_all(card_flow, 6, 0);

    // Helper for creating a flow node
    auto create_flow_node = [card_flow](const char *title, int x) -> lv_obj_t* {
        lv_obj_t *lbl_t = lv_label_create(card_flow);
        lv_label_set_text(lbl_t, title);
        lv_obj_set_style_text_font(lbl_t, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(lbl_t, ui_theme_muted(), 0);
        lv_obj_align(lbl_t, LV_ALIGN_TOP_LEFT, x, 2);

        lv_obj_t *lbl_v = lv_label_create(card_flow);
        lv_label_set_text(lbl_v, "-- W");
        lv_obj_set_style_text_font(lbl_v, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl_v, ui_theme_text(), 0);
        lv_obj_align(lbl_v, LV_ALIGN_TOP_LEFT, x, 22);
        return lbl_v;
    };

    // Node 1: Solar
    lbl_flow_solar_val = create_flow_node("Solar", 14);

    // Arrow 1
    lbl_flow_arr1 = lv_label_create(card_flow);
    lv_label_set_text(lbl_flow_arr1, ">");
    lv_obj_set_style_text_font(lbl_flow_arr1, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_flow_arr1, ui_theme_muted(), 0);
    lv_obj_align(lbl_flow_arr1, LV_ALIGN_CENTER, -65, 2);

    // Node 2: Battery Net
    lbl_flow_bat_val = create_flow_node("Batterie", 165);

    // Arrow 2
    lbl_flow_arr2 = lv_label_create(card_flow);
    lv_label_set_text(lbl_flow_arr2, ">");
    lv_obj_set_style_text_font(lbl_flow_arr2, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_flow_arr2, ui_theme_muted(), 0);
    lv_obj_align(lbl_flow_arr2, LV_ALIGN_CENTER, 75, 2);

    // Node 3: Load (Consumers)
    lbl_flow_load_val = create_flow_node("Verbrauch", 315);

    // ==========================================
    // 3. Solar Card (Bottom: Height 144px)
    // ==========================================
    lv_obj_t *card_solar = ui_create_card(parent, 440, 144);
    lv_obj_align(card_solar, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Solar Arc Gauge (Left)
    arc_solar = lv_arc_create(card_solar);
    lv_obj_set_size(arc_solar, 118, 118);
    lv_arc_set_rotation(arc_solar, 135);
    lv_arc_set_bg_angles(arc_solar, 0, 270);
    int s_max = (int)state.solar_max_w;
    if (s_max <= 0) s_max = 200;
    lv_arc_set_range(arc_solar, 0, s_max);
    lv_arc_set_value(arc_solar, 0);
    lv_obj_clear_flag(arc_solar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(arc_solar, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_set_style_arc_color(arc_solar, lv_color_hex(UI_COLOR_WARNING), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_solar, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc_solar, ui_theme_track(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_solar, 12, LV_PART_MAIN);
    lv_obj_remove_style(arc_solar, NULL, LV_PART_KNOB);

    lbl_solar_watt = lv_label_create(arc_solar);
    lv_obj_set_style_text_font(lbl_solar_watt, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_solar_watt, lv_color_hex(UI_COLOR_WARNING), 0);
    lv_label_set_text(lbl_solar_watt, "0 W");
    lv_obj_center(lbl_solar_watt);

    // Solar Info (Right)
    lv_obj_t *l_soltitle = lv_label_create(card_solar);
    lv_label_set_text_fmt(l_soltitle, "%s Solaranlage", LV_SYMBOL_CHARGE);
    lv_obj_set_style_text_font(l_soltitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l_soltitle, ui_theme_muted(), 0);
    lv_obj_align(l_soltitle, LV_ALIGN_TOP_LEFT, 145, 8);

    lbl_solar_curr = lv_label_create(card_solar);
    lv_obj_set_style_text_font(lbl_solar_curr, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_solar_curr, lv_color_hex(UI_COLOR_WARNING), 0);
    lv_label_set_text(lbl_solar_curr, "-- A");
    lv_obj_align(lbl_solar_curr, LV_ALIGN_TOP_LEFT, 145, 38);

    lv_obj_t *lbl_sol_max = lv_label_create(card_solar);
    lv_obj_set_style_text_font(lbl_sol_max, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_sol_max, ui_theme_muted(), 0);
    lv_label_set_text_fmt(lbl_sol_max, "Max: %d W", s_max);
    lv_obj_align(lbl_sol_max, LV_ALIGN_TOP_LEFT, 145, 86);
}
