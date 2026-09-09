#include "ui_main.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

static lv_obj_t * http_test_win = NULL;
static lv_obj_t * http_lbl_result = NULL;

static void close_http_test_cb(lv_event_t * e) {
    if (http_test_win) {
        lv_obj_del(http_test_win);
        http_test_win = NULL;
    }
}

static void run_http_fetch(lv_event_t * e) {
    if(!http_lbl_result) return;
    
    if(WiFi.status() != WL_CONNECTED) {
        lv_label_set_text(http_lbl_result, "WLAN nicht verbunden!");
        return;
    }
    
    lv_label_set_text(http_lbl_result, "Lade Daten von Port 1880...");
    lv_timer_handler(); // force redraw
    
    String baseUrl = "http://" + state.vanpi_ip + ":1880";
    String result = "";
    
    HTTPClient http;
    const char* endpoints[] = {"/batt", "/relay", "/wrelay", "/dimmer", "/names"};
    
    for(int i=0; i<5; i++) {
        String url = baseUrl + endpoints[i];
        http.begin(url);
        int code = http.GET();
        result += "=== " + String(endpoints[i]) + " ===\n";
        if(code > 0) {
            result += "Code " + String(code) + ":\n";
            result += http.getString() + "\n\n";
        } else {
            result += "Fehler: " + http.errorToString(code) + "\n\n";
        }
        http.end();
    }
    
    lv_label_set_text(http_lbl_result, result.c_str());
}

