#include "ui_main.h"
#include "wifi_diagnostics.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

static lv_obj_t *sw_debug = NULL, *sw_demo = NULL;
static lv_obj_t * http_test_win = NULL;
static lv_obj_t * http_lbl_result = NULL;
static lv_obj_t *dd_time_h = NULL;
static lv_obj_t *dd_time_m = NULL;
static lv_obj_t *dd_tz = NULL;
static lv_obj_t *cont_man_time = NULL;
static lv_obj_t *lbl_time_status = NULL;

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
static lv_obj_t *password_eye_label, *wifi_status_label;
static lv_obj_t *ta_mqtt_ip;
static lv_obj_t *dd_out_temp = NULL;



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

void ui_sync_demo_controls() {
    lv_obj_t *controls[] = {sw_debug, sw_demo};
    for (lv_obj_t *control : controls) {
        if (!control || !lv_obj_is_valid(control)) continue;
        if (state.debug_mode) lv_obj_add_state(control, LV_STATE_CHECKED);
        else lv_obj_clear_state(control, LV_STATE_CHECKED);
    }
}
void ui_update_settings_tab() {
    ui_sync_demo_controls();
    if (lv_scr_act() == scr_settings && wifi_status_label) {
        String details = wifi_connection_details();
        ui_label_set_text_if_changed(wifi_status_label, details.c_str());
    }
    if (lv_scr_act() == scr_settings && lbl_time_status) {
        time_t now = time(nullptr);
        struct tm tm_now;
        localtime_r(&now, &tm_now);
        char buf[80];
        snprintf(buf, sizeof(buf), "Aktuelle Systemzeit: %02d:%02d:%02d (%s)",
                 tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec,
                 state.time_auto_ntp ? "NTP Internet" : "Manuell");
        ui_label_set_text_if_changed(lbl_time_status, buf);
    }
    if (ui_home_settings_is_active()) ui_home_settings_refresh();
    if (scan_in_progress) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            scan_in_progress = false;
            // Keep the saved WLAN selectable even when it is out of range.
            String options = state.wifi_ssid;
            for (int i = 0; i < n; ++i) {
                String ssid = WiFi.SSID(i);
                if (ssid.length() == 0 || ssid == state.wifi_ssid) continue;
                if (options.length() > 0) options += "\n";
                options += ssid;
            }
            if (options.length() == 0) options = "Keine Netzwerke gefunden";
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
    if(code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(kb); // Bring keyboard to front
    }
    else if(code == LV_EVENT_DEFOCUSED || code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
            lv_indev_reset(NULL, ta); // clear focus
        }
    }
}

static void kb_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_indev_reset(NULL, NULL);
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

static void mbox_save_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * mbox = lv_event_get_current_target(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        lv_msgbox_close_async(mbox);
        ui_init();
        lv_scr_load(scr_main);
    } else if (code == LV_EVENT_DELETE) {
        ui_init();
        lv_scr_load(scr_main);
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
    lv_obj_t * mbox = lv_msgbox_create(NULL, "Gespeichert", "Einstellungen erfolgreich gespeichert.", btns, true);
    lv_obj_center(mbox);
    lv_obj_add_event_cb(mbox, mbox_save_cb, LV_EVENT_ALL, NULL);
    
    // Attempt reconnects
    WiFi.disconnect();
    wifi_connect_configured();
    
    // Load main screen beneath the dialog
    lv_scr_load(scr_main);
}

static void update_dd_out_temp_options() {
    if (!dd_out_temp) return;
    String opts = "";
    for (int i = 0; i < 4; i++) {
        if (state.temp_sensor_names[i].length() > 0 && 
            state.temp_sensor_names[i] != "Sensor " + String(i + 1) && 
            state.temp_sensor_names[i] != "Temp " + String(i + 1)) {
            opts += "Sensor " + String(i + 1) + " (" + state.temp_sensor_names[i] + ")";
        } else {
            opts += "Sensor " + String(i + 1) + " (Temp " + String(i + 1) + ")";
        }
        if (i < 3) opts += "\n";
    }
    lv_dropdown_set_options(dd_out_temp, opts.c_str());
    lv_dropdown_set_selected(dd_out_temp, constrain(state.outdoor_temp_sensor, 0, 3));
}

