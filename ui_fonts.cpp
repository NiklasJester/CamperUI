#include "ui_fonts.h"
#include <Arduino.h>

// Undefine macros so this file can access the real library fonts
#undef lv_font_montserrat_12
#undef lv_font_montserrat_14
#undef lv_font_montserrat_16
#undef lv_font_montserrat_18
#undef lv_font_montserrat_20
#undef lv_font_montserrat_24
#undef lv_font_montserrat_28

lv_font_t ui_font_montserrat_12;
lv_font_t ui_font_montserrat_14;
lv_font_t ui_font_montserrat_16;
lv_font_t ui_font_montserrat_18;
lv_font_t ui_font_montserrat_20;
lv_font_t ui_font_montserrat_24;
lv_font_t ui_font_montserrat_28;

void ui_fonts_init(void) {
    ui_font_montserrat_12 = lv_font_montserrat_12;
    ui_font_montserrat_12.fallback = &ui_font_de_12;

    ui_font_montserrat_14 = lv_font_montserrat_14;
    ui_font_montserrat_14.fallback = &ui_font_de_14;

    ui_font_montserrat_16 = lv_font_montserrat_16;
    ui_font_montserrat_16.fallback = &ui_font_de_16;

    ui_font_montserrat_18 = lv_font_montserrat_18;
    ui_font_montserrat_18.fallback = &ui_font_de_18;

    ui_font_montserrat_20 = lv_font_montserrat_20;
    ui_font_montserrat_20.fallback = &ui_font_de_20;

    ui_font_montserrat_24 = lv_font_montserrat_24;
    ui_font_montserrat_24.fallback = &ui_font_de_24;

    ui_font_montserrat_28 = lv_font_montserrat_28;
    ui_font_montserrat_28.fallback = &ui_font_de_28;
}

