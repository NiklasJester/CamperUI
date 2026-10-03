#include "ui_main.h"

// Layout preview only: fixed sample values, no HTTP or relay commands.
namespace {
lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y,
                const lv_font_t *font, uint32_t color) {
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    lv_obj_set_pos(obj, x, y);
    return obj;
}
lv_obj_t *card(lv_obj_t *parent, int x, int y, int w, int h, uint32_t border) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(obj, 10, 0);
    lv_obj_set_style_radius(obj, 14, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x191d25), 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(border), 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    return obj;
}
}

void ui_build_home(lv_obj_t *parent) {
    // Fixed dark layout deliberately independent of live VanPi state.
    lv_obj_set_style_pad_all(parent, 8, 0);
    lv_obj_set_style_bg_color(parent, lv_color_hex(0x101319), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
    lv_obj_set_scroll_dir(parent, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    label(parent, "HOME", 4, 0, &lv_font_montserrat_12, 0x94a3b8);
    label(parent, "Demo", 396, 0, &lv_font_montserrat_12, 0x94a3b8);

    const char *names[] = {"Wechselrichter", "Wasserpumpe"};
    for (int i = 0; i < 2; ++i) {
        lv_obj_t *btn = lv_btn_create(parent);
        lv_obj_set_pos(btn, i * 228, 20);
        lv_obj_set_size(btn, 220, 44);
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x263345), 0);
        lv_obj_t *text = lv_label_create(btn);
        lv_label_set_text(text, names[i]);
        lv_obj_set_style_text_font(text, &lv_font_montserrat_16, 0);
        lv_obj_center(text);
        // No callback: these buttons cannot switch hardware.
    }

    const char *temps[] = {"Innen", "Aussen", "Kuehlschrank"};
    const char *values[] = {"23.9 C", "16.2 C", "4.5 C"};
    for (int i = 0; i < 3; ++i) {
        lv_obj_t *c = card(parent, i * 152, 68, 144, 62, 0xc5cad3);
        label(c, temps[i], 0, 0, &lv_font_montserrat_12, 0xb9c0cd);
        label(c, values[i], 0, 20, &lv_font_montserrat_22, 0xf8fafc);
    }

    lv_obj_t *battery = card(parent, 0, 138, 232, 122, 0x94733f);
    label(battery, "Batterie   73%", 0, 0, &lv_font_montserrat_16, 0xc7ce55);
    label(battery, "-16.1 W", 0, 25, &lv_font_montserrat_28, 0xc7ce55);
    label(battery, "13.4 V      -1.2 A", 0, 62, &lv_font_montserrat_16, 0xf8fafc);
    label(battery, "Rest: 1 T 19 h", 0, 86, &lv_font_montserrat_12, 0xb9c0cd);

    lv_obj_t *solar = card(parent, 240, 138, 208, 122, 0x3c4655);
    label(solar, "Solar", 0, 0, &lv_font_montserrat_16, 0xb9c0cd);
    label(solar, "120 W", 0, 25, &lv_font_montserrat_28, 0xf8fafc);
    label(solar, "27.2 V    4.4 A", 0, 62, &lv_font_montserrat_16, 0xf8fafc);
    label(solar, "PV-Leistung", 0, 86, &lv_font_montserrat_12, 0xb9c0cd);

    lv_obj_t *water = card(parent, 0, 268, 448, 60, 0x44638c);
    label(water, "Frischwasser", 0, 0, &lv_font_montserrat_16, 0xb8ccfa);
    label(water, "22%", 360, 0, &lv_font_montserrat_22, 0xf8fafc);
    lv_obj_t *bar = lv_bar_create(water);
    lv_obj_set_pos(bar, 0, 30);
    lv_obj_set_size(bar, 424, 8);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 22, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x233249), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x709bd4), LV_PART_INDICATOR);
}
