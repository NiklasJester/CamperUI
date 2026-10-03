#include "ui_main.h"

void ui_build_home(lv_obj_t *parent);

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
static lv_obj_t *lbl_maxxfan_status;

// Smart Header Badges
static lv_obj_t *badge_cont;
static lv_obj_t *badge_frost;
static lv_obj_t *badge_bat;
static lv_obj_t *badge_fresh;
static lv_obj_t *badge_waste;

static constexpr uint8_t PAGE_COUNT = 9;
static lv_obj_t *content;
static lv_obj_t *pages[PAGE_COUNT] = {};
static uint8_t active_page = 0;
static bool rebuild_pending = false;
static lv_obj_t *home_nav;
static lv_obj_t *home_nav_buttons[PAGE_COUNT] = {};

static void update_home_nav() {
    uint16_t active = active_page;
    for (int i = 0; i < PAGE_COUNT; ++i) {
        if (!home_nav_buttons[i]) continue;
        if (i == active) lv_obj_add_state(home_nav_buttons[i], LV_STATE_CHECKED);
        else lv_obj_clear_state(home_nav_buttons[i], LV_STATE_CHECKED);
    }
    if (active < PAGE_COUNT && home_nav_buttons[active])
        lv_obj_scroll_to_view(home_nav_buttons[active], LV_ANIM_ON);
}

static void select_page(uint8_t index) {
    if (index >= PAGE_COUNT || !pages[index]) return;
    for (uint8_t i = 0; i < PAGE_COUNT; ++i) {
        if (i == index) lv_obj_clear_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
    }
    active_page = index;
    if (index == 2) ui_trigger_power_anim();
    else if (index == 3) ui_trigger_water_anim();
    update_home_nav();
}

static void home_nav_clicked(lv_event_t *event) {
    select_page((uint8_t)(uintptr_t)lv_event_get_user_data(event));
}

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

    lv_obj_set_style_bg_color(scr_main, bg_color, 0);
    if (content) lv_obj_set_style_bg_color(content, bg_color, 0);
    if (home_nav) {
        lv_obj_set_style_bg_color(home_nav, ui_theme_card(), 0);
        for (int i = 0; i < PAGE_COUNT; ++i) {
            lv_obj_set_style_bg_color(home_nav_buttons[i], ui_theme_card(), 0);
            lv_obj_set_style_text_color(home_nav_buttons[i], ui_theme_muted(), 0);
        }
    }
}

// Rebuild outside an LVGL event callback, releasing the old screens first.
static void rebuild_ui(void *) {
    rebuild_pending = false;
    lv_obj_t *old_main = scr_main;
    lv_obj_t *old_settings = scr_settings;
    lv_obj_t *temporary = lv_obj_create(nullptr);
    lv_scr_load(temporary);
    if (old_main) lv_obj_del(old_main);
    if (old_settings) lv_obj_del(old_settings);
    scr_main = scr_settings = nullptr;
    lbl_debug_info = nullptr;
    ui_init();
    lv_obj_del(temporary);
}

