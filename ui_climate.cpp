#include "ui_main.h"
#include "http_handler.h"

static lv_obj_t *lbl_indoor;
static lv_obj_t *lbl_outdoor;
static lv_obj_t *lbl_target;
static lv_obj_t *arc_target;
static lv_obj_t *btn_start_stop;
static lv_obj_t *lbl_btn_start_stop;
static lv_obj_t *dd_mode;
static lv_obj_t *lbl_heater_status;

#define HEATER_HOLD_MS 5000

// Translate the raw Autoterm "heatstatus" text from VanPi into a short German label.
// Unknown values are shown as-is.
static String heater_status_text(const String &raw) {
    String s = raw; s.trim();
    String l = s; l.toLowerCase();
    if (l.length() == 0 || l == "wait")         return "Warte auf Daten...";
    if (l == "standby" || l == "heater off")    return "Standby";
    if (l == "heating" || l == "running")       return "Heizt";
    if (l == "ventilation" || l == "only fan")  return "Lueftet";
    if (l.indexOf("glow plug") >= 0)            return "Gluehkerze vorheizen";
    if (l.indexOf("ignition 1") >= 0)           return "Zuendung 1";
    if (l.indexOf("ignition 2") >= 0)           return "Zuendung 2";
    if (l == "starting")                        return "Startet";
    if (l == "warming up")                      return "Aufwaermen";
    if (l == "cooling flame sensor")            return "Flammsensor kuehlt";
    if (l == "cooling down")                    return "Abkuehlen";
    if (l == "shutting down")                   return "Faehrt herunter";
    if (l == "flame-out")                       return "Flammabriss!";
    if (l == "no ignition error")               return "Fehler: Keine Zuendung";
    if (l.indexOf("no fuel") >= 0)              return "Kein Kraftstoff? Neuer Versuch";
    if (l == "unknown status")                  return "Status unbekannt";
    return s;
}

static void update_status_label() {
    if (!lbl_heater_status) return;
    String txt = heater_status_text(state.heater_status);
    String err = state.heater_error; err.trim();
    bool has_err = err.length() > 0 && err != "no" && err != "0" && err != "null";
    if (has_err) txt += " | Fehler: " + err;

    if (strcmp(lv_label_get_text(lbl_heater_status), txt.c_str()) != 0) {
        lv_label_set_text(lbl_heater_status, txt.c_str());
    }

    String l = state.heater_status; l.toLowerCase();
    lv_color_t c = ui_theme_muted();
    if (has_err || l.indexOf("error") >= 0 || l.indexOf("flame-out") >= 0 || l.indexOf("no fuel") >= 0) {
        c = lv_color_hex(UI_COLOR_DANGER);
    } else if (l == "heating" || l == "running" || l.indexOf("ignition") >= 0 || l.indexOf("glow") >= 0 ||
               l == "starting" || l == "warming up") {
        c = lv_color_hex(UI_COLOR_WARNING);
    } else if (l == "ventilation" || l == "only fan") {
        c = lv_color_hex(UI_COLOR_PRIMARY);
    }
    lv_obj_set_style_text_color(lbl_heater_status, c, 0);
}

static void update_arc_color(lv_obj_t *arc) {
    int val = lv_arc_get_value(arc);
    int pct = 0;
    
    if (state.heater_vent_mode) {
        pct = 0;
    } else {
        if (state.heater_power_mode) {
            pct = ((val - 1) * 255) / 9;
        } else {
            pct = ((val - 10) * 255) / 25;
        }
    }
    
    if (pct < 0) pct = 0;
    if (pct > 255) pct = 255;
    
    lv_color_t color;
    if (state.heater_vent_mode) {
        color = lv_color_hex(UI_COLOR_PRIMARY); // Blue for vent
    } else {
        color = lv_color_mix(lv_color_hex(UI_COLOR_DANGER), lv_color_hex(UI_COLOR_PRIMARY), pct);
    }
    
    lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_set_style_border_color(arc, color, LV_PART_KNOB);
    lv_obj_set_style_border_width(arc, 4, LV_PART_KNOB);
}