static void open_settings_cb(lv_event_t * e) {
    update_dd_out_temp_options();
    lv_textarea_set_password_mode(ta_wifi_pass, true);
    lv_label_set_text(password_eye_label, LV_SYMBOL_EYE_OPEN);
    time_t now = time(nullptr);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    if (dd_time_h) lv_dropdown_set_selected(dd_time_h, constrain(tm_now.tm_hour, 0, 23));
    if (dd_time_m) lv_dropdown_set_selected(dd_time_m, constrain(tm_now.tm_min, 0, 59));
    if (dd_tz) lv_dropdown_set_selected(dd_tz, constrain(state.time_zone_idx, 0, TIMEZONE_COUNT - 1));
    lv_scr_load(scr_settings);
}

// ==========================================
// 5th Tab on Main Screen
// ==========================================
void ui_build_settings(lv_obj_t *parent) {
    ui_setup_tab_page(parent);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(parent, 12, 0);
    
    // Modern Action Button
    lv_obj_t *btn_open = lv_btn_create(parent);
    lv_obj_set_size(btn_open, 320, 60);
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
    
    lv_obj_t *btn_home = lv_btn_create(parent);
    lv_obj_set_size(btn_home, 320, 46);
    lv_obj_set_style_radius(btn_home, 14, 0);
    lv_obj_set_style_bg_color(btn_home, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_add_event_cb(btn_home, [](lv_event_t *) { ui_open_home_settings(); }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *lbl_home = lv_label_create(btn_home);
    lv_label_set_text(lbl_home, LV_SYMBOL_HOME " Home-Einstellungen");
    lv_obj_set_style_text_font(lbl_home, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_home, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_home);

    lv_obj_t *btn_nav = lv_btn_create(parent);
    lv_obj_set_size(btn_nav, 320, 46);
    lv_obj_set_style_radius(btn_nav, 14, 0);
    lv_obj_set_style_bg_color(btn_nav, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_add_event_cb(btn_nav, [](lv_event_t *) { ui_open_nav_settings(); }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *lbl_nav = lv_label_create(btn_nav);
    lv_label_set_text(lbl_nav, LV_SYMBOL_LIST "  Menueleiste / Tabs anpassen");
    lv_obj_set_style_text_font(lbl_nav, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_nav, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_nav);

    // Simulation / Debug Toggle Card
    lv_obj_t *card_dbg = ui_create_card(parent, 360, 56);
    lv_obj_set_style_pad_all(card_dbg, 10, 0);
    
    sw_debug = lv_switch_create(card_dbg);
    if (state.debug_mode) lv_obj_add_state(sw_debug, LV_STATE_CHECKED);
    lv_obj_align(sw_debug, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(sw_debug, [](lv_event_t * e) {
        lv_obj_t *sw = lv_event_get_target(e);
        state.debug_mode = lv_obj_has_state(sw, LV_STATE_CHECKED);
        ui_sync_demo_controls();
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
    lv_obj_t *t_time = lv_tabview_add_tab(tv, "Zeit");

    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tv);
    if (tab_btns) {
        lv_obj_set_style_pad_left(tab_btns, 2, 0);
        lv_obj_set_style_pad_right(tab_btns, 2, 0);
    }
    
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
    lv_obj_set_width(ta_wifi_pass, 130);
    lv_obj_align(ta_wifi_pass, LV_ALIGN_TOP_LEFT, 160, 30);
    lv_obj_add_event_cb(ta_wifi_pass, ta_event_cb, LV_EVENT_ALL, NULL);
    
    lv_obj_t *password_eye = lv_btn_create(t_net);
    lv_obj_set_pos(password_eye, 298, 30); lv_obj_set_size(password_eye, 40, 40);
    password_eye_label = lv_label_create(password_eye);
    lv_label_set_text(password_eye_label, LV_SYMBOL_EYE_OPEN); lv_obj_center(password_eye_label);
    lv_obj_add_event_cb(password_eye, [](lv_event_t *) {
        bool show = lv_textarea_get_password_mode(ta_wifi_pass);
        lv_textarea_set_password_mode(ta_wifi_pass, !show);
        lv_label_set_text(password_eye_label, show ? LV_SYMBOL_EYE_CLOSE : LV_SYMBOL_EYE_OPEN);
    }, LV_EVENT_CLICKED, nullptr);

    btn_wifi_scan = lv_btn_create(t_net);
    lv_obj_set_size(btn_wifi_scan, 94, 40);
    lv_obj_align(btn_wifi_scan, LV_ALIGN_TOP_LEFT, 346, 30);
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
    
        
        
        
    wifi_status_label = lv_label_create(t_net);
    lv_obj_set_pos(wifi_status_label, 0, 178); lv_obj_set_width(wifi_status_label, 440);
    lv_obj_set_style_text_font(wifi_status_label, &lv_font_montserrat_12, 0);
    lv_label_set_long_mode(wifi_status_label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(wifi_status_label, wifi_connection_details().c_str());

    // --- TAB 2: ALLGEMEIN ---
    lv_obj_clear_flag(t_gen, LV_OBJ_FLAG_SCROLLABLE);
    
    // Left Column: Display Settings
    lv_obj_t *l_disp = lv_label_create(t_gen);
    lv_label_set_text(l_disp, "Display & System");
    lv_obj_align(l_disp, LV_ALIGN_TOP_LEFT, 0, 0);
    
    lv_obj_t *l_bri = lv_label_create(t_gen);
    lv_label_set_text(l_bri, "Helligkeit:");
    lv_obj_align(l_bri, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_obj_t *sl_bri = lv_slider_create(t_gen);
    lv_slider_set_range(sl_bri, 10, 100);
    lv_slider_set_value(sl_bri, state.display_brightness, LV_ANIM_OFF);
    lv_obj_set_width(sl_bri, 130);
    lv_obj_align(sl_bri, LV_ALIGN_TOP_LEFT, 90, 30);
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
    lv_obj_set_width(dd_time, 130);
    lv_obj_align(dd_time, LV_ALIGN_TOP_LEFT, 90, 110);
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
    sw_demo = lv_switch_create(t_gen);
    if(state.debug_mode) lv_obj_add_state(sw_demo, LV_STATE_CHECKED);
    lv_obj_align(sw_demo, LV_ALIGN_TOP_LEFT, 0, 160);
    lv_obj_add_event_cb(sw_demo, [](lv_event_t * e) {
        lv_obj_t *sw = lv_event_get_target(e);
        state.debug_mode = lv_obj_has_state(sw, LV_STATE_CHECKED);
        ui_sync_demo_controls();
        state_save();
    }, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_t *l_demo = lv_label_create(t_gen);
    lv_label_set_text(l_demo, "Simulation / Dummy");
    lv_obj_align(l_demo, LV_ALIGN_TOP_LEFT, 60, 165);

    // Reboot Button
    lv_obj_t *btn_reboot = lv_btn_create(t_gen);
    lv_obj_set_size(btn_reboot, 180, 42);
    lv_obj_align(btn_reboot, LV_ALIGN_TOP_LEFT, 0, 215);
    lv_obj_set_style_bg_color(btn_reboot, lv_color_hex(0xe74c3c), 0); // Red
    lv_obj_add_event_cb(btn_reboot, [](lv_event_t * e) {
        ESP.restart();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_reboot = lv_label_create(btn_reboot);
    lv_label_set_text(l_reboot, "Neustart");
    lv_obj_center(l_reboot);

    // Right Column: Sensor Assignment
    lv_obj_t *l_sens_sec = lv_label_create(t_gen);
    lv_label_set_text(l_sens_sec, "Temperatursensor");
    lv_obj_align(l_sens_sec, LV_ALIGN_TOP_LEFT, 240, 0);

    lv_obj_t *l_out_sens = lv_label_create(t_gen);
    lv_label_set_text(l_out_sens, "Aussentemperatur:");
    lv_obj_align(l_out_sens, LV_ALIGN_TOP_LEFT, 240, 30);

    dd_out_temp = lv_dropdown_create(t_gen);
    lv_dropdown_set_options(dd_out_temp, "Sensor 1 (Temp 1)\nSensor 2 (Temp 2)\nSensor 3 (Temp 3)\nSensor 4 (Temp 4)");
    lv_dropdown_set_selected(dd_out_temp, constrain(state.outdoor_temp_sensor, 0, 3));
    lv_obj_set_width(dd_out_temp, 200);
    lv_obj_align(dd_out_temp, LV_ALIGN_TOP_LEFT, 240, 55);
    lv_obj_add_event_cb(dd_out_temp, [](lv_event_t * e) {
        lv_obj_t *dd = lv_event_get_target(e);
        state.outdoor_temp_sensor = lv_dropdown_get_selected(dd);
        int out_idx = constrain(state.outdoor_temp_sensor, 0, 3);
        state.outdoor_temp = state.temp_sensors[out_idx];
        int in_idx = (out_idx == 0) ? 1 : 0;
        state.indoor_temp = state.temp_sensors[in_idx];
        state_save();
    }, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *l_out_desc = lv_label_create(t_gen);
    lv_label_set_text(l_out_desc, "Waehlt den Sensor fuer\ndas Aussen-Klima &\ndie Frostwarnung.");
    lv_obj_set_style_text_color(l_out_desc, ui_theme_muted(), 0);
    lv_obj_set_style_text_font(l_out_desc, &lv_font_montserrat_12, 0);
    lv_obj_align(l_out_desc, LV_ALIGN_TOP_LEFT, 240, 115);

    lv_obj_t *btn_nav_modal = lv_btn_create(t_gen);
    lv_obj_set_size(btn_nav_modal, 200, 42);
    lv_obj_align(btn_nav_modal, LV_ALIGN_TOP_LEFT, 240, 168);
    lv_obj_set_style_bg_color(btn_nav_modal, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_add_event_cb(btn_nav_modal, [](lv_event_t *) { ui_open_nav_settings(); }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *lbl_nav_m = lv_label_create(btn_nav_modal);
    lv_label_set_text(lbl_nav_m, LV_SYMBOL_LIST " Tabs anpassen");
    lv_obj_center(lbl_nav_m);
    
    // --- TAB 3: SICHTBARKEIT ---
    lv_obj_add_flag(t_vis, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(t_vis, LV_DIR_VER);
    lv_obj_set_style_pad_bottom(t_vis, 40, 0);
    
    lv_obj_t *l_vis = lv_label_create(t_vis);
    lv_label_set_text(l_vis, "Schalter & Dimmer Sichtbarkeit");
    lv_obj_align(l_vis, LV_ALIGN_TOP_LEFT, 0, 0);
    
    for (int i = 0; i < 8; i++) {
        // Relais Column
        lv_obj_t *cb_r = lv_checkbox_create(t_vis);
        lv_checkbox_set_text(cb_r, state.switch_names[i].c_str());
        if (state.switch_visible[i]) lv_obj_add_state(cb_r, LV_STATE_CHECKED);
        lv_obj_align(cb_r, LV_ALIGN_TOP_LEFT, 0, 28 + (i * 28));
        lv_obj_add_event_cb(cb_r, [](lv_event_t * e) {
            lv_obj_t *obj = lv_event_get_target(e);
            int idx = (int)(intptr_t)lv_event_get_user_data(e);
            state.switch_visible[idx] = lv_obj_has_state(obj, LV_STATE_CHECKED);
        }, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)i);
        
        // Dimmer Column
        lv_obj_t *cb_d = lv_checkbox_create(t_vis);
        lv_checkbox_set_text(cb_d, state.dimmer_names[i].c_str());
        if (state.dimmer_visible[i]) lv_obj_add_state(cb_d, LV_STATE_CHECKED);
        lv_obj_align(cb_d, LV_ALIGN_TOP_LEFT, 210, 28 + (i * 28));
        lv_obj_add_event_cb(cb_d, [](lv_event_t * e) {
            lv_obj_t *obj = lv_event_get_target(e);
            int idx = (int)(intptr_t)lv_event_get_user_data(e);
            state.dimmer_visible[idx] = lv_obj_has_state(obj, LV_STATE_CHECKED);
        }, LV_EVENT_VALUE_CHANGED, (void*)(intptr_t)i);
    }

    // WiFi Relais Section
    lv_obj_t *l_wr_head = lv_label_create(t_vis);
    lv_label_set_text(l_wr_head, "WiFi-Relais");
    lv_obj_set_style_text_font(l_wr_head, &lv_font_montserrat_16, 0);
    lv_obj_align(l_wr_head, LV_ALIGN_TOP_LEFT, 0, 260);

    lv_obj_t *cont_wr = lv_obj_create(t_vis);
    lv_obj_set_size(cont_wr, 440, 130);
    lv_obj_align(cont_wr, LV_ALIGN_TOP_LEFT, 0, 330);
    lv_obj_set_style_bg_opa(cont_wr, 0, 0);
    lv_obj_set_style_border_width(cont_wr, 0, 0);
    lv_obj_set_style_pad_all(cont_wr, 0, 0);
    if (!state.show_wrelay) lv_obj_add_flag(cont_wr, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *sw_wrelay = lv_switch_create(t_vis);
    if (state.show_wrelay) lv_obj_add_state(sw_wrelay, LV_STATE_CHECKED);
    lv_obj_align(sw_wrelay, LV_ALIGN_TOP_LEFT, 0, 290);
    lv_obj_add_event_cb(sw_wrelay, [](lv_event_t * e) {
        lv_obj_t *sw = lv_event_get_target(e);
        lv_obj_t *cont = (lv_obj_t *)lv_event_get_user_data(e);
        state.show_wrelay = lv_obj_has_state(sw, LV_STATE_CHECKED);
        if (state.show_wrelay) {
            lv_obj_clear_flag(cont, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
        }
    }, LV_EVENT_VALUE_CHANGED, cont_wr);

    lv_obj_t *l_sw_wr = lv_label_create(t_vis);
    lv_label_set_text(l_sw_wr, "WiFi-Relais anzeigen");
    lv_obj_align(l_sw_wr, LV_ALIGN_TOP_LEFT, 60, 295);

    for (int i = 0; i < 8; i++) {
        lv_obj_t *cb_wr = lv_checkbox_create(cont_wr);
        lv_checkbox_set_text(cb_wr, state.wrelay_names[i].c_str());
        if (state.wrelay_visible[i]) lv_obj_add_state(cb_wr, LV_STATE_CHECKED);
        lv_obj_align(cb_wr, LV_ALIGN_TOP_LEFT, (i < 4 ? 0 : 210), (i % 4) * 28);
        lv_obj_add_event_cb(cb_wr, [](lv_event_t * e) {
            lv_obj_t *obj = lv_event_get_target(e);
            int idx = (int)(intptr_t)lv_event_get_user_data(e);
            state.wrelay_visible[idx] = lv_obj_has_state(obj, LV_STATE_CHECKED);
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

    // --- TAB 6: ZEIT ---
    lv_obj_clear_flag(t_time, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *l_time_title = lv_label_create(t_time);
    lv_label_set_text(l_time_title, "Datum & Uhrzeit");
    lv_obj_set_style_text_font(l_time_title, &lv_font_montserrat_16, 0);
    lv_obj_align(l_time_title, LV_ALIGN_TOP_LEFT, 0, 4);

    lv_obj_t *l_tz = lv_label_create(t_time);
    lv_label_set_text(l_tz, "Zeitzone:");
    lv_obj_align(l_tz, LV_ALIGN_TOP_LEFT, 0, 34);

    String opts_tz = "";
    for (int i = 0; i < TIMEZONE_COUNT; i++) {
        opts_tz += TIMEZONES[i].name;
        if (i < TIMEZONE_COUNT - 1) opts_tz += "\n";
    }

    dd_tz = lv_dropdown_create(t_time);
    lv_dropdown_set_options(dd_tz, opts_tz.c_str());
    lv_dropdown_set_selected(dd_tz, constrain(state.time_zone_idx, 0, TIMEZONE_COUNT - 1));
    lv_obj_set_width(dd_tz, 440);
    lv_obj_align(dd_tz, LV_ALIGN_TOP_LEFT, 0, 56);
    lv_obj_add_event_cb(dd_tz, [](lv_event_t * e) {
        lv_obj_t *dd = lv_event_get_target(e);
        state.time_zone_idx = lv_dropdown_get_selected(dd);
        time_apply_configuration();
    }, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *sw_auto_time = lv_switch_create(t_time);
    if (state.time_auto_ntp) lv_obj_add_state(sw_auto_time, LV_STATE_CHECKED);
    lv_obj_align(sw_auto_time, LV_ALIGN_TOP_LEFT, 0, 110);

    lv_obj_t *l_auto_time = lv_label_create(t_time);
    lv_label_set_text(l_auto_time, "Automatische Zeit (NTP / Internet)");
    lv_obj_align(l_auto_time, LV_ALIGN_TOP_LEFT, 60, 115);

    cont_man_time = lv_obj_create(t_time);
    lv_obj_set_size(cont_man_time, 440, 56);
    lv_obj_align(cont_man_time, LV_ALIGN_TOP_LEFT, 0, 155);
    lv_obj_set_style_bg_opa(cont_man_time, 0, 0);
    lv_obj_set_style_border_width(cont_man_time, 0, 0);
    lv_obj_set_style_pad_all(cont_man_time, 0, 0);
    if (state.time_auto_ntp) lv_obj_add_flag(cont_man_time, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_event_cb(sw_auto_time, [](lv_event_t * e) {
        lv_obj_t *sw = lv_event_get_target(e);
        state.time_auto_ntp = lv_obj_has_state(sw, LV_STATE_CHECKED);
        if (cont_man_time) {
            if (state.time_auto_ntp) lv_obj_add_flag(cont_man_time, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(cont_man_time, LV_OBJ_FLAG_HIDDEN);
        }
        time_apply_configuration();
    }, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *l_man = lv_label_create(cont_man_time);
    lv_label_set_text(l_man, "Manuell:");
    lv_obj_align(l_man, LV_ALIGN_LEFT_MID, 0, 0);

    String opts_h = "";
    for (int i = 0; i < 24; i++) {
        char b[8]; snprintf(b, sizeof(b), "%02d", i);
        opts_h += b;
        if (i < 23) opts_h += "\n";
    }
    dd_time_h = lv_dropdown_create(cont_man_time);
    lv_dropdown_set_options(dd_time_h, opts_h.c_str());
    lv_dropdown_set_selected(dd_time_h, constrain(state.manual_hour, 0, 23));
    lv_obj_set_width(dd_time_h, 75);
    lv_obj_align(dd_time_h, LV_ALIGN_LEFT_MID, 75, 0);

    lv_obj_t *l_sep = lv_label_create(cont_man_time);
    lv_label_set_text(l_sep, ":");
    lv_obj_set_style_text_font(l_sep, &lv_font_montserrat_18, 0);
    lv_obj_align(l_sep, LV_ALIGN_LEFT_MID, 156, 0);

    String opts_m = "";
    for (int i = 0; i < 60; i++) {
        char b[8]; snprintf(b, sizeof(b), "%02d", i);
        opts_m += b;
        if (i < 59) opts_m += "\n";
    }
    dd_time_m = lv_dropdown_create(cont_man_time);
    lv_dropdown_set_options(dd_time_m, opts_m.c_str());
    lv_dropdown_set_selected(dd_time_m, constrain(state.manual_min, 0, 59));
    lv_obj_set_width(dd_time_m, 75);
    lv_obj_align(dd_time_m, LV_ALIGN_LEFT_MID, 168, 0);

    lv_obj_t *btn_set_time = lv_btn_create(cont_man_time);
    lv_obj_set_size(btn_set_time, 130, 42);
    lv_obj_align(btn_set_time, LV_ALIGN_LEFT_MID, 255, 0);
    lv_obj_set_style_bg_color(btn_set_time, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_add_event_cb(btn_set_time, [](lv_event_t * e) {
        if (!dd_time_h || !dd_time_m) return;
        int h = lv_dropdown_get_selected(dd_time_h);
        int m = lv_dropdown_get_selected(dd_time_m);
        time_set_manual(h, m);
        state_save();
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *lbl_st = lv_label_create(btn_set_time);
    lv_label_set_text(lbl_st, "Uhrzeit setzen");
    lv_obj_center(lbl_st);

    lbl_time_status = lv_label_create(t_time);
    lv_obj_align(lbl_time_status, LV_ALIGN_TOP_LEFT, 0, 225);
    lv_obj_set_width(lbl_time_status, 440);
    lv_obj_set_style_text_font(lbl_time_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_time_status, ui_theme_muted(), 0);
    lv_label_set_text(lbl_time_status, "Aktuelle Systemzeit: --:--:--");

    // Virtual Keyboard (Global to scr_settings)
    kb = lv_keyboard_create(scr_settings);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, NULL);
    
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
        if (dd_out_temp) {
            state.outdoor_temp_sensor = lv_dropdown_get_selected(dd_out_temp);
            int out_idx = constrain(state.outdoor_temp_sensor, 0, 3);
            state.outdoor_temp = state.temp_sensors[out_idx];
            int in_idx = (out_idx == 0) ? 1 : 0;
            state.indoor_temp = state.temp_sensors[in_idx];
        }
        if (!state.time_auto_ntp && dd_time_h && dd_time_m) {
            state.manual_hour = lv_dropdown_get_selected(dd_time_h);
            state.manual_min = lv_dropdown_get_selected(dd_time_m);
        }
        time_apply_configuration();
        save_settings_cb(e);
    }, LV_EVENT_CLICKED, NULL);
    lv_obj_t *l_save = lv_label_create(btn_save);
    lv_label_set_text(l_save, "Speichern & Beenden");
    lv_obj_set_style_text_font(l_save, &lv_font_montserrat_18, 0);
    lv_obj_center(l_save);
}
