#include "ui_main.h"
#include "http_handler.h"

static lv_obj_t *switches[8];
static lv_obj_t *lbl_switch_name[8];
static lv_obj_t *w_switches[8];
static lv_obj_t *lbl_wswitch_name[8];
static lv_obj_t *dimmers[8];
static lv_obj_t *lbl_dim_name[8];
static lv_obj_t *lbl_dim_pct[8];

static void update_dimmer_visuals(int idx) {
    if (!dimmers[idx]) return;
    int val = state.dimmer_val[idx];
    
    if (lbl_dim_pct[idx]) {
        if (val > 0) {
            lv_label_set_text_fmt(lbl_dim_pct[idx], "%d%%", val);
            lv_obj_set_style_text_color(lbl_dim_pct[idx], lv_color_hex(UI_COLOR_WARNING), 0);
        } else {
            lv_label_set_text(lbl_dim_pct[idx], "Aus");
            lv_obj_set_style_text_color(lbl_dim_pct[idx], ui_theme_muted(), 0);
        }
    }
    
    if (val > 0) {
        lv_color_t color = lv_color_hex(UI_COLOR_WARNING);
        lv_obj_set_style_bg_color(dimmers[idx], color, LV_PART_INDICATOR);
    } else {
        lv_color_t off_color = ui_theme_track();
        lv_obj_set_style_bg_color(dimmers[idx], off_color, LV_PART_INDICATOR);
    }
}

static int last_switch_state[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
static int last_wswitch_state[8] = {-1, -1, -1, -1, -1, -1, -1, -1};

static void update_switch_visual(int idx, bool force = false) {
    if (!switches[idx]) return;
    bool on = state.switch_state[idx];
    if (!force && last_switch_state[idx] == (int)on) return;
    last_switch_state[idx] = (int)on;

    if (on) {
        lv_obj_set_style_bg_color(switches[idx], lv_color_hex(UI_COLOR_SUCCESS), 0);
        lv_obj_set_style_border_color(switches[idx], lv_color_hex(0x34d399), 0);
        if (lbl_switch_name[idx]) {
            ui_text_color_if_changed(lbl_switch_name[idx], lv_color_hex(0xffffff));
        }
    } else {
        lv_obj_set_style_bg_color(switches[idx], ui_theme_card(), 0);
        lv_obj_set_style_border_color(switches[idx], ui_theme_border(), 0);
        if (lbl_switch_name[idx]) {
            ui_text_color_if_changed(lbl_switch_name[idx], ui_theme_text());
        }
    }
}

static void update_wswitch_visual(int idx, bool force = false) {
    if (!w_switches[idx]) return;
    bool on = state.wrelay_state[idx];
    if (!force && last_wswitch_state[idx] == (int)on) return;
    last_wswitch_state[idx] = (int)on;

    if (on) {
        lv_obj_set_style_bg_color(w_switches[idx], lv_color_hex(UI_COLOR_SUCCESS), 0);
        lv_obj_set_style_border_color(w_switches[idx], lv_color_hex(0x34d399), 0);
        if (lbl_wswitch_name[idx]) {
            ui_text_color_if_changed(lbl_wswitch_name[idx], lv_color_hex(0xffffff));
        }
    } else {
        lv_obj_set_style_bg_color(w_switches[idx], ui_theme_card(), 0);
        lv_obj_set_style_border_color(w_switches[idx], ui_theme_border(), 0);
        if (lbl_wswitch_name[idx]) {
            ui_text_color_if_changed(lbl_wswitch_name[idx], ui_theme_text());
        }
    }
}

static bool switches_animating = false;
static void switches_anim_cb(void *var, int32_t ratio);

static void relay_btn_event_cb(lv_event_t * e) {
    if (switches_animating) {
        lv_anim_del(NULL, switches_anim_cb);
        switches_animating = false;
    }
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    state.switch_state[idx] = !state.switch_state[idx];
    update_switch_visual(idx);
    http_publish_switch(idx, state.switch_state[idx]);
}

static void wrelay_btn_event_cb(lv_event_t * e) {
    if (switches_animating) {
        lv_anim_del(NULL, switches_anim_cb);
        switches_animating = false;
    }
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    state.wrelay_state[idx] = !state.wrelay_state[idx];
    update_wswitch_visual(idx);
    http_publish_wrelay(idx, state.wrelay_state[idx]);
}

// ==========================================
// Dimmer Animation (Tab-Load Smooth Fill)
// ==========================================
static bool dimmer_animating[8] = {false};

static void dimmer_anim_cb(void *var, int32_t val) {
    int idx = (int)(intptr_t)var;
    if (idx < 0 || idx >= 8 || !dimmers[idx]) return;
    lv_slider_set_value(dimmers[idx], val, LV_ANIM_OFF);
    if (lbl_dim_pct[idx]) {
        if (val > 0) {
            lv_label_set_text_fmt(lbl_dim_pct[idx], "%d%%", (int)val);
            lv_obj_set_style_text_color(lbl_dim_pct[idx], lv_color_hex(UI_COLOR_WARNING), 0);
        } else {
            lv_label_set_text(lbl_dim_pct[idx], "Aus");
            lv_obj_set_style_text_color(lbl_dim_pct[idx], ui_theme_muted(), 0);
        }
    }
}

static void dimmer_anim_ready_cb(lv_anim_t *a) {
    int idx = (int)(intptr_t)a->var;
    if (idx >= 0 && idx < 8) {
        dimmer_animating[idx] = false;
        update_dimmer_visuals(idx);
    }
}

void ui_trigger_dimmers_anim() {
    for (int i = 0; i < 8; i++) {
        if (!dimmers[i]) continue;
        lv_anim_del((void*)(intptr_t)i, dimmer_anim_cb);
        if (state.dimmer_val[i] > 0) {
            dimmer_animating[i] = true;
            lv_slider_set_value(dimmers[i], 0, LV_ANIM_OFF);
            lv_obj_set_style_bg_color(dimmers[i], lv_color_hex(UI_COLOR_WARNING), LV_PART_INDICATOR);
            if (lbl_dim_pct[i]) {
                lv_label_set_text(lbl_dim_pct[i], "0%");
                lv_obj_set_style_text_color(lbl_dim_pct[i], lv_color_hex(UI_COLOR_WARNING), 0);
            }
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, (void*)(intptr_t)i);
            lv_anim_set_exec_cb(&a, dimmer_anim_cb);
            lv_anim_set_time(&a, 600);
            lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
            lv_anim_set_values(&a, 0, state.dimmer_val[i]);
            lv_anim_set_ready_cb(&a, dimmer_anim_ready_cb);
            lv_anim_start(&a);
        } else {
            dimmer_animating[i] = false;
            lv_slider_set_value(dimmers[i], 0, LV_ANIM_OFF);
            update_dimmer_visuals(i);
        }
    }
}

