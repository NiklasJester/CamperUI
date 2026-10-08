#include "ui_main.h"
#include "http_handler.h"
#include <math.h>
namespace {
lv_obj_t *root, *version, *mode_text;
lv_obj_t *favorites[2], *favorite_text[2], *temperatures[3], *temp_name[3], *temp_value[3];
lv_obj_t *battery, *bat_title, *bat_power, *bat_detail, *starter;
lv_obj_t *solar, *solar_power, *solar_detail, *water, *water_title, *water_value, *water_bar;
uint8_t remembered_dimmer[8] = {};
lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y, const lv_font_t *font) {
    lv_obj_t *obj = lv_label_create(parent); lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, font, 0); lv_obj_set_style_text_color(obj, ui_theme_text(), 0);
    lv_obj_set_pos(obj, x, y); return obj;
}
void visible(lv_obj_t *obj, bool show) {
    if (show) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}
void layout() {
    if (!root) return;
    int y = 32, count = (state.home_favorite[0] != 0) + (state.home_favorite[1] != 0), column = 0;
    for (int i = 0; i < 2; ++i) {
        bool show = state.home_favorite[i] != 0; visible(favorites[i], show); if (!show) continue;
        int width = count == 1 ? 464 : 228;
        lv_obj_set_pos(favorites[i], column++ * 236, y); lv_obj_set_width(favorites[i], width);
        lv_obj_set_width(favorite_text[i], width - 24); lv_obj_center(favorite_text[i]);
    }
    if (count) y += 44;
    count = column = 0;
    for (int i = 0; i < 3; ++i) if (state.home_temp_source[i] >= 0) ++count;
    for (int i = 0; i < 3; ++i) {
        bool show = state.home_temp_source[i] >= 0; visible(temperatures[i], show); if (!show) continue;
        int width = (464 - (count - 1) * 8) / count;
        lv_obj_set_pos(temperatures[i], column++ * (width + 8), y);
        lv_obj_set_width(temperatures[i], width); lv_obj_set_width(temp_name[i], width - 24);
    }
    if (count) y += 78;
    visible(battery, state.home_show_battery); visible(solar, state.home_show_solar);
    bool both = state.home_show_battery && state.home_show_solar;
    lv_obj_set_pos(battery, 0, y); lv_obj_set_width(battery, both ? 228 : 464);
    lv_obj_set_pos(solar, both ? 236 : 0, y); lv_obj_set_width(solar, both ? 228 : 464);
    visible(starter, state.home_show_starter);
    if (state.home_show_battery || state.home_show_solar) y += 130;
    visible(water, state.home_show_water); lv_obj_set_pos(water, 0, y);
    lv_obj_update_layout(root);
    lv_obj_scroll_to_y(root, lv_obj_get_scroll_y(root), LV_ANIM_OFF);
}
bool source_ready() { return state.data_is_demo == state.debug_mode; }
bool favorite_ready(int selection) {
    if (!source_ready() || selection < 1 || selection > 16) return false;
    if (!state.debug_mode && (!state.wifi_connected || !state.vanpi_connected)) return false;
    return selection <= 8 ? (state.relay_fields & (1 << (selection - 1))) != 0 :
                           (state.dimmer_fields & (1 << (selection - 9))) != 0;
}
void favorite_click(lv_event_t *event) {
    int slot = (int)(uintptr_t)lv_event_get_user_data(event), selection = state.home_favorite[slot];
    if (!favorite_ready(selection)) return;
    if (selection <= 8) http_publish_switch(selection - 1, !state.switch_state[selection - 1]);
    else {
        int index = selection - 9, value = state.dimmer_val[index];
        if (value > 0) remembered_dimmer[index] = value;
        int target = value > 0 ? 0 : remembered_dimmer[index] ? remembered_dimmer[index] : 100;
        state.dimmer_hold_until[index] = millis() + 2000; http_publish_dimmer(index, target);
    }
    ui_update_home();
}
void mode_click(lv_event_t *) {
    state.debug_mode = !state.debug_mode; ui_sync_demo_controls(); state_save(); ui_update_home(); ui_update_data();
}
void value(lv_obj_t *obj, const char *format, float number, bool ready) {
    if (ready && isfinite(number)) ui_label_set_float(obj, format, number);
    else ui_label_set_text_if_changed(obj, "--");
}
}
void ui_home_layout_changed() { layout(); }
void ui_update_home() {
    if (!root) return;
    char text[96];
    snprintf(text, sizeof(text), "%s %s", state.debug_mode ? "Demo" :
        state.vanpi_connected ? "Live" : "Live offline", CAMPERUI_VERSION);
    ui_label_set_text_if_changed(version, text);
    ui_label_set_text_if_changed(mode_text, state.debug_mode ? "Dummy > Live" : "Live > Dummy");
    bool ready = source_ready();
    for (int i = 0; i < 2; ++i) {
        int selection = state.home_favorite[i]; if (selection < 1 || selection > 16) continue;
        bool valid = favorite_ready(selection);
        bool on = selection <= 8 ? state.switch_state[selection - 1] : state.dimmer_val[selection - 9] > 0;
        const String &name = selection <= 8 ? state.switch_names[selection - 1] : state.dimmer_names[selection - 9];
        snprintf(text, sizeof(text), "%s %s", name.c_str(), !valid ? "--" : on ? "EIN" : "AUS");
        ui_label_set_text_if_changed(favorite_text[i], text);
        if (valid) lv_obj_clear_state(favorites[i], LV_STATE_DISABLED); else lv_obj_add_state(favorites[i], LV_STATE_DISABLED);
        if (valid && on) lv_obj_add_state(favorites[i], LV_STATE_CHECKED); else lv_obj_clear_state(favorites[i], LV_STATE_CHECKED);
    }
    for (int i = 0; i < 3; ++i) {
        int source = state.home_temp_source[i]; if (source < 0 || source > 3) continue;
        ui_label_set_text_if_changed(temp_name[i], state.temp_sensor_names[source].c_str());
        // Dummy temp3 contains humidity; never mislabel it as Celsius.
        value(temp_value[i], state.debug_mode && source == 2 ? "%.1f %%" : "%.1f C",
              state.temp_sensors[source], ready && (state.temp_fields & (1 << source)));
    }
    if (ready && (state.battery_fields & 4)) {
        snprintf(text, sizeof(text), "Batterie %d%%", state.bat_soc);
    } else {
        snprintf(text, sizeof(text), "Batterie --%%");
    }
    ui_label_set_text_if_changed(bat_title, text);
    ui_text_color_if_changed(bat_title, ready && (state.battery_fields & 4) ?
        lv_color_hex(state.bat_soc < 20 ? UI_COLOR_DANGER : state.bat_soc <= 40 ? 0xffdf00 : UI_COLOR_SUCCESS) : ui_theme_muted());
    value(bat_power, "%.1f W", state.bat_voltage * state.bat_current, ready && (state.battery_fields & 3) == 3);

    char v_buf[16], a_buf[16];
    if (ready && (state.battery_fields & 1)) snprintf(v_buf, sizeof(v_buf), "%.1f", state.bat_voltage);
    else strcpy(v_buf, "--");
    if (ready && (state.battery_fields & 2)) snprintf(a_buf, sizeof(a_buf), "%.1f", state.bat_current);
    else strcpy(a_buf, "--");
    snprintf(text, sizeof(text), "%s V   %s A", v_buf, a_buf);
    ui_label_set_text_if_changed(bat_detail, text);

    if (ready && (state.battery_fields & 8)) snprintf(v_buf, sizeof(v_buf), "%.1f", state.starter_voltage);
    else strcpy(v_buf, "--");
    snprintf(text, sizeof(text), "Starter %s V", v_buf);
    ui_label_set_text_if_changed(starter, text);

    value(solar_power, "%.0f W", state.solar_power, ready && (state.solar_fields & 1));

    if (ready && (state.solar_fields & 4)) snprintf(v_buf, sizeof(v_buf), "%.1f", state.solar_voltage);
    else strcpy(v_buf, "--");
    if (ready && (state.solar_fields & 2)) snprintf(a_buf, sizeof(a_buf), "%.1f", state.solar_current);
    else strcpy(a_buf, "--");
    snprintf(text, sizeof(text), "%s V   %s A", v_buf, a_buf);
    ui_label_set_text_if_changed(solar_detail, text);
    int tank = constrain(state.home_water_source, 0, 3); ui_label_set_text_if_changed(water_title, state.tank_names[tank].c_str());
    bool tank_ready = ready && (state.tank_fields & (1 << tank));
    snprintf(text, sizeof(text), "%d%%", state.tank_level[tank]); ui_label_set_text_if_changed(water_value, tank_ready ? text : "--");
    int level = tank_ready ? state.tank_level[tank] : 0;
    if (lv_bar_get_value(water_bar) != level) lv_bar_set_value(water_bar, level, LV_ANIM_OFF);
}
void ui_build_home(lv_obj_t *parent) {
    root = parent;
    lv_obj_set_style_pad_all(parent, 8, 0); lv_obj_set_style_bg_color(parent, ui_theme_bg(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0); lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO); lv_obj_add_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
    label(parent, "HOME", 0, 4, &lv_font_montserrat_12);
    lv_obj_t *mode = lv_btn_create(parent); lv_obj_set_pos(mode, 64, 0); lv_obj_set_size(mode, 136, 26);
    mode_text = label(mode, "Dummy > Live", 0, 0, &lv_font_montserrat_12); lv_obj_center(mode_text);
    lv_obj_add_event_cb(mode, mode_click, LV_EVENT_CLICKED, NULL);
    version = label(parent, "", 0, 4, &lv_font_montserrat_12); lv_obj_align(version, LV_ALIGN_TOP_RIGHT, 0, 4);
    for (int i = 0; i < 2; ++i) {
        favorites[i] = lv_btn_create(parent); lv_obj_set_height(favorites[i], 36);
        lv_obj_set_style_radius(favorites[i], 12, 0); lv_obj_set_style_bg_color(favorites[i], ui_theme_card(), 0);
        lv_obj_set_style_bg_color(favorites[i], lv_color_hex(UI_COLOR_PRIMARY), LV_STATE_CHECKED);
        lv_obj_set_style_text_color(favorites[i], lv_color_hex(0xffffff), LV_STATE_CHECKED);
        lv_obj_add_event_cb(favorites[i], favorite_click, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        favorite_text[i] = label(favorites[i], "--", 0, 0, &lv_font_montserrat_16);
        lv_label_set_long_mode(favorite_text[i], LV_LABEL_LONG_DOT);
        lv_obj_set_style_text_align(favorite_text[i], LV_TEXT_ALIGN_CENTER, 0);
    }
    for (int i = 0; i < 3; ++i) {
        temperatures[i] = ui_create_card(parent, 149, 70);
        temp_name[i] = label(temperatures[i], "--", 0, 0, &lv_font_montserrat_12);
        lv_label_set_long_mode(temp_name[i], LV_LABEL_LONG_DOT);
        temp_value[i] = label(temperatures[i], "--", 0, 16, &lv_font_montserrat_22);
    }
    battery = ui_create_card(parent, 240, 122);
    bat_title = label(battery, "Batterie --%", 0, 0, &lv_font_montserrat_16);
    bat_power = label(battery, "--", 0, 25, &lv_font_montserrat_28);
    lv_obj_set_style_text_color(bat_power, lv_color_hex(UI_COLOR_WARNING), 0);
    bat_detail = label(battery, "-- V   -- A", 0, 58, &lv_font_montserrat_16);
    starter = label(battery, "Starter -- V", 0, 80, &lv_font_montserrat_12);
    solar = ui_create_card(parent, 216, 122); label(solar, "Solar", 0, 0, &lv_font_montserrat_16);
    solar_power = label(solar, "--", 0, 25, &lv_font_montserrat_28);
    lv_obj_set_style_text_color(solar_power, lv_color_hex(UI_COLOR_WARNING), 0);
    solar_detail = label(solar, "-- V   -- A", 0, 58, &lv_font_montserrat_16);
    label(solar, "PV-Leistung", 0, 80, &lv_font_montserrat_12);
    water = ui_create_card(parent, 464, 60);
    water_title = label(water, "--", 0, 0, &lv_font_montserrat_16);
    water_value = label(water, "--", 360, 0, &lv_font_montserrat_22);
    water_bar = lv_bar_create(water); lv_obj_set_pos(water_bar, 0, 26); lv_obj_set_size(water_bar, 438, 8);
    lv_bar_set_range(water_bar, 0, 100); lv_obj_set_style_bg_color(water_bar, ui_theme_track(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(water_bar, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_INDICATOR);
    layout();
}
