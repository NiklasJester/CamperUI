#include "ui_main.h"

lv_obj_t *scr_main;
lv_obj_t *scr_settings;

static lv_obj_t *status_bar;
static lv_obj_t *lbl_time;
static lv_obj_t *lbl_wifi;
static lv_obj_t *lbl_soc_icon;
static lv_obj_t *lbl_soc;
static lv_obj_t *lbl_temp;
static lv_obj_t *lbl_pump_status;
static lv_obj_t *lbl_heater_status;

// Smart Header Badges
static lv_obj_t *badge_cont;
static lv_obj_t *badge_frost;
static lv_obj_t *badge_bat;
static lv_obj_t *badge_fresh;
static lv_obj_t *badge_waste;

static lv_obj_t *tabview;

// Helper to configure consistent padding across all tab pages
void ui_setup_tab_page(lv_obj_t *page) {
    if (!page) return;
    lv_obj_set_style_pad_left(page, 20, 0);
    lv_obj_set_style_pad_right(page, 20, 0);
    lv_obj_set_style_pad_top(page, 10, 0);
    lv_obj_set_style_pad_bottom(page, 10, 0);
    lv_obj_set_style_bg_opa(page, 0, 0);
    lv_obj_set_style_border_width(page, 0, 0);
}

// Helper to create a styled modern card
lv_obj_t* ui_create_card(lv_obj_t *parent, int w, int h) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    
    // Subtle modern shadow
    lv_obj_set_style_shadow_width(card, 12, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(card, state.dark_mode ? LV_OPA_40 : LV_OPA_10, 0);
    lv_obj_set_style_shadow_ofs_y(card, 4, 0);
    
    // Theme colors
    lv_obj_set_style_bg_color(card, ui_theme_card(), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, ui_theme_border(), 0);
    
    lv_obj_set_style_radius(card, 14, 0);
    lv_obj_set_style_pad_all(card, 12, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

void ui_apply_theme() {
    lv_disp_t * disp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(disp, 
                                               lv_color_hex(UI_COLOR_PRIMARY), 
                                               lv_color_hex(UI_COLOR_DANGER), 
                                               state.dark_mode, 
                                               &lv_font_montserrat_14);
    lv_disp_set_theme(disp, theme);

    lv_obj_t *scr = lv_scr_act();
    lv_color_t bg_color = ui_theme_bg();
    lv_obj_set_style_bg_color(scr, bg_color, 0);
    
    // Refresh status bar
    lv_color_t bar_bg = state.dark_mode ? lv_color_hex(0x181c23) : lv_color_hex(0xe9edf3);
    lv_obj_set_style_bg_color(status_bar, bar_bg, 0);
    lv_obj_set_style_border_color(status_bar, ui_theme_border(), 0);
    lv_obj_set_style_text_color(status_bar, ui_theme_text(), 0);

    // Tabview styling
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_bg_color(tab_btns, bar_bg, 0);
    lv_obj_set_style_border_color(tab_btns, ui_theme_border(), 0);
    lv_obj_set_style_border_width(tab_btns, 1, 0);
    lv_obj_set_style_border_side(tab_btns, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_text_color(tab_btns, ui_theme_muted(), 0);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(UI_COLOR_PRIMARY), LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(tabview, bg_color, 0);
}

static void tab_switch_cb(lv_event_t * e) {
    lv_obj_t * tv = lv_event_get_target(e);
    uint16_t tab = lv_tabview_get_tab_act(tv);
    // Tab 0: Dimmer, Tab 1: Power, Tab 2: Water, Tab 3: Climate, Tab 4: Switches, Tab 5: Level, Tab 6: Settings
    if (tab == 1) ui_trigger_power_anim();
    else if (tab == 2) ui_trigger_water_anim();
}

void ui_init() {
    lv_disp_t * disp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_default_init(disp, 
                                               lv_color_hex(UI_COLOR_PRIMARY), 
                                               lv_color_hex(UI_COLOR_DANGER), 
                                               state.dark_mode, 
                                               &lv_font_montserrat_14);
    lv_disp_set_theme(disp, theme);
    
    // Create Main Screen
    scr_main = lv_obj_create(NULL);
    lv_obj_clear_flag(scr_main, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr_main, ui_theme_bg(), 0);

    // Initialize Settings Screen
    ui_settings_screen_init();

    // 1. Status Bar (Height = 40px)
    status_bar = lv_obj_create(scr_main);
    lv_obj_set_size(status_bar, 480, 40);
    lv_obj_align(status_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_set_style_border_width(status_bar, 1, 0);
    lv_obj_set_style_border_side(status_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    
    // Left: Time
    lbl_time = lv_label_create(status_bar);
    lv_label_set_text(lbl_time, "12:00");
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_16, 0);
    lv_obj_align(lbl_time, LV_ALIGN_LEFT_MID, 16, 0);

    // Smart Header Badges Container (Middle)
    badge_cont = lv_obj_create(status_bar);
    lv_obj_set_size(badge_cont, 130, 26);
    lv_obj_align(badge_cont, LV_ALIGN_LEFT_MID, 68, 0);
    lv_obj_set_style_bg_opa(badge_cont, LV_OPA_0, 0);
    lv_obj_set_style_border_width(badge_cont, 0, 0);
    lv_obj_set_style_pad_all(badge_cont, 0, 0);
    lv_obj_set_flex_flow(badge_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(badge_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(badge_cont, 4, 0);
    lv_obj_clear_flag(badge_cont, LV_OBJ_FLAG_SCROLLABLE);

    auto make_badge = [](lv_obj_t *parent, const char *sym, lv_color_t bg_c, lv_color_t text_c) -> lv_obj_t* {
        lv_obj_t *b = lv_label_create(parent);
        lv_label_set_text(b, sym);
        lv_obj_set_style_text_font(b, &ui_font_mdi_18, 0);
        lv_obj_set_style_text_color(b, text_c, 0);
        lv_obj_set_style_bg_color(b, bg_c, 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_hor(b, 4, 0);
        lv_obj_set_style_pad_ver(b, 2, 0);
        lv_obj_set_style_radius(b, 4, 0);
        lv_obj_add_flag(b, LV_OBJ_FLAG_HIDDEN);
        return b;
    };

    badge_frost = make_badge(badge_cont, MDI_SNOWFLAKE, lv_color_hex(0x1a2e45), lv_color_hex(UI_COLOR_PRIMARY));
    badge_bat   = make_badge(badge_cont, MDI_BATTERY,   lv_color_hex(0x422f00), lv_color_hex(UI_COLOR_WARNING));
    badge_fresh = make_badge(badge_cont, MDI_WATER,     lv_color_hex(0x122e4d), lv_color_hex(UI_COLOR_PRIMARY));
    badge_waste = make_badge(badge_cont, MDI_ALERT,     lv_color_hex(0x451a1a), lv_color_hex(UI_COLOR_DANGER));

    // Right side indicators (cleanly spaced, modern MDI icons)
    lbl_wifi = lv_label_create(status_bar);
    lv_label_set_text(lbl_wifi, MDI_WIFI);
    lv_obj_set_style_text_font(lbl_wifi, &ui_font_mdi_18, 0);
    lv_obj_align(lbl_wifi, LV_ALIGN_RIGHT_MID, -16, 0);
    
    lbl_soc = lv_label_create(status_bar);
    lv_label_set_text(lbl_soc, "--%");
    lv_obj_set_style_text_font(lbl_soc, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_soc, LV_ALIGN_RIGHT_MID, -40, 0);

    lbl_soc_icon = lv_label_create(status_bar);
    lv_label_set_text(lbl_soc_icon, MDI_BATTERY);
    lv_obj_set_style_text_font(lbl_soc_icon, &ui_font_mdi_18, 0);
    lv_obj_align(lbl_soc_icon, LV_ALIGN_RIGHT_MID, -78, 0);

    lbl_temp = lv_label_create(status_bar);
    lv_label_set_text(lbl_temp, "IN: -- C");
    lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_14, 0);
    lv_obj_align(lbl_temp, LV_ALIGN_RIGHT_MID, -106, 0);

    lbl_heater_status = lv_label_create(status_bar);
    lv_label_set_text(lbl_heater_status, MDI_FIRE);
    lv_obj_set_style_text_font(lbl_heater_status, &ui_font_mdi_18, 0);
    lv_obj_set_style_text_color(lbl_heater_status, lv_color_hex(UI_COLOR_DANGER), 0);
    lv_obj_align(lbl_heater_status, LV_ALIGN_RIGHT_MID, -186, 0);
    lv_obj_add_flag(lbl_heater_status, LV_OBJ_FLAG_HIDDEN);

    lbl_pump_status = lv_label_create(status_bar);
    lv_label_set_text(lbl_pump_status, MDI_PUMP);
    lv_obj_set_style_text_font(lbl_pump_status, &ui_font_mdi_18, 0);
    lv_obj_align(lbl_pump_status, LV_ALIGN_RIGHT_MID, -212, 0);
    lv_obj_set_style_text_color(lbl_pump_status, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_add_flag(lbl_pump_status, LV_OBJ_FLAG_HIDDEN);

    // 2. Tabview
    tabview = lv_tabview_create(scr_main, LV_DIR_BOTTOM, 60);
    lv_obj_clear_flag(tabview, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(tabview, 480, 440);
    lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(tabview, tab_switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Tab bar font: Material Design Icons 32px
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tabview);
    lv_obj_set_style_text_font(tab_btns, &ui_font_mdi_32, 0);

    lv_obj_t *t_dim = lv_tabview_add_tab(tabview, MDI_LIGHTBULB);        // Tab 0: Dimmer / Licht
    lv_obj_t *t1    = lv_tabview_add_tab(tabview, MDI_BATTERY_CHARGING); // Tab 1: Power
    lv_obj_t *t2    = lv_tabview_add_tab(tabview, MDI_WATER);            // Tab 2: Wasser
    lv_obj_t *t3    = lv_tabview_add_tab(tabview, MDI_THERMOMETER);      // Tab 3: Klima
    lv_obj_t *t_sw  = lv_tabview_add_tab(tabview, MDI_TOGGLE_SWITCH);   // Tab 4: Relais / Schalter
    lv_obj_t *t_lvl = lv_tabview_add_tab(tabview, MDI_SPIRIT_LEVEL);    // Tab 5: Wasserwaage / Level
    lv_obj_t *t5    = lv_tabview_add_tab(tabview, MDI_TUNE);            // Tab 6: Setup

    ui_setup_tab_page(t_dim);
    ui_setup_tab_page(t1);
    ui_setup_tab_page(t2);
    ui_setup_tab_page(t3);
    ui_setup_tab_page(t_sw);
    ui_setup_tab_page(t_lvl);
    ui_setup_tab_page(t5);

    ui_build_dimmers(t_dim);
    ui_build_power(t1);
    ui_build_water(t2);
    ui_build_climate(t3);
    ui_build_switches(t_sw);
    ui_build_level(t_lvl);
    ui_build_settings(t5);

    ui_apply_theme();
    lv_scr_load(scr_main);
}

void ui_update_data() {
    // Update Status Bar
    if (state.wifi_connected) {
        lv_obj_clear_flag(lbl_wifi, LV_OBJ_FLAG_HIDDEN);
        if (state.wifi_rssi > -60) lv_obj_set_style_text_color(lbl_wifi, lv_color_hex(UI_COLOR_SUCCESS), 0);
        else if (state.wifi_rssi > -80) lv_obj_set_style_text_color(lbl_wifi, lv_color_hex(UI_COLOR_WARNING), 0);
        else lv_obj_set_style_text_color(lbl_wifi, lv_color_hex(UI_COLOR_DANGER), 0);
    } else {
        lv_obj_add_flag(lbl_wifi, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text_fmt(lbl_soc, "%d%%", state.bat_soc);
    if (state.battery_icon_mode) {
        lv_obj_clear_flag(lbl_soc_icon, LV_OBJ_FLAG_HIDDEN);
        if (state.bat_soc <= state.warn_bat_soc) {
            lv_obj_set_style_text_color(lbl_soc_icon, lv_color_hex(UI_COLOR_DANGER), 0);
        } else if (state.bat_soc <= 40) {
            lv_obj_set_style_text_color(lbl_soc_icon, lv_color_hex(UI_COLOR_WARNING), 0);
        } else {
            lv_obj_set_style_text_color(lbl_soc_icon, lv_color_hex(UI_COLOR_SUCCESS), 0);
        }
    } else {
        lv_obj_add_flag(lbl_soc_icon, LV_OBJ_FLAG_HIDDEN);
    }

    ui_label_set_float(lbl_temp, "IN: %.1f C", state.indoor_temp);
    
    if (lbl_pump_status) {
        if (state.pump_relay >= 0 && state.pump_relay < 8 && state.switch_state[state.pump_relay]) {
            lv_obj_clear_flag(lbl_pump_status, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(lbl_pump_status, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (lbl_heater_status) {
        if (state.heating_on) {
            lv_obj_clear_flag(lbl_heater_status, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(lbl_heater_status, LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Smart Header Badges Evaluation
    if (badge_frost) {
        if (state.outdoor_temp <= state.warn_frost_temp && state.outdoor_temp > -50.0f) {
            lv_obj_clear_flag(badge_frost, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(badge_frost, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (badge_bat) {
        if (state.bat_soc > 0 && state.bat_soc <= state.warn_bat_soc) {
            lv_obj_clear_flag(badge_bat, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(badge_bat, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (badge_fresh) {
        if (state.tank_level[0] > 0 && state.tank_level[0] <= state.warn_fresh_min) {
            lv_obj_clear_flag(badge_fresh, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(badge_fresh, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (badge_waste) {
        if (state.tank_level[1] >= state.warn_waste_max) {
            lv_obj_clear_flag(badge_waste, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(badge_waste, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    // Update Debug Info
    if (lbl_debug_info != NULL) {
        String debug_txt = "WLAN: ";
        if (state.wifi_connected) debug_txt += "Verbunden (" + String(state.wifi_rssi) + " dBm)\n";
        else debug_txt += "Getrennt (Suche '" + state.wifi_ssid + "')\n";
        
        lv_label_set_text(lbl_debug_info, debug_txt.c_str());
    }
}