static void update_ui_from_mode() {
    if (state.heater_vent_mode) {
        lv_dropdown_set_selected(dd_mode, 2);
        lv_arc_set_range(arc_target, 1, 10);
        lv_arc_set_value(arc_target, state.heater_power_level > 0 ? state.heater_power_level : 1);
        lv_label_set_text_fmt(lbl_target, "Luefter Stufe: %d", lv_arc_get_value(arc_target));
    } else if (state.heater_power_mode) {
        lv_dropdown_set_selected(dd_mode, 1);
        lv_arc_set_range(arc_target, 1, 10);
        lv_arc_set_value(arc_target, state.heater_power_level > 0 ? state.heater_power_level : 1);
        lv_label_set_text_fmt(lbl_target, "Heiz Stufe: %d", lv_arc_get_value(arc_target));
    } else {
        lv_dropdown_set_selected(dd_mode, 0);
        lv_arc_set_range(arc_target, 10, 35);
        int t = (int)(state.target_temp + 0.5f);
        if (t < 10) t = 20;
        if (t > 35) t = 35;
        lv_arc_set_value(arc_target, t);
        lv_label_set_text_fmt(lbl_target, "Ziel: %d C", t);
    }
    update_arc_color(arc_target);
    
    if (state.heating_on) {
        lv_obj_set_style_bg_color(btn_start_stop, lv_color_hex(UI_COLOR_DANGER), 0);
        lv_obj_set_style_border_color(btn_start_stop, lv_color_hex(0xf87171), 0);
        lv_obj_set_style_shadow_color(btn_start_stop, lv_color_hex(UI_COLOR_DANGER), 0);
        lv_obj_set_style_shadow_opa(btn_start_stop, LV_OPA_30, 0);
        lv_label_set_text(lbl_btn_start_stop, "Stop");
    } else {
        lv_obj_set_style_bg_color(btn_start_stop, lv_color_hex(UI_COLOR_SUCCESS), 0);
        lv_obj_set_style_border_color(btn_start_stop, lv_color_hex(0x34d399), 0);
        lv_obj_set_style_shadow_color(btn_start_stop, lv_color_hex(UI_COLOR_SUCCESS), 0);
        lv_obj_set_style_shadow_opa(btn_start_stop, LV_OPA_30, 0);
        lv_label_set_text(lbl_btn_start_stop, "Start");
    }
}

static void dd_mode_event_cb(lv_event_t * e) {
    lv_obj_t * dd = lv_event_get_target(e);
    int sel = lv_dropdown_get_selected(dd);
    state.heater_hold_until = millis() + HEATER_HOLD_MS;
    
    if (sel == 0) { state.heater_vent_mode = false; state.heater_power_mode = false; }
    else if (sel == 1) { state.heater_vent_mode = false; state.heater_power_mode = true; }
    else if (sel == 2) { state.heater_vent_mode = true; state.heater_power_mode = false; }
    
    update_ui_from_mode();

    // Heater already running -> switch mode immediately
    if (state.heating_on) {
        String m = "temp";
        int val = (int)state.target_temp;
        if (state.heater_vent_mode) { m = "vent"; val = state.heater_power_level; }
        else if (state.heater_power_mode) { m = "power"; val = state.heater_power_level; }
        http_publish_heater_cmd(m, val);
    }
}

static void btn_start_stop_event_cb(lv_event_t * e) {
    state.heater_hold_until = millis() + HEATER_HOLD_MS;
    if (state.heating_on) {
        http_publish_heater_cmd("stop", 0);
        state.heating_on = false;
    } else {
        String m = "temp";
        int val = (int)state.target_temp;
        if (state.heater_vent_mode) { m = "vent"; val = state.heater_power_level; }
        else if (state.heater_power_mode) { m = "power"; val = state.heater_power_level; }
        
        http_publish_heater_cmd(m, val);
        state.heating_on = true;
    }
    update_ui_from_mode();
}

static void arc_target_event_cb(lv_event_t * e) {
    lv_obj_t * arc = lv_event_get_target(e);
    int val = lv_arc_get_value(arc);
    state.heater_hold_until = millis() + HEATER_HOLD_MS;
    
    if (state.heater_vent_mode) {
        state.heater_power_level = val;
        lv_label_set_text_fmt(lbl_target, "Luefter Stufe: %d", val);
    } else if (state.heater_power_mode) {
        state.heater_power_level = val;
        lv_label_set_text_fmt(lbl_target, "Heiz Stufe: %d", val);
    } else {
        state.target_temp = val;
        lv_label_set_text_fmt(lbl_target, "Ziel: %d C", val);
    }
    
    update_arc_color(arc);
    
    if(lv_event_get_code(e) == LV_EVENT_RELEASED) {
        if (state.heating_on) {
            String m = "temp";
            if (state.heater_vent_mode) m = "vent";
            else if (state.heater_power_mode) m = "power";
            http_publish_heater_cmd(m, val);
        }
    }
}