void ui_init() {
    if (scr_main) {
        if (!rebuild_pending) {
            rebuild_pending = true;
            lv_async_call(rebuild_ui, nullptr);
        }
        return;
    }
    content = home_nav = nullptr;
    for (uint8_t i = 0; i < PAGE_COUNT; ++i) {
        pages[i] = home_nav_buttons[i] = nullptr;
    }
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
    lv_obj_set_size(scr_main, 480, 480);
    lv_obj_set_style_pad_all(scr_main, 0, 0);
    lv_obj_set_style_border_width(scr_main, 0, 0);
    lv_obj_set_style_bg_color(scr_main, ui_theme_bg(), 0);

    // Initialize Settings Screen
    ui_settings_screen_init();

    // 1. Status Bar (Height = 40px)
    status_bar = lv_obj_create(scr_main);
    lv_obj_set_size(status_bar, 480, 40);
    lv_obj_set_style_pad_all(status_bar, 0, 0);
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

    lbl_maxxfan_status = lv_label_create(status_bar);
    lv_label_set_text(lbl_maxxfan_status, MDI_FAN);
    lv_obj_set_style_text_font(lbl_maxxfan_status, &ui_font_mdi_18, 0);
    lv_obj_set_style_text_color(lbl_maxxfan_status, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_align(lbl_maxxfan_status, LV_ALIGN_RIGHT_MID, -212, 0);
    lv_obj_add_flag(lbl_maxxfan_status, LV_OBJ_FLAG_HIDDEN);

    lbl_pump_status = lv_label_create(status_bar);
    lv_label_set_text(lbl_pump_status, MDI_PUMP);
    lv_obj_set_style_text_font(lbl_pump_status, &ui_font_mdi_18, 0);
    lv_obj_align(lbl_pump_status, LV_ALIGN_RIGHT_MID, -238, 0);
    lv_obj_set_style_text_color(lbl_pump_status, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_add_flag(lbl_pump_status, LV_OBJ_FLAG_HIDDEN);

    // One fixed viewport. Pages cannot move or recreate the global bars.
    content = lv_obj_create(scr_main);
    lv_obj_set_pos(content, 0, 40);
    lv_obj_set_size(content, 480, 380);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(content, ui_theme_bg(), 0);
    for (uint8_t i = 0; i < PAGE_COUNT; ++i) {
        pages[i] = lv_obj_create(content);
        lv_obj_set_pos(pages[i], 0, 0);
        lv_obj_set_size(pages[i], 480, 380);
        lv_obj_set_style_radius(pages[i], 0, 0);
        lv_obj_set_style_border_width(pages[i], 0, 0);
        lv_obj_set_style_bg_opa(pages[i], LV_OPA_TRANSP, 0);
        lv_obj_set_scroll_dir(pages[i], LV_DIR_VER);
        lv_obj_clear_flag(pages[i], LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
        lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_t *t_home = pages[0], *t_dim = pages[1], *t1 = pages[2];
    lv_obj_t *t2 = pages[3], *t3 = pages[4], *t_fan = pages[5];
    lv_obj_t *t_sw = pages[6], *t_lvl = pages[7], *t5 = pages[8];

    ui_setup_tab_page(t_dim);
    ui_setup_tab_page(t1);
    ui_setup_tab_page(t2);
    ui_setup_tab_page(t3);
    ui_setup_tab_page(t_fan);
    ui_setup_tab_page(t_sw);
    ui_setup_tab_page(t_lvl);
    ui_setup_tab_page(t5);

    ui_build_home(t_home);
    ui_build_dimmers(t_dim);
    ui_build_power(t1);
    ui_build_water(t2);
    ui_build_climate(t3);
    ui_build_maxxfan(t_fan);
    ui_build_switches(t_sw);
    ui_build_level(t_lvl);
    ui_build_settings(t5);

    // The only navigation strip: a fixed sibling of status bar and content.
    home_nav = lv_obj_create(scr_main);
    lv_obj_set_pos(home_nav, 0, 420);
    lv_obj_set_size(home_nav, 480, 60);
    lv_obj_add_flag(home_nav, LV_OBJ_FLAG_FLOATING);
    lv_obj_set_style_pad_all(home_nav, 3, 0);
    lv_obj_set_style_pad_column(home_nav, 4, 0);
    lv_obj_set_style_border_width(home_nav, 0, 0);
    lv_obj_set_style_radius(home_nav, 0, 0);
    lv_obj_set_style_bg_color(home_nav, ui_theme_card(), 0);
    lv_obj_set_flex_flow(home_nav, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(home_nav, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_dir(home_nav, LV_DIR_HOR);
    lv_obj_set_scrollbar_mode(home_nav, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_clear_flag(home_nav, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_clear_flag(home_nav, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    const char *nav_icons[9] = {MDI_HOME, MDI_LIGHTBULB, MDI_BATTERY_CHARGING,
        MDI_WATER, MDI_THERMOMETER, MDI_FAN, MDI_TOGGLE_SWITCH, MDI_SPIRIT_LEVEL, MDI_TUNE};
    for (uintptr_t i = 0; i < PAGE_COUNT; ++i) {
        lv_obj_t *button = lv_btn_create(home_nav);
        home_nav_buttons[i] = button;
        lv_obj_set_size(button, 68, 48);
        lv_obj_set_style_pad_all(button, 0, 0);
        lv_obj_set_style_radius(button, 10, 0);
        lv_obj_set_style_shadow_width(button, 0, 0);
        lv_obj_set_style_bg_color(button, ui_theme_card(), 0);
        lv_obj_set_style_bg_color(button, lv_color_hex(0x263b52), LV_STATE_CHECKED);
        lv_obj_set_style_text_color(button, ui_theme_muted(), 0);
        lv_obj_set_style_text_color(button, lv_color_hex(0xb8ccfa), LV_STATE_CHECKED);
        lv_obj_add_event_cb(button, home_nav_clicked, LV_EVENT_CLICKED, (void *)i);
        lv_obj_t *icon = lv_label_create(button);
        lv_label_set_text(icon, nav_icons[i]);
        lv_obj_set_style_text_font(icon, &ui_font_mdi_32, 0);
        lv_obj_center(icon);
    }
    select_page(active_page);
    lv_obj_move_foreground(status_bar);
    lv_obj_move_foreground(home_nav);
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

    if (lbl_maxxfan_status) {
        if (ui_maxxfan_running()) lv_obj_clear_flag(lbl_maxxfan_status, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(lbl_maxxfan_status, LV_OBJ_FLAG_HIDDEN);
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
