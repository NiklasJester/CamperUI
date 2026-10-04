#include "ui_main.h"

// Layout preview only: fixed sample values, no HTTP or relay commands.
namespace {
lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y,
                const lv_font_t *font, uint32_t color) {
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, ((color == 0xf8fafc) ? ui_theme_text() : ((color == 0x94a3b8 || color == 0xb9c0cd) ? ui_theme_muted() : lv_color_hex(color))), 0);
    lv_obj_set_pos(obj, x, y);
    return obj;
}
lv_obj_t *card(lv_obj_t *parent, int x, int y, int w, int h) {
    lv_obj_t *obj = ui_create_card(parent, w, h);
    lv_obj_set_pos(obj, x, y);
    return obj;
}
}

void ui_build_home(lv_obj_t *parent) {
    // Static layout preview, independent of live VanPi state.
    lv_obj_set_style_pad_all(parent, 8, 0);
    lv_obj_set_style_bg_color(parent, ui_theme_bg(), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    label(parent, "HOME", 0, 0, &lv_font_montserrat_12, 0x94a3b8);
    lv_obj_t *version = label(parent, "Demo " CAMPERUI_VERSION, 0, 0, &lv_font_montserrat_12, 0x94a3b8);
    lv_obj_align(version, LV_ALIGN_TOP_RIGHT, 0, 0);

    const char *names[] = {"Taster Favorit 1", "Taster Favorit 2"};
    for (int i = 0; i < 2; ++i) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_set_pos(btn, i * 236, 22);
        lv_obj_set_size(btn, 228, 44);
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x263345), 0);
        // Local demo toggle only; no hardware commands are sent.
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
        lv_obj_set_style_bg_color(btn, lv_color_hex(UI_COLOR_PRIMARY), LV_STATE_CHECKED);
        lv_obj_set_style_text_color(btn, lv_color_hex(0xffffff), LV_STATE_CHECKED);
        lv_obj_t *text = lv_label_create(btn);
        lv_label_set_text(text, names[i]);
        lv_obj_set_style_text_font(text, &lv_font_montserrat_16, 0);
        lv_obj_center(text);
        // LVGL toggles LV_STATE_CHECKED on tap. No hardware callback.
    }

    const char *temps[] = {"Innen", "Aussen", "Kuehlschrank"};
    const char *values[] = {"23.9 C", "16.2 C", "4.5 C"};
    for (int i = 0; i < 3; ++i) {
        lv_obj_t *c = card(parent, i * 157, 74, 149, 70);
        label(c, temps[i], 0, 0, &lv_font_montserrat_12, 0xb9c0cd);
        label(c, values[i], 0, 16, &lv_font_montserrat_22, 0xf8fafc);
    }

    lv_obj_t *battery = card(parent, 0, 152, 240, 122);
    label(battery, "Batterie   73%", 0, 0, &lv_font_montserrat_16, 0xc7ce55);
    label(battery, "-16.1 W", 0, 25, &lv_font_montserrat_28, 0xc7ce55);
    label(battery, "13.4 V      -1.2 A", 0, 58, &lv_font_montserrat_16, 0xf8fafc);
    label(battery, MDI_BATTERY, 0, 78, &ui_font_mdi_18, 0xb9c0cd);
    label(battery, "Starter 12.7 V", 24, 80, &lv_font_montserrat_12, 0xb9c0cd);

    lv_obj_t *solar = card(parent, 248, 152, 216, 122);
    label(solar, "Solar", 0, 0, &lv_font_montserrat_16, 0xb9c0cd);
    label(solar, "120 W", 0, 25, &lv_font_montserrat_28, 0xf8fafc);
    label(solar, "27.2 V    4.4 A", 0, 58, &lv_font_montserrat_16, 0xf8fafc);
    label(solar, "PV-Leistung", 0, 80, &lv_font_montserrat_12, 0xb9c0cd);

    lv_obj_t *water = card(parent, 0, 282, 464, 60);
    label(water, "Frischwasser", 0, 0, &lv_font_montserrat_16, 0xb8ccfa);
    label(water, "22%", 360, 0, &lv_font_montserrat_22, 0xf8fafc);
    lv_obj_t *bar = lv_bar_create(water);
    lv_obj_set_pos(bar, 0, 26);
    lv_obj_set_size(bar, 438, 8);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 22, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x233249), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x709bd4), LV_PART_INDICATOR);
}