void ui_update_climate_tab() {
    if (!lbl_indoor) return;
    
    ui_label_set_float(lbl_indoor, "%.1f C", state.indoor_temp);
    if (lbl_outdoor) {
        ui_label_set_float(lbl_outdoor, "%.1f C", state.outdoor_temp);
    }

    update_status_label();

    // Don't touch controls while the user is interacting with them
    if (lv_obj_has_state(arc_target, LV_STATE_PRESSED) || lv_dropdown_is_open(dd_mode)) return;
    
    int expected_sel = 0;
    if (state.heater_vent_mode) expected_sel = 2;
    else if (state.heater_power_mode) expected_sel = 1;

    int expected_val;
    if (expected_sel == 0) {
        expected_val = (int)(state.target_temp + 0.5f);
        if (expected_val < 10) expected_val = 20;
        if (expected_val > 35) expected_val = 35;
    } else {
        expected_val = constrain(state.heater_power_level, 1, 10);
    }
    
    bool arc_needs_update = false;
    if (expected_sel != (int)lv_dropdown_get_selected(dd_mode)) arc_needs_update = true;
    if (lv_arc_get_value(arc_target) != expected_val) arc_needs_update = true;
    
    if (arc_needs_update) {
        update_ui_from_mode();
    } else {
        if (state.heating_on) {
            lv_obj_set_style_bg_color(btn_start_stop, lv_color_hex(UI_COLOR_DANGER), 0);
            lv_obj_set_style_border_color(btn_start_stop, lv_color_hex(0xf87171), 0);
            lv_obj_set_style_shadow_color(btn_start_stop, lv_color_hex(UI_COLOR_DANGER), 0);
            lv_obj_set_style_shadow_opa(btn_start_stop, LV_OPA_30, 0);
            lv_label_set_text(lbl_btn_start_stop, "Stop");
        } else {
            lv_obj_set_style_bg_color(btn_start_stop, lv_color_hex(UI_COLOR_SUCCESS), 0);
            lv_obj_set_style_border_color(btn_start_stop, lv_color_hex(0x34d399), 0);
            lv_obj_set_style_shadow_color(btn_start_stop, lv_color_hex(UI_COLOR_SUCCESS), 0);
            lv_obj_set_style_shadow_opa(btn_start_stop, LV_OPA_30, 0);
            lv_label_set_text(lbl_btn_start_stop, "Start");
        }
    }
}