// ==========================================
// Switches Animation (Tab-Load Color Fade)
// ==========================================
static void switches_anim_cb(void *var, int32_t ratio) {
    (void)var;
    uint8_t r = (uint8_t)(ratio > 255 ? 255 : (ratio < 0 ? 0 : ratio));
    lv_color_t bg_target = lv_color_hex(UI_COLOR_SUCCESS);
    lv_color_t border_target = lv_color_hex(0x34d399);
    lv_color_t text_target = lv_color_hex(0xffffff);
    lv_color_t bg_start = ui_theme_card();
    lv_color_t border_start = ui_theme_border();
    lv_color_t text_start = ui_theme_text();

    lv_color_t cur_bg = lv_color_mix(bg_target, bg_start, r);
    lv_color_t cur_border = lv_color_mix(border_target, border_start, r);
    lv_color_t cur_text = lv_color_mix(text_target, text_start, r);

    for (int i = 0; i < 8; i++) {
        if (state.switch_visible[i] && switches[i] && state.switch_state[i]) {
            lv_obj_set_style_bg_color(switches[i], cur_bg, 0);
            lv_obj_set_style_border_color(switches[i], cur_border, 0);
            if (lbl_switch_name[i]) {
                lv_obj_set_style_text_color(lbl_switch_name[i], cur_text, 0);
            }
        }
        if (state.show_wrelay && state.wrelay_visible[i] && w_switches[i] && state.wrelay_state[i]) {
            lv_obj_set_style_bg_color(w_switches[i], cur_bg, 0);
            lv_obj_set_style_border_color(w_switches[i], cur_border, 0);
            if (lbl_wswitch_name[i]) {
                lv_obj_set_style_text_color(lbl_wswitch_name[i], cur_text, 0);
            }
        }
    }
}