static void open_http_test_cb(lv_event_t * e) {
    http_test_win = lv_obj_create(lv_scr_act());
    lv_obj_set_size(http_test_win, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(http_test_win, state.dark_mode ? lv_color_hex(0x1e1e1e) : lv_color_hex(0xf0f0f0), 0);
    
    lv_obj_t * btn_close = lv_btn_create(http_test_win);
    lv_obj_align(btn_close, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_t * lbl_c = lv_label_create(btn_close);
    lv_label_set_text(lbl_c, "X");
    lv_obj_add_event_cb(btn_close, close_http_test_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t * btn_run = lv_btn_create(http_test_win);
    lv_obj_align(btn_run, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * lbl_r = lv_label_create(btn_run);
    lv_label_set_text(lbl_r, "Laden");
    lv_obj_add_event_cb(btn_run, run_http_fetch, LV_EVENT_CLICKED, NULL);
    
    http_lbl_result = lv_label_create(http_test_win);
    lv_obj_set_width(http_lbl_result, 440);
    lv_label_set_long_mode(http_lbl_result, LV_LABEL_LONG_WRAP);
    lv_obj_align(http_lbl_result, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_label_set_text(http_lbl_result, "Druecke Laden...");
}

#include <WiFi.h>
#include <Wire.h>
#include "WS_CH32_IO.h"

extern TwoWire Wire;

static lv_obj_t *dd_wifi_ssid;
static lv_obj_t *btn_wifi_scan;
static lv_obj_t *lbl_wifi_scan;
static lv_obj_t *ta_wifi_pass;
static lv_obj_t *ta_mqtt_ip;



static lv_obj_t *kb;

static lv_obj_t *dd_rename_item;
static lv_obj_t *ta_rename;

static lv_obj_t *ta_bat_cap;
static lv_obj_t *ta_solar_max;
static lv_obj_t *ta_tank_max[4];

static lv_obj_t *ta_warn_frost;
static lv_obj_t *ta_warn_bat;
static lv_obj_t *ta_warn_fresh;
static lv_obj_t *ta_warn_waste;

lv_obj_t *lbl_debug_info = NULL;

static bool scan_in_progress = false;

void ui_update_settings_tab() {
    if (scan_in_progress) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            scan_in_progress = false;
            String options = "";
            for (int i = 0; i < n; ++i) {
                options += WiFi.SSID(i);
                if (i < n - 1) options += "\n";
            }
            if(n == 0) options = "Keine Netzwerke gefunden";
            lv_dropdown_set_options(dd_wifi_ssid, options.c_str());
            lv_label_set_text(lbl_wifi_scan, "Scan");
            WiFi.scanDelete();
        } else if (n == WIFI_SCAN_FAILED) {
            scan_in_progress = false;
            lv_label_set_text(lbl_wifi_scan, "Scan Fehler");
        }
    }
}

static void ta_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    if(code == LV_EVENT_FOCUSED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(kb); // Bring keyboard to front
    }
    else if(code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static void wifi_scan_cb(lv_event_t * e) {
    if(!scan_in_progress) {
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.disconnect();
            delay(50);
        }
        lv_label_set_text(lbl_wifi_scan, "Scanne...");
        WiFi.scanNetworks(true); // async
        scan_in_progress = true;
    }
}

static void save_settings_cb(lv_event_t * e) {
    char buf[64];
    lv_dropdown_get_selected_str(dd_wifi_ssid, buf, sizeof(buf));
    state.wifi_ssid = String(buf);
    state.wifi_pass = String(lv_textarea_get_text(ta_wifi_pass));
    state.vanpi_ip = String(lv_textarea_get_text(ta_mqtt_ip));
    
            
    state_save();
    
    static const char * btns[] = {"OK", ""};
    lv_obj_t * mbox = lv_msgbox_create(NULL, "Gespeichert", "Einstellungen gespeichert.\nWechsle zum Hauptmenue...", btns, true);
    lv_obj_center(mbox);
    
    // Attempt reconnects
    WiFi.disconnect();
    WiFi.begin(state.wifi_ssid.c_str(), state.wifi_pass.c_str());
    
    // Load main screen
    lv_scr_load(scr_main);
}

static void open_settings_cb(lv_event_t * e) {
    lv_scr_load(scr_settings);
}

// ==========================================
// 5th Tab on Main Screen
// ==========================================
void ui_build_settings(lv_obj_t *parent) {
    ui_setup_tab_page(parent);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(parent, 20, 0);
    
    // Modern Action Button
    lv_obj_t *btn_open = lv_btn_create(parent);
    lv_obj_set_size(btn_open, 320, 64);
    lv_obj_set_style_radius(btn_open, 14, 0);
    lv_obj_set_style_bg_color(btn_open, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_set_style_shadow_width(btn_open, 12, 0);
    lv_obj_set_style_shadow_color(btn_open, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_set_style_shadow_opa(btn_open, LV_OPA_30, 0);
    lv_obj_set_style_shadow_ofs_y(btn_open, 4, 0);
    lv_obj_add_event_cb(btn_open, open_settings_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl_open = lv_label_create(btn_open);
    lv_label_set_text_fmt(lbl_open, "%s System-Einstellungen", LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_font(lbl_open, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_open, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_open);
    
    // Simulation / Debug Toggle Card
    lv_obj_t *card_dbg = ui_create_card(parent, 360, 56);
    lv_obj_set_style_pad_all(card_dbg, 10, 0);
    
    lv_obj_t *sw_debug = lv_switch_create(card_dbg);
    if (state.debug_mode) lv_obj_add_state(sw_debug, LV_STATE_CHECKED);
    lv_obj_align(sw_debug, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(sw_debug, [](lv_event_t * e) {
        lv_obj_t *sw = lv_event_get_target(e);
        state.debug_mode = lv_obj_has_state(sw, LV_STATE_CHECKED);
        state_save();
    }, LV_EVENT_VALUE_CHANGED, NULL);
    
    lv_obj_t *lbl_sw_dbg = lv_label_create(card_dbg);
    lv_label_set_text(lbl_sw_dbg, "Simulation / Dummy-Daten");
    lv_obj_set_style_text_font(lbl_sw_dbg, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_sw_dbg, ui_theme_text(), 0);
    lv_obj_align(lbl_sw_dbg, LV_ALIGN_LEFT_MID, 65, 0);

    // Status Card
    lv_obj_t *card_status = ui_create_card(parent, 360, 100);
    lv_obj_set_style_pad_all(card_status, 12, 0);
    
    lbl_debug_info = lv_label_create(card_status);
    lv_label_set_text(lbl_debug_info, "System Status:\nInitialisiere...");
    lv_obj_set_style_text_font(lbl_debug_info, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(lbl_debug_info, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_debug_info, ui_theme_text(), 0);
    lv_obj_center(lbl_debug_info);
}

// ==========================================
// The Dedicated Settings Screen
// ==========================================
void ui_settings_screen_init() {
    scr_settings = lv_obj_create(NULL);
    lv_obj_clear_flag(scr_settings, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(scr_settings, ui_theme_bg(), 0);
    
    // Create Top Tabview
    lv_obj_t *tv = lv_tabview_create(scr_settings, LV_DIR_TOP, 50);
    lv_obj_set_size(tv, 480, 360); // Leave room for Save button
    lv_obj_align(tv, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(tv, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *t_net = lv_tabview_add_tab(tv, "Netzwerk");
    lv_obj_t *t_gen = lv_tabview_add_tab(tv, "Allgemein");
    lv_obj_t *t_vis = lv_tabview_add_tab(tv, "Anzeige");
    lv_obj_t *t_val = lv_tabview_add_tab(tv, "Werte");
    lv_obj_t *t_alm = lv_tabview_add_tab(tv, "Alarme");
    
    // --- TAB 1: NETZWERK ---
    lv_obj_clear_flag(t_net, LV_OBJ_FLAG_SCROLLABLE); // Fit entirely
    
    // WLAN Row
    lv_obj_t *l_w = lv_label_create(t_net);
    lv_label_set_text(l_w, "WLAN Setup");
    lv_obj_align(l_w, LV_ALIGN_TOP_LEFT, 0, 0);
    
    dd_wifi_ssid = lv_dropdown_create(t_net);
    lv_dropdown_set_options(dd_wifi_ssid, state.wifi_ssid.c_str());
    lv_obj_set_width(dd_wifi_ssid, 150);
    lv_obj_align(dd_wifi_ssid, LV_ALIGN_TOP_LEFT, 0, 30);
    
    ta_wifi_pass = lv_textarea_create(t_net);
    lv_textarea_set_password_mode(ta_wifi_pass, true);
    lv_textarea_set_one_line(ta_wifi_pass, true);
    lv_textarea_set_text(ta_wifi_pass, state.wifi_pass.c_str());
    lv_textarea_set_placeholder_text(ta_wifi_pass, "Passwort");
    lv_obj_set_width(ta_wifi_pass, 150);
    lv_obj_align(ta_wifi_pass, LV_ALIGN_TOP_LEFT, 160, 30);
    lv_obj_add_event_cb(ta_wifi_pass, ta_event_cb, LV_EVENT_ALL, NULL);
    
    btn_wifi_scan = lv_btn_create(t_net);
    lv_obj_set_size(btn_wifi_scan, 120, 40);
    lv_obj_align(btn_wifi_scan, LV_ALIGN_TOP_LEFT, 320, 30);
    lv_obj_add_event_cb(btn_wifi_scan, wifi_scan_cb, LV_EVENT_CLICKED, NULL);
    lbl_wifi_scan = lv_label_create(btn_wifi_scan);
    lv_label_set_text(lbl_wifi_scan, "Scan");
    lv_obj_center(lbl_wifi_scan);
    
    // MQTT Row
    lv_obj_t *l_m = lv_label_create(t_net);
    lv_label_set_text(l_m, "VanPi IP Adresse");
    lv_obj_align(l_m, LV_ALIGN_TOP_LEFT, 0, 90);
    
    ta_mqtt_ip = lv_textarea_create(t_net);
    lv_textarea_set_one_line(ta_mqtt_ip, true);
    lv_textarea_set_text(ta_mqtt_ip, state.vanpi_ip.c_str());
    lv_textarea_set_placeholder_text(ta_mqtt_ip, "IP Adresse");
    lv_obj_set_width(ta_mqtt_ip, 200);
    lv_obj_align(ta_mqtt_ip, LV_ALIGN_TOP_LEFT, 0, 120);
    lv_obj_add_event_cb(ta_mqtt_ip, ta_event_cb, LV_EVENT_ALL, NULL);
    
        
        
        
    // --- TAB 2: ALLGEMEIN ---
    lv_obj_clear_flag(t_gen, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *l_disp = lv_label_create(t_gen);
    lv_label_set_text(l_disp, "Display Einstellungen");
    lv_obj_align(l_disp, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *l_bri = lv_label_create(t_gen);
    lv_label_set_text(l_bri, "Helligkeit:");
    lv_obj_align(l_bri, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_obj_t *sl_bri = lv_slider_create(t_gen);
    lv_slider_set_range(sl_bri, 10, 100);
    lv_slider_set_value(sl_bri, state.display_brightness, LV_ANIM_OFF);
    lv_obj_set_width(sl_bri, 200);
    lv_obj_align(sl_bri, LV_ALIGN_TOP_LEFT, 100, 30);
    lv_obj_add_event_cb(sl_bri, [](lv_event_t * e) {
        lv_obj_t *slider = lv_event_get_target(e);
        state.display_brightness = lv_slider_get_value(slider);
        uint8_t pwm = 255 - (state.display_brightness * 255 / 100);
        WS_CH32_IO::setPwm(Wire, pwm);
        state_save();
    }, LV_EVENT_VALUE_CHANGED, NULL);
    
    lv_obj_t *sw_theme = lv_switch_create(t_gen);
    if(state.dark_mode) lv_obj_add_state(sw_theme, LV_STATE_CHECKED);
    lv_obj_align(sw_theme, LV_ALIGN_TOP_LEFT, 0, 70);
    lv_obj_add_event_cb(sw_theme, [](lv_event_t * e) {
        lv_obj_t *sw = lv_event_get_target(e);
        state.dark_mode = lv_obj_has_state(sw, LV_STATE_CHECKED);
        state_save();
        ui_init(); // Reload theme
    }, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *l_theme = lv_label_create(t_gen);
    lv_label_set_text(l_theme, "Dunkler Modus");
    lv_obj_align(l_theme, LV_ALIGN_TOP_LEFT, 60, 75);
    
    lv_obj_t *l_time = lv_label_create(t_gen);
    lv_label_set_text(l_time, "Timeout:");
    lv_obj_align(l_time, LV_ALIGN_TOP_LEFT, 0, 120);
    lv_obj_t *dd_time = lv_dropdown_create(t_gen);
    lv_dropdown_set_options(dd_time, "15 Sekunden\n30 Sekunden\n1 Minute\n5 Minuten\nImmer an");
    if(state.display_timeout == 15) lv_dropdown_set_selected(dd_time, 0);
    else if(state.display_timeout == 30) lv_dropdown_set_selected(dd_time, 1);
    else if(state.display_timeout == 60) lv_dropdown_set_selected(dd_time, 2);
    else if(state.display_timeout == 300) lv_dropdown_set_selected(dd_time, 3);
    else lv_dropdown_set_selected(dd_time, 4);
    lv_obj_align(dd_time, LV_ALIGN_TOP_LEFT, 100, 110);
    lv_obj_add_event_cb(dd_time, [](lv_event_t * e) {
        lv_obj_t *dd = lv_event_get_target(e);
        int sel = lv_dropdown_get_selected(dd);
        if (sel == 0) state.display_timeout = 15;
        else if (sel == 1) state.display_timeout = 30;
        else if (sel == 2) state.display_timeout = 60;
        else if (sel == 3) state.display_timeout = 300;
        else if (sel == 4) state.display_timeout = 0;
        state_save();
    }, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Demo Mode Switch
    lv_obj_t *sw_demo = lv_switch_create(t_gen);
    if(state.debug_mode) lv_obj_add_state(sw_demo, LV_STATE_CHECKED);
    lv_obj_align(sw_demo, LV_ALIGN_TOP_LEFT, 0, 160);
    lv_obj_add_event_cb(sw_demo, [](lv_event_t * e) {
        lv_obj_t *sw = lv_event_get_target(e);
        state.debug_mode = lv_obj_has_state(sw, LV_STATE_CHECKED);
        state_save();
    }, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *l_demo = lv_label_create(t_gen);
    lv_label_set_text(l_demo, "Simulation / Dummy-Daten");
    lv_obj_align(l_demo, LV_ALIGN_TOP_LEFT, 60, 165);

    // Reboot Button
    lv_obj_t *btn_reboot = lv_btn_create(t_gen);
    lv_obj_set_size(btn_reboot, 200, 45);
    lv_obj_align(btn_reboot, LV_ALIGN_TOP_LEFT, 0, 215);
    lv_obj_set_style_bg_color(btn_reboot, lv_color_hex(0xe74c3c), 0); // Red
    lv_obj_add_event_cb(btn_reboot, [](lv_event_t * e) {
        ESP.restart();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_reboot = lv_label_create(btn_reboot);
    lv_label_set_text(l_reboot, "Neustart");
    lv_obj_center(l_reboot);
    
    // --- TAB 3: SICHTBARKEIT ---
    lv_obj_clear_flag(t_vis, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *l_vis = lv_label_create(t_vis);
    lv_label_set_text(l_vis, "Schalter & Dimmer Sichtbarkeit");
    lv_obj_align(l_vis, LV_ALIGN_TOP_LEFT, 0, 0);
    
    for (int i = 0; i < 8; i++) {
        // Relais Column
        lv_obj_t *cb_r = lv_checkbox_create(t_vis);
        lv_checkbox_set_text(cb_r, state.switch_names[i].c_str());
        if (state.switch_visible[i]) lv_obj_add_state(cb_r, LV_STATE_CHECKED);
        lv_obj_align(cb_r, LV_ALIGN_TOP_LEFT, 0, 30 + (i * 30));
        lv_obj_add_event_cb(cb_r, [](lv_event_t * e) {
            lv_obj_t *obj = lv_event_get_target(e);
            int idx = (int)(intptr_t)lv_event_get_user_data(e);
            state.switch_visible[idx] = lv_obj_has_state(obj, LV_STATE_CHECKED);
        }, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)i);
        
        // Dimmer Column
        lv_obj_t *cb_d = lv_checkbox_create(t_vis);
        lv_checkbox_set_text(cb_d, state.dimmer_names[i].c_str());
        if (state.dimmer_visible[i]) lv_obj_add_state(cb_d, LV_STATE_CHECKED);
        lv_obj_align(cb_d, LV_ALIGN_TOP_LEFT, 200, 30 + (i * 30));
        lv_obj_add_event_cb(cb_d, [](lv_event_t * e) {
            lv_obj_t *obj = lv_event_get_target(e);
            int idx = (int)(intptr_t)lv_event_get_user_data(e);
            state.dimmer_visible[idx] = lv_obj_has_state(obj, LV_STATE_CHECKED);
        }, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)i);
    }
    
    // --- TAB 4: WERTE ---
    lv_obj_clear_flag(t_val, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t *l_pump = lv_label_create(t_val);
    lv_label_set_text(l_pump, "Wasserpumpe Relais:");
    lv_obj_align(l_pump, LV_ALIGN_TOP_LEFT, 0, 20);
    
    lv_obj_t *dd_pump = lv_dropdown_create(t_val);
    String opts_p = "Keins\n";
    for(int i=0; i<8; i++) { opts_p += "Relais " + String(i+1); if (i!=7) opts_p+="\n"; }
    lv_dropdown_set_options(dd_pump, opts_p.c_str());
    lv_dropdown_set_selected(dd_pump, state.pump_relay + 1);
    lv_obj_align(dd_pump, LV_ALIGN_TOP_LEFT, 180, 10);
    lv_obj_add_event_cb(dd_pump, [](lv_event_t * e) {
        lv_obj_t *dd = lv_event_get_target(e);
        state.pump_relay = lv_dropdown_get_selected(dd) - 1;
    }, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Solar Max
    lv_obj_t *l_solar = lv_label_create(t_val);
    lv_label_set_text(l_solar, "Solar (W):");
    lv_obj_align(l_solar, LV_ALIGN_TOP_LEFT, 290, 20);
    
    ta_solar_max = lv_textarea_create(t_val);
    lv_textarea_set_one_line(ta_solar_max, true);
    lv_textarea_set_text(ta_solar_max, String((int)state.solar_max_w).c_str());
    lv_obj_set_width(ta_solar_max, 70);
    lv_obj_align(ta_solar_max, LV_ALIGN_TOP_LEFT, 370, 10);
    lv_obj_add_event_cb(ta_solar_max, ta_event_cb, LV_EVENT_ALL, NULL);
    
    
    lv_obj_t *l_drain = lv_label_create(t_val);
    lv_label_set_text(l_drain, "Abwasser Relais:");
    lv_obj_align(l_drain, LV_ALIGN_TOP_LEFT, 0, 70);
    
    lv_obj_t *dd_drain = lv_dropdown_create(t_val);
    lv_dropdown_set_options(dd_drain, opts_p.c_str());
    lv_dropdown_set_selected(dd_drain, state.drain_relay + 1);
    lv_obj_align(dd_drain, LV_ALIGN_TOP_LEFT, 180, 60);
    lv_obj_add_event_cb(dd_drain, [](lv_event_t * e) {
        lv_obj_t *dd = lv_event_get_target(e);
        state.drain_relay = lv_dropdown_get_selected(dd) - 1;
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Tank Limits
    lv_obj_t *l_tanks = lv_label_create(t_val);
    lv_label_set_text(l_tanks, "Tank Max (Liter):");
    lv_obj_align(l_tanks, LV_ALIGN_TOP_LEFT, 0, 120);
    
    for (int i = 0; i < 4; i++) {
        int row = i / 2;
        int col = i % 2;
        lv_obj_t *l_t = lv_label_create(t_val);
        lv_label_set_text_fmt(l_t, "T%d:", i+1);
        lv_obj_align(l_t, LV_ALIGN_TOP_LEFT, col * 120, 155 + (row * 50));
        
        ta_tank_max[i] = lv_textarea_create(t_val);
        lv_textarea_set_one_line(ta_tank_max[i], true);
        lv_textarea_set_text(ta_tank_max[i], String(state.tank_max[i]).c_str());
        lv_obj_set_width(ta_tank_max[i], 70);
        lv_obj_align(ta_tank_max[i], LV_ALIGN_TOP_LEFT, 35 + (col * 120), 145 + (row * 50));
        lv_obj_add_event_cb(ta_tank_max[i], ta_event_cb, LV_EVENT_ALL, NULL);
    }
    
    // --- TAB 5: ALARME ---
    lv_obj_clear_flag(t_alm, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *l_alm_title = lv_label_create(t_alm);
    lv_label_set_text(l_alm_title, "Smart Header Alarmschwellen");
    lv_obj_set_style_text_font(l_alm_title, &lv_font_montserrat_16, 0);
    lv_obj_align(l_alm_title, LV_ALIGN_TOP_LEFT, 0, 4);

    // 1. Frost
    lv_obj_t *l_f = lv_label_create(t_alm);
    lv_label_set_text(l_f, "Frostgrenze Aussen (<= deg C):");
    lv_obj_align(l_f, LV_ALIGN_TOP_LEFT, 0, 36);

    ta_warn_frost = lv_textarea_create(t_alm);
    lv_textarea_set_one_line(ta_warn_frost, true);
    char buf_f[16];
    snprintf(buf_f, sizeof(buf_f), "%.1f", state.warn_frost_temp);
    lv_textarea_set_text(ta_warn_frost, buf_f);
    lv_obj_set_width(ta_warn_frost, 80);
    lv_obj_align(ta_warn_frost, LV_ALIGN_TOP_LEFT, 270, 28);
    lv_obj_add_event_cb(ta_warn_frost, ta_event_cb, LV_EVENT_ALL, NULL);

    // 2. Batterie
    lv_obj_t *l_b = lv_label_create(t_alm);
    lv_label_set_text(l_b, "Batterie Schwach (<= % SoC):");
    lv_obj_align(l_b, LV_ALIGN_TOP_LEFT, 0, 84);

    ta_warn_bat = lv_textarea_create(t_alm);
    lv_textarea_set_one_line(ta_warn_bat, true);
    lv_textarea_set_text(ta_warn_bat, String(state.warn_bat_soc).c_str());
    lv_obj_set_width(ta_warn_bat, 80);
    lv_obj_align(ta_warn_bat, LV_ALIGN_TOP_LEFT, 270, 76);
    lv_obj_add_event_cb(ta_warn_bat, ta_event_cb, LV_EVENT_ALL, NULL);

    // 3. Frischwasser
    lv_obj_t *l_fr = lv_label_create(t_alm);
    lv_label_set_text(l_fr, "Frischwasser Leer (<= %):");
    lv_obj_align(l_fr, LV_ALIGN_TOP_LEFT, 0, 132);

    ta_warn_fresh = lv_textarea_create(t_alm);
    lv_textarea_set_one_line(ta_warn_fresh, true);
    lv_textarea_set_text(ta_warn_fresh, String(state.warn_fresh_min).c_str());
    lv_obj_set_width(ta_warn_fresh, 80);
    lv_obj_align(ta_warn_fresh, LV_ALIGN_TOP_LEFT, 270, 124);
    lv_obj_add_event_cb(ta_warn_fresh, ta_event_cb, LV_EVENT_ALL, NULL);

    // 4. Abwasser
    lv_obj_t *l_wa = lv_label_create(t_alm);
    lv_label_set_text(l_wa, "Abwasser Voll (>= %):");
    lv_obj_align(l_wa, LV_ALIGN_TOP_LEFT, 0, 180);

    ta_warn_waste = lv_textarea_create(t_alm);
    lv_textarea_set_one_line(ta_warn_waste, true);
    lv_textarea_set_text(ta_warn_waste, String(state.warn_waste_max).c_str());
    lv_obj_set_width(ta_warn_waste, 80);
    lv_obj_align(ta_warn_waste, LV_ALIGN_TOP_LEFT, 270, 172);
    lv_obj_add_event_cb(ta_warn_waste, ta_event_cb, LV_EVENT_ALL, NULL);

    // Virtual Keyboard (Global to scr_settings)
    kb = lv_keyboard_create(scr_settings);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    
    // Save & Exit Button
    lv_obj_t *btn_save = lv_btn_create(scr_settings);
    lv_obj_set_size(btn_save, 400, 50);
    lv_obj_align(btn_save, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(btn_save, lv_color_hex(0x2ecc71), 0);
    lv_obj_add_event_cb(btn_save, [](lv_event_t * e) {
        state.solar_max_w = String(lv_textarea_get_text(ta_solar_max)).toFloat();
        for (int i=0; i<4; i++) {
            state.tank_max[i] = String(lv_textarea_get_text(ta_tank_max[i])).toInt();
        }
        if (ta_warn_frost) state.warn_frost_temp = String(lv_textarea_get_text(ta_warn_frost)).toFloat();
        if (ta_warn_bat)   state.warn_bat_soc    = String(lv_textarea_get_text(ta_warn_bat)).toInt();
        if (ta_warn_fresh) state.warn_fresh_min  = String(lv_textarea_get_text(ta_warn_fresh)).toInt();
        if (ta_warn_waste) state.warn_waste_max  = String(lv_textarea_get_text(ta_warn_waste)).toInt();
        save_settings_cb(e);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_save = lv_label_create(btn_save);
    lv_label_set_text(l_save, "Speichern & Beenden");
    lv_obj_set_style_text_font(l_save, &lv_font_montserrat_18, 0);
    lv_obj_center(l_save);
}
