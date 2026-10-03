#include "ui_main.h"
#include "ui_maxxfan_assets.h"

// Interactive layout preview. No HTTP requests or hardware commands.
namespace {
bool lid_open = false, running = false, intake = false, automatic = false;
int speed = 3, target = 26;
lv_obj_t *lid_label, *speed_label, *target_label, *power_label;
lv_obj_t *open_button, *close_button, *power_button, *out_button, *in_button, *auto_switch;
lv_obj_t *lid_image;

lv_obj_t *text(lv_obj_t *parent, const char *value, int x, int y,
               const lv_font_t *font = &lv_font_montserrat_14) {
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, value);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, ui_theme_text(), 0);
    return obj;
}
lv_obj_t *panel(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *obj = ui_create_card(parent, w, h);
    lv_obj_set_pos(obj, x, y);
    return obj;
}
void checked(lv_obj_t *obj, bool on) {
    if (on) lv_obj_add_state(obj, LV_STATE_CHECKED);
    else lv_obj_clear_state(obj, LV_STATE_CHECKED);
}
void refresh() {
    lv_label_set_text(lid_label, lid_open ? "OFFEN" : "ZU");
    lv_label_set_text_fmt(speed_label, "%d", speed);
    lv_label_set_text_fmt(target_label, "%d C", target);
    lv_label_set_text(power_label, running ? LV_SYMBOL_POWER " EIN" : LV_SYMBOL_POWER " AUS");
    checked(open_button, lid_open);
    checked(close_button, !lid_open);
    checked(power_button, running);
    checked(out_button, !intake);
    checked(in_button, intake);
    checked(auto_switch, automatic);
    lv_img_set_src(lid_image, lid_open ? &ui_maxxfan_open : &ui_maxxfan_closed);
    ui_update_data(); // Also refresh the global fan indicator.
}
enum Action { OPEN, CLOSE, MINUS, PLUS, POWER, OUT, IN, TEMP_MINUS, TEMP_PLUS, RESET };
void clicked(lv_event_t *event) {
    switch ((uintptr_t)lv_event_get_user_data(event)) {
        case OPEN: lid_open = true; break;
        case CLOSE: lid_open = false; break;
        case MINUS: if (speed > 1) --speed; break;
        case PLUS: if (speed < 10) ++speed; break;
        case POWER: running = !running; break;
        case OUT: intake = false; break;
        case IN: intake = true; break;
        case TEMP_MINUS: if (target > 10) --target; break;
        case TEMP_PLUS: if (target < 40) ++target; break;
        case RESET:
            lid_open = running = intake = automatic = false;
            speed = 3; target = 26; break;
    }
    refresh();
}
void auto_changed(lv_event_t *event) {
    automatic = lv_obj_has_state(lv_event_get_target(event), LV_STATE_CHECKED);
    refresh();
}
lv_obj_t *button(lv_obj_t *parent, const char *value, int x, int y,
                 int w, int h, Action action, lv_obj_t **label_out = nullptr) {
    lv_obj_t *obj = lv_btn_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 10, 0);
    lv_obj_set_style_bg_color(obj, ui_theme_track(), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(UI_COLOR_PRIMARY), LV_STATE_CHECKED);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_border_color(obj, ui_theme_border(), 0);
    lv_obj_set_style_shadow_width(obj, 0, 0);
    lv_obj_t *lbl = text(obj, value, 0, 0);
    lv_obj_center(lbl);
    if (label_out) *label_out = lbl;
    lv_obj_add_event_cb(obj, clicked, LV_EVENT_CLICKED, (void *)(uintptr_t)action);
    return obj;
}
}

bool ui_maxxfan_running() { return running; }

void ui_build_maxxfan(lv_obj_t *parent) {
    lv_obj_set_style_pad_all(parent, 8, 0);
    lv_obj_set_style_border_width(parent, 0, 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, 0);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    text(parent, "MAXXFAN", 0, 0);
    text(parent, "Demo", 414, 0, &lv_font_montserrat_12);

    lv_obj_t *lid = panel(parent, 0, 26, 464, 80);
    lid_image = lv_img_create(lid);
    lv_obj_set_pos(lid_image, 0, 0);
    lv_obj_set_size(lid_image, 140, 54);
    text(lid, "Klappe", 150, 4, &lv_font_montserrat_12);
    lid_label = text(lid, "ZU", 150, 24, &lv_font_montserrat_22);
    open_button = button(lid, LV_SYMBOL_UP "\nAUF", 240, 0, 92, 54, OPEN);
    close_button = button(lid, LV_SYMBOL_DOWN "\nZU", 340, 0, 92, 54, CLOSE);

    lv_obj_t *fan = panel(parent, 0, 114, 352, 58);
    text(fan, MDI_FAN, 0, 6, &ui_font_mdi_18);
    text(fan, "Stufe", 24, 6);
    button(fan, LV_SYMBOL_MINUS, 100, 0, 48, 32, MINUS);
    speed_label = text(fan, "3", 178, 0, &lv_font_montserrat_28);
    button(fan, LV_SYMBOL_PLUS, 250, 0, 48, 32, PLUS);
    power_button = button(parent, LV_SYMBOL_POWER " AUS", 360, 114, 104, 58, POWER, &power_label);

    lv_obj_t *direction = panel(parent, 0, 180, 464, 50);
    text(direction, "Luftrichtung", 0, 4, &lv_font_montserrat_12);
    out_button = button(direction, LV_SYMBOL_UP " OUT", 132, 0, 146, 24, OUT);
    in_button = button(direction, LV_SYMBOL_DOWN " IN", 286, 0, 146, 24, IN);

    lv_obj_t *auto_panel = panel(parent, 0, 238, 464, 60);
    text(auto_panel, "Automatik", 0, 0, &lv_font_montserrat_12);
    text(auto_panel, "Zieltemperatur", 0, 18, &lv_font_montserrat_12);
    button(auto_panel, LV_SYMBOL_MINUS, 122, 0, 42, 34, TEMP_MINUS);
    target_label = text(auto_panel, "26 C", 176, 5, &lv_font_montserrat_22);
    button(auto_panel, LV_SYMBOL_PLUS, 244, 0, 42, 34, TEMP_PLUS);
    text(auto_panel, "AUTO", 298, 9, &lv_font_montserrat_12);
    auto_switch = lv_switch_create(auto_panel);
    lv_obj_set_pos(auto_switch, 354, 4);
    lv_obj_set_size(auto_switch, 70, 26);
    lv_obj_add_event_cb(auto_switch, auto_changed, LV_EVENT_VALUE_CHANGED, nullptr);

    // MaxxFan-specific settings are deferred until the VanPi mapping is known.
    lv_obj_t *settings = panel(parent, 0, 306, 228, 38);
    text(settings, "Einstellungen (folgen)", 0, 0, &lv_font_montserrat_12);
    button(parent, LV_SYMBOL_REFRESH " Reset Demo", 236, 306, 228, 38, RESET);
    refresh();
}