static void switches_anim_ready_cb(lv_anim_t *a) {
    (void)a;
    switches_animating = false;
    for (int i = 0; i < 8; i++) {
        if (state.switch_visible[i] && switches[i]) update_switch_visual(i, true);
        if (state.show_wrelay && state.wrelay_visible[i] && w_switches[i]) update_wswitch_visual(i, true);
    }
}

void ui_trigger_switches_anim() {
    lv_anim_del(NULL, switches_anim_cb);
    bool has_active = false;
    for (int i = 0; i < 8; i++) {
        if (state.switch_visible[i] && switches[i] && state.switch_state[i]) has_active = true;
        if (state.show_wrelay && state.wrelay_visible[i] && w_switches[i] && state.wrelay_state[i]) has_active = true;
    }
    if (!has_active) {
        switches_animating = false;
        return;
    }

    switches_animating = true;
    for (int i = 0; i < 8; i++) {
        if (state.switch_visible[i] && switches[i] && state.switch_state[i]) {
            lv_obj_set_style_bg_color(switches[i], ui_theme_card(), 0);
            lv_obj_set_style_border_color(switches[i], ui_theme_border(), 0);
            if (lbl_switch_name[i]) {
                lv_obj_set_style_text_color(lbl_switch_name[i], ui_theme_text(), 0);
            }
        }
        if (state.show_wrelay && state.wrelay_visible[i] && w_switches[i] && state.wrelay_state[i]) {
            lv_obj_set_style_bg_color(w_switches[i], ui_theme_card(), 0);
            lv_obj_set_style_border_color(w_switches[i], ui_theme_border(), 0);
            if (lbl_wswitch_name[i]) {
                lv_obj_set_style_text_color(lbl_wswitch_name[i], ui_theme_text(), 0);
            }
        }
    }

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, NULL);
    lv_anim_set_exec_cb(&a, switches_anim_cb);
    lv_anim_set_time(&a, 500);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_values(&a, 0, 255);
    lv_anim_set_ready_cb(&a, switches_anim_ready_cb);
    lv_anim_start(&a);
}

static void dimmer_slider_event_cb(lv_event_t * e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_t *slider = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING) {
        if (dimmer_animating[idx]) {
            lv_anim_del((void*)(intptr_t)idx, dimmer_anim_cb);
            dimmer_animating[idx] = false;
        }
        state.dimmer_hold_until[idx] = millis() + 3000;
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        if (dimmer_animating[idx]) {
            lv_anim_del((void*)(intptr_t)idx, dimmer_anim_cb);
            dimmer_animating[idx] = false;
        }
        state.dimmer_hold_until[idx] = millis() + 3000;
        state.dimmer_val[idx] = lv_slider_get_value(slider);
        update_dimmer_visuals(idx);
    } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        http_publish_dimmer(idx, state.dimmer_val[idx]);
        // Give VanPi time to apply the new value before polling overrides it again
        state.dimmer_hold_until[idx] = millis() + 2000;
    }
}

void ui_update_switches_tab() {
    for (int i = 0; i < 8; i++) {
        if (state.switch_visible[i] && switches[i]) {
            if (!switches_animating) update_switch_visual(i);
            if (lbl_switch_name[i]) {
                ui_label_set_text_if_changed(lbl_switch_name[i], state.switch_names[i].c_str());
            }
        }
        if (state.show_wrelay && state.wrelay_visible[i] && w_switches[i]) {
            if (!switches_animating) update_wswitch_visual(i);
            if (lbl_wswitch_name[i]) {
                char wlabel[64];
                snprintf(wlabel, sizeof(wlabel), LV_SYMBOL_WIFI " %s", state.wrelay_names[i].c_str());
                ui_label_set_text_if_changed(lbl_wswitch_name[i], wlabel);
            }
        }
    }
}

