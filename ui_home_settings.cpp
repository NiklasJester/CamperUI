#include "ui_main.h"
namespace {
lv_obj_t *temp_select[3], *favorite_select[2], *tank_select;
bool updating = false;
lv_obj_t *home_settings_screen = nullptr;
void set_options(lv_obj_t *dropdown, const String &options, int selected) {
    if (!dropdown) return;
    if (strcmp(lv_dropdown_get_options(dropdown), options.c_str()) != 0)
        lv_dropdown_set_options(dropdown, options.c_str());
    if (lv_dropdown_get_selected(dropdown) != selected) lv_dropdown_set_selected(dropdown, selected);
}
void choice_changed(lv_event_t *event) {
    if (updating) return;
    int field = (int)(uintptr_t)lv_event_get_user_data(event);
    int selected = lv_dropdown_get_selected(lv_event_get_target(event));
    if (field < 3) state.home_temp_source[field] = selected - 1;
    else if (field < 5) state.home_favorite[field - 3] = selected;
    else state.home_water_source = selected;
    state_save(); ui_home_layout_changed();
}
void visibility_changed(lv_event_t *event) {
    bool *setting = (bool *)lv_event_get_user_data(event);
    *setting = lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED);
    state_save(); ui_home_layout_changed();
}
void toggle_card(lv_obj_t *parent, const char *text, int x, int y, bool *setting) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_size(card, 212, 44);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_hor(card, 8, 0);
    lv_obj_set_style_pad_ver(card, 4, 0);
    lv_obj_set_style_bg_color(card, ui_theme_card(), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, ui_theme_border(), 0);
    lv_obj_set_style_radius(card, 8, 0);

    lv_obj_t *sw = lv_switch_create(card);
    lv_obj_set_size(sw, 46, 26);
    lv_obj_align(sw, LV_ALIGN_LEFT_MID, 0, 0);
    if (*setting) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, visibility_changed, LV_EVENT_VALUE_CHANGED, setting);

    lv_obj_t *lbl = lv_label_create(card);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl, ui_theme_text(), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 54, 0);

    lv_obj_add_event_cb(card, [](lv_event_t *e) {
        lv_obj_t *s = (lv_obj_t *)lv_event_get_user_data(e);
        if (lv_obj_has_state(s, LV_STATE_CHECKED)) lv_obj_clear_state(s, LV_STATE_CHECKED);
        else lv_obj_add_state(s, LV_STATE_CHECKED);
        lv_event_send(s, LV_EVENT_VALUE_CHANGED, NULL);
    }, LV_EVENT_CLICKED, sw);
}
lv_obj_t *choice(lv_obj_t *parent, const char *title, int y, int field) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(label, 0, y + 12);
    lv_obj_t *obj = lv_dropdown_create(parent);
    lv_obj_set_pos(obj, 145, y);
    lv_obj_set_size(obj, 285, 44);
    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, 0);
    lv_obj_add_event_cb(obj, choice_changed, LV_EVENT_VALUE_CHANGED, (void *)(uintptr_t)field);
    return obj;
}
}
void ui_home_settings_refresh() {
    updating = true;
    String options = "Nicht anzeigen";
    for (int i = 0; i < 4; ++i) options += "\nTemp " + String(i + 1) + ": " + state.temp_sensor_names[i];
    for (int i = 0; i < 3; ++i) set_options(temp_select[i], options, state.home_temp_source[i] + 1);
    options = "Nicht anzeigen";
    for (int i = 0; i < 8; ++i) options += "\nRelais " + String(i + 1) + ": " + state.switch_names[i];
    for (int i = 0; i < 8; ++i) options += "\nDimmer " + String(i + 1) + ": " + state.dimmer_names[i];
    for (int i = 0; i < 2; ++i) set_options(favorite_select[i], options, state.home_favorite[i]);
    options = "";
    for (int i = 0; i < 4; ++i) {
        if (i) options += "\n";
        options += "Tank " + String(i + 1) + ": " + state.tank_names[i];
    }
    set_options(tank_select, options, state.home_water_source);
    updating = false;
}
void ui_build_home_settings(lv_obj_t *parent) {
    lv_obj_set_style_pad_all(parent, 12, 0);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Home Dashboard");
    lv_obj_set_pos(title, 0, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(UI_COLOR_PRIMARY), 0);
    toggle_card(parent, "Batterie", 0, 36, &state.home_show_battery);
    toggle_card(parent, "Solar", 220, 36, &state.home_show_solar);
    toggle_card(parent, "Wassertank", 0, 88, &state.home_show_water);
    toggle_card(parent, "Starterspannung", 220, 88, &state.home_show_starter);
    lv_obj_t *note = lv_label_create(parent);
    lv_obj_set_pos(note, 0, 142);
    lv_obj_set_width(note, 426);
    lv_label_set_text(note, "Sofort gespeichert. Dummy/Live gilt für alle Seiten.\nTemp 3 ist im Dummy Feuchte; Anzeige dann in %.");
    lv_obj_set_style_text_font(note, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(note, ui_theme_muted(), 0);
    const char *names[3] = {"Temperaturfeld 1", "Temperaturfeld 2", "Temperaturfeld 3"};
    for (int i = 0; i < 3; ++i) temp_select[i] = choice(parent, names[i], 194 + i * 50, i);
    for (int i = 0; i < 2; ++i) favorite_select[i] = choice(parent, i ? "Favorit 2" : "Favorit 1", 354 + i * 50, 3 + i);
    tank_select = choice(parent, "Tankquelle", 464, 5);
    ui_home_settings_refresh();
}
bool ui_home_settings_is_active() {
    return home_settings_screen && lv_scr_act() == home_settings_screen;
}
void ui_home_settings_destroy() {
    if (home_settings_screen) lv_obj_del(home_settings_screen);
    home_settings_screen = nullptr;
    for (auto &obj : temp_select) obj = nullptr;
    for (auto &obj : favorite_select) obj = nullptr;
    tank_select = nullptr;
}
void ui_open_home_settings() {
    if (!home_settings_screen) {
        home_settings_screen = lv_obj_create(nullptr);
        lv_obj_clear_flag(home_settings_screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(home_settings_screen, ui_theme_bg(), 0);
        lv_obj_set_style_text_font(home_settings_screen, &lv_font_montserrat_14, 0);
        lv_obj_t *back = lv_btn_create(home_settings_screen);
        lv_obj_set_pos(back, 12, 10);
        lv_obj_set_size(back, 105, 42);
        lv_obj_add_event_cb(back, [](lv_event_t *) {
            lv_scr_load(scr_main);
            ui_home_settings_destroy();
        }, LV_EVENT_CLICKED, nullptr);
        lv_obj_t *back_label = lv_label_create(back);
        lv_label_set_text(back_label, LV_SYMBOL_LEFT " Zurück");
        lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, 0);
        lv_obj_center(back_label);
        lv_obj_t *title = lv_label_create(home_settings_screen);
        lv_label_set_text(title, "Home-Einstellungen");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(title, ui_theme_text(), 0);
        lv_obj_set_pos(title, 132, 21);
        lv_obj_t *content = lv_obj_create(home_settings_screen);
        lv_obj_set_pos(content, 0, 60); lv_obj_set_size(content, 480, 420);
        lv_obj_set_style_bg_color(content, ui_theme_bg(), 0);
        lv_obj_set_style_text_color(content, ui_theme_text(), 0);
        lv_obj_set_style_border_width(content, 0, 0);
        lv_obj_set_style_radius(content, 0, 0);
        ui_build_home_settings(content);
    }
    ui_home_settings_refresh();
    lv_scr_load(home_settings_screen);
}
