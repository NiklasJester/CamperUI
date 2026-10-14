#include "ui_screensaver.h"
#include "system_state.h"
#include "WS_CH32_IO.h"
#include "ui_fonts.h"
#include <Wire.h>
#include <time.h>
#include "HWCDC.h"

static lv_obj_t *screensaver_cont = nullptr;
static lv_obj_t *lbl_time = nullptr;
static lv_obj_t *lbl_ttgo = nullptr;
static lv_obj_t *lbl_sub = nullptr;
static bool screensaver_active = false;

void ui_screensaver_init() {
    if (screensaver_cont) return;

    // Build screensaver overlay on LVGL system layer (sits above all screens)
    screensaver_cont = lv_obj_create(lv_layer_sys());
    lv_obj_set_size(screensaver_cont, 480, 480);
    lv_obj_set_pos(screensaver_cont, 0, 0);
    lv_obj_set_style_bg_color(screensaver_cont, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screensaver_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screensaver_cont, 0, 0);
    lv_obj_set_style_radius(screensaver_cont, 0, 0);
    lv_obj_set_style_pad_all(screensaver_cont, 0, 0);
    lv_obj_clear_flag(screensaver_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Flex centered content container
    lv_obj_t *box = lv_obj_create(screensaver_cont);
    lv_obj_set_size(box, 440, 260);
    lv_obj_center(box);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(box, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(box, 14, 0);

    // Large Clock in dark red (#dc2626)
    lbl_time = lv_label_create(box);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_42, 0);
    lv_obj_set_style_text_color(lbl_time, lv_color_hex(0xdc2626), 0);
    lv_label_set_text(lbl_time, "--:--");

    // Time to go in deep red (#b91c1c)
    lbl_ttgo = lv_label_create(box);
    lv_obj_set_style_text_font(lbl_ttgo, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_ttgo, lv_color_hex(0xb91c1c), 0);
    lv_label_set_text(lbl_ttgo, "Time to go: --");

    // Battery / Voltage status subtitle (#7f1d1d)
    lbl_sub = lv_label_create(box);
    lv_obj_set_style_text_font(lbl_sub, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_sub, lv_color_hex(0x7f1d1d), 0);
    lv_label_set_text(lbl_sub, "");

    // Safety touch handler on container
    lv_obj_add_event_cb(screensaver_cont, [](lv_event_t *) {
        ui_screensaver_exit();
    }, LV_EVENT_PRESSED, nullptr);

    // Initially hidden
    lv_obj_add_flag(screensaver_cont, LV_OBJ_FLAG_HIDDEN);
}

bool ui_screensaver_is_active() {
    return screensaver_active;
}

void ui_screensaver_update() {
    if (!screensaver_cont) return;

    time_t now = time(nullptr);
    struct tm ti;
    if (now > 100000 && localtime_r(&now, &ti)) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d", ti.tm_hour, ti.tm_min);
        lv_label_set_text(lbl_time, buf);
    } else {
        lv_label_set_text(lbl_time, "--:--");
    }

    if (state.time_to_go.length() > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Time to go: %s", state.time_to_go.c_str());
        lv_label_set_text(lbl_ttgo, buf);
    } else {
        lv_label_set_text(lbl_ttgo, "Time to go: --");
    }

    if (lbl_sub) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Batterie: %d%% · %.1f V", state.bat_soc, state.bat_voltage);
        lv_label_set_text(lbl_sub, buf);
    }
}

void ui_screensaver_enter() {
    if (screensaver_active) return;
    screensaver_active = true;

    if (screensaver_cont) {
        ui_screensaver_update();
        lv_obj_clear_flag(screensaver_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(screensaver_cont);
    }

    // Set display brightness to 1% (255 is OFF, 0 is 100% -> 253 is 1%)
    WS_CH32_IO::setPwm(Wire, 253);
    Serial.println("[SCREENSAVER] Activated (backlight 1%)");
}

void ui_screensaver_exit() {
    if (!screensaver_active) return;
    screensaver_active = false;

    if (screensaver_cont) {
        lv_obj_add_flag(screensaver_cont, LV_OBJ_FLAG_HIDDEN);
    }

    // Restore user-configured brightness
    uint8_t pwm = 255 - (state.display_brightness * 255 / 100);
    WS_CH32_IO::setPwm(Wire, pwm);
    lv_disp_trig_activity(nullptr);
    lv_obj_invalidate(lv_scr_act());
    Serial.println("[SCREENSAVER] Deactivated (brightness restored)");
}