void ui_update_dimmers_tab() {
    for (int i = 0; i < 8; i++) {
        if (dimmers[i]) {
            // Never move a slider under the user's finger or during entry animation
            if (lv_obj_has_state(dimmers[i], LV_STATE_PRESSED)) continue;
            if (dimmer_animating[i]) continue;
            if (lv_slider_get_value(dimmers[i]) != state.dimmer_val[i]) {
                lv_slider_set_value(dimmers[i], state.dimmer_val[i], LV_ANIM_OFF);
                update_dimmer_visuals(i);
            }
            if (lbl_dim_name[i]) {
                ui_label_set_text_if_changed(lbl_dim_name[i], state.dimmer_names[i].c_str());
            }
        }
    }
}

void ui_build_switches(lv_obj_t *parent) {
    for (int i = 0; i < 8; i++) {
        last_switch_state[i] = -1;
        last_wswitch_state[i] = -1;
    }

    ui_setup_tab_page(parent);

    // Relays Container: 440px wide, centered flex row wrap
    lv_obj_t *cont_sw = lv_obj_create(parent);
    lv_obj_set_width(cont_sw, 440);
    lv_obj_set_height(cont_sw, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(cont_sw, 0, 0);
    lv_obj_set_style_border_width(cont_sw, 0, 0);
    lv_obj_set_flex_flow(cont_sw, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_all(cont_sw, 0, 0);
    lv_obj_set_style_pad_row(cont_sw, 12, 0);
    lv_obj_set_style_pad_column(cont_sw, 16, 0);
    lv_obj_set_style_flex_main_place(cont_sw, LV_FLEX_ALIGN_CENTER, 0);
    lv_obj_align(cont_sw, LV_ALIGN_TOP_MID, 0, 0);

    for (int i = 0; i < 8; i++) {
        switches[i] = NULL;
        lbl_switch_name[i] = NULL;
        if (!state.switch_visible[i]) continue;

        // 2 buttons per row: (440 - 16) / 2 = 212px wide, 94px high.
        // 3 rows = 3 * 94 + 2 * 12 = 306px <= 360px (no scrollbar for <= 6 buttons)
        switches[i] = lv_btn_create(cont_sw);
        lv_obj_set_size(switches[i], 212, 94);
        lv_obj_set_style_radius(switches[i], 14, 0);
        lv_obj_set_style_border_width(switches[i], 1, 0);
        lv_obj_set_style_shadow_width(switches[i], 0, 0);
        lv_obj_add_event_cb(switches[i], relay_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lbl_switch_name[i] = lv_label_create(switches[i]);
        lv_label_set_text(lbl_switch_name[i], state.switch_names[i].c_str());
        lv_label_set_long_mode(lbl_switch_name[i], LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl_switch_name[i], 190);
        lv_obj_set_style_text_font(lbl_switch_name[i], &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_align(lbl_switch_name[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(lbl_switch_name[i]);

        update_switch_visual(i, true);
    }

    // WiFi Relays
    for (int i = 0; i < 8; i++) {
        w_switches[i] = NULL;
        lbl_wswitch_name[i] = NULL;
        if (!state.show_wrelay || !state.wrelay_visible[i]) continue;

        w_switches[i] = lv_btn_create(cont_sw);
        lv_obj_set_size(w_switches[i], 212, 94);
        lv_obj_set_style_radius(w_switches[i], 14, 0);
        lv_obj_set_style_border_width(w_switches[i], 1, 0);
        lv_obj_set_style_shadow_width(w_switches[i], 0, 0);
        lv_obj_add_event_cb(w_switches[i], wrelay_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)i);

        lbl_wswitch_name[i] = lv_label_create(w_switches[i]);
        char wlabel[64];
        snprintf(wlabel, sizeof(wlabel), LV_SYMBOL_WIFI " %s", state.wrelay_names[i].c_str());
        lv_label_set_text(lbl_wswitch_name[i], wlabel);
        lv_label_set_long_mode(lbl_wswitch_name[i], LV_LABEL_LONG_WRAP);
        lv_obj_set_width(lbl_wswitch_name[i], 190);
        lv_obj_set_style_text_font(lbl_wswitch_name[i], &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_align(lbl_wswitch_name[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(lbl_wswitch_name[i]);

        update_wswitch_visual(i);
    }
}

void ui_build_dimmers(lv_obj_t *parent) {
    ui_setup_tab_page(parent);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(parent, 10, 0);

    for (int i = 0; i < 8; i++) {
        dimmers[i] = NULL;
        lbl_dim_name[i] = NULL;
        lbl_dim_pct[i] = NULL;
        if (!state.dimmer_visible[i]) continue;

        // Modern Home Assistant Style Card: Width 440px, Height 80px
        // 4 cards * 80px + 3 gaps * 10px = 350px <= 360px (Exactly 4 dimmers fit without scrollbar!)
        lv_obj_t *dim_card = ui_create_card(parent, 440, 80);
        lv_obj_set_style_pad_hor(dim_card, 16, 0);
        lv_obj_set_style_pad_ver(dim_card, 0, 0);
        
        // 1. Name label (Left side, vertically centered)
        lbl_dim_name[i] = lv_label_create(dim_card);
        lv_label_set_text(lbl_dim_name[i], state.dimmer_names[i].c_str());
        lv_obj_set_style_text_font(lbl_dim_name[i], &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(lbl_dim_name[i], ui_theme_text(), 0);
        lv_label_set_long_mode(lbl_dim_name[i], LV_LABEL_LONG_DOT);
        lv_obj_set_width(lbl_dim_name[i], 120);
        lv_obj_align(lbl_dim_name[i], LV_ALIGN_LEFT_MID, 0, 0);
        
        // 2. Modern Bar Slider (Middle, 32px thick, no knob/ball, fills like a level bar)
        dimmers[i] = lv_slider_create(dim_card);
        lv_obj_set_size(dimmers[i], 225, 32);
        lv_slider_set_range(dimmers[i], 0, 100);
        lv_slider_set_value(dimmers[i], state.dimmer_val[i], LV_ANIM_OFF);
        lv_obj_align(dimmers[i], LV_ALIGN_LEFT_MID, 130, 0);
        
        // Track styling (capsule pill shape)
        lv_obj_set_style_bg_color(dimmers[i], ui_theme_track(), LV_PART_MAIN);
        lv_obj_set_style_radius(dimmers[i], 16, LV_PART_MAIN);
        lv_obj_set_style_border_width(dimmers[i], 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(dimmers[i], ui_theme_border(), LV_PART_MAIN);
        lv_obj_set_style_clip_corner(dimmers[i], true, LV_PART_MAIN);
        
        // Indicator styling
        lv_obj_set_style_radius(dimmers[i], 16, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(dimmers[i], lv_color_hex(UI_COLOR_WARNING), LV_PART_INDICATOR);
        
        // Completely remove knob (no ball at the start or anywhere)
        lv_obj_remove_style(dimmers[i], NULL, (lv_style_selector_t)LV_PART_KNOB | (lv_style_selector_t)LV_STATE_ANY);
        lv_obj_set_style_bg_opa(dimmers[i], LV_OPA_TRANSP, LV_PART_KNOB);
        lv_obj_set_style_pad_all(dimmers[i], 0, LV_PART_KNOB);
        lv_obj_set_style_border_width(dimmers[i], 0, LV_PART_KNOB);
        lv_obj_set_style_shadow_width(dimmers[i], 0, LV_PART_KNOB);
        lv_obj_set_style_outline_width(dimmers[i], 0, LV_PART_KNOB);

        // Extended click area for easy fingertip touch interaction
        lv_obj_set_ext_click_area(dimmers[i], 6);

        lv_obj_add_event_cb(dimmers[i], dimmer_slider_event_cb, LV_EVENT_ALL, (void*)(intptr_t)i);

        // 3. Percentage / State label (Right side, vertically centered)
        lbl_dim_pct[i] = lv_label_create(dim_card);
        lv_label_set_text_fmt(lbl_dim_pct[i], "%d%%", state.dimmer_val[i]);
        lv_obj_set_style_text_font(lbl_dim_pct[i], &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_align(lbl_dim_pct[i], LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_width(lbl_dim_pct[i], 48);
        lv_obj_align(lbl_dim_pct[i], LV_ALIGN_RIGHT_MID, 0, 0);

        update_dimmer_visuals(i);
    }
}