void ui_build_climate(lv_obj_t *parent) {
    ui_setup_tab_page(parent);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // 1. Top Row: Outdoor Temp (Left) & Mode Dropdown (Right)
    // ==========================================
    // Left Badge: Outdoor Temp (as user liked before)
    lv_obj_t *card_out = ui_create_card(parent, 150, 58);
    lv_obj_align(card_out, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_pad_all(card_out, 8, 0);
    
    lv_obj_t *l_out_sub = lv_label_create(card_out);
    lv_label_set_text_fmt(l_out_sub, "%s Aussen", LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(l_out_sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l_out_sub, ui_theme_muted(), 0);
    lv_obj_align(l_out_sub, LV_ALIGN_TOP_LEFT, 4, 0);

    lbl_outdoor = lv_label_create(card_out);
    lv_label_set_text(lbl_outdoor, "--.- C");
    lv_obj_set_style_text_font(lbl_outdoor, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_outdoor, ui_theme_text(), 0);
    lv_obj_align(lbl_outdoor, LV_ALIGN_BOTTOM_LEFT, 4, -2);

    // Right: Mode Dropdown
    dd_mode = lv_dropdown_create(parent);
    lv_dropdown_set_options(dd_mode, "Heizen (Temp)\nHeizen (Stufe)\nLueften");
    lv_obj_set_size(dd_mode, 190, 58);
    lv_obj_align(dd_mode, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_radius(dd_mode, 14, 0);
    lv_obj_set_style_bg_color(dd_mode, ui_theme_card(), 0);
    lv_obj_set_style_border_color(dd_mode, ui_theme_border(), 0);
    lv_obj_set_style_border_width(dd_mode, 1, 0);
    lv_obj_set_style_text_color(dd_mode, ui_theme_text(), 0);
    lv_obj_set_style_text_font(dd_mode, &lv_font_montserrat_16, 0);
    lv_obj_set_style_pad_top(dd_mode, 19, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(dd_mode, 19, LV_PART_MAIN);
    lv_obj_set_style_pad_left(dd_mode, 14, LV_PART_MAIN);
    lv_obj_set_style_pad_right(dd_mode, 36, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(dd_mode, 8, 0);
    lv_obj_set_style_shadow_ofs_y(dd_mode, 3, 0);
    
    lv_obj_t * list = lv_dropdown_get_list(dd_mode);
    lv_obj_set_style_text_font(list, &lv_font_montserrat_18, 0);
    lv_obj_set_style_pad_all(list, 10, 0);
    lv_obj_set_style_bg_color(list, ui_theme_card(), 0);
    lv_obj_set_style_border_color(list, ui_theme_border(), 0);
    lv_obj_add_event_cb(dd_mode, dd_mode_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // ==========================================
    // 2. Thermostat Arc Gauge
    // ==========================================
    arc_target = lv_arc_create(parent);
    lv_obj_set_size(arc_target, 230, 230);
    lv_obj_align(arc_target, LV_ALIGN_TOP_MID, 0, 56);
    
    lv_obj_set_style_arc_width(arc_target, 16, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_target, ui_theme_track(), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_target, 16, LV_PART_INDICATOR);
    
    // Tactile knob with clear distinct white background and shadow
    lv_obj_set_style_radius(arc_target, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc_target, 7, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(arc_target, 10, LV_PART_KNOB);
    lv_obj_set_style_shadow_color(arc_target, lv_color_hex(0x000000), LV_PART_KNOB);
    lv_obj_set_style_shadow_opa(arc_target, LV_OPA_50, LV_PART_KNOB);
    
    lv_obj_add_event_cb(arc_target, arc_target_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(arc_target, arc_target_event_cb, LV_EVENT_RELEASED, NULL);

    // Indoor Temp Label (Dead-centered)
    lbl_indoor = lv_label_create(parent);
    lv_obj_set_width(lbl_indoor, 200);
    lv_obj_set_style_text_align(lbl_indoor, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lbl_indoor, "--.- C");
    lv_obj_set_style_text_font(lbl_indoor, &lv_font_montserrat_42, 0);
    lv_obj_set_style_text_color(lbl_indoor, ui_theme_text(), 0);
    lv_obj_align_to(lbl_indoor, arc_target, LV_ALIGN_CENTER, 0, -14);
    
    // Target Value Label (Dead-centered)
    lbl_target = lv_label_create(parent);
    lv_obj_set_width(lbl_target, 200);
    lv_obj_set_style_text_align(lbl_target, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(lbl_target, "Ziel: -- C");
    lv_obj_set_style_text_font(lbl_target, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_target, ui_theme_muted(), 0);
    lv_obj_align_to(lbl_target, arc_target, LV_ALIGN_CENTER, 0, 26);

    // Heater Status Text (below the arc, e.g. "Standby", "Heizt", "Zuendung 1")
    lbl_heater_status = lv_label_create(parent);
    lv_obj_set_width(lbl_heater_status, 400);
    lv_label_set_long_mode(lbl_heater_status, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(lbl_heater_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_heater_status, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_heater_status, ui_theme_muted(), 0);
    lv_label_set_text(lbl_heater_status, "");
    // Arc bottom = 56 + 230 = 286; the arc ends + knob reach down to ~267,
    // the Start/Stop button starts at 306 -> place label at ~268..288.
    lv_obj_align_to(lbl_heater_status, arc_target, LV_ALIGN_OUT_BOTTOM_MID, 0, -18);
    update_status_label();

    // ==========================================
    // 3. Controls (Bottom Row: Centered Start/Stop Button)
    // ==========================================
    btn_start_stop = lv_btn_create(parent);
    lv_obj_set_size(btn_start_stop, 260, 54);
    lv_obj_align(btn_start_stop, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_radius(btn_start_stop, 14, 0);
    lv_obj_set_style_border_width(btn_start_stop, 1, 0);
    lv_obj_set_style_shadow_width(btn_start_stop, 8, 0);
    lv_obj_set_style_shadow_ofs_y(btn_start_stop, 3, 0);
    lv_obj_add_event_cb(btn_start_stop, btn_start_stop_event_cb, LV_EVENT_CLICKED, NULL);
    
    lbl_btn_start_stop = lv_label_create(btn_start_stop);
    lv_label_set_text(lbl_btn_start_stop, "Start");
    lv_obj_set_style_text_font(lbl_btn_start_stop, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_btn_start_stop);
    
    update_ui_from_mode();
}
