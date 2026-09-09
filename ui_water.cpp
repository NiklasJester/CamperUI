#include "ui_main.h"
#include "http_handler.h"

static lv_obj_t *bar_tanks[4];
static lv_obj_t *lbl_tanks_pct[4];
static lv_obj_t *lbl_tanks_liters[4];
static int last_tank_level[4] = {-1, -1, -1, -1};

static lv_obj_t *btn_pump;
static lv_obj_t *lbl_pump;
static lv_obj_t *btn_drain;
static lv_obj_t *lbl_drain;

static void update_water_button_visuals() {
    // Pump Button
    if (btn_pump && state.pump_relay >= 0 && state.pump_relay < 8) {
        bool on = state.switch_state[state.pump_relay];
        if (on) {
            lv_obj_set_style_bg_color(btn_pump, lv_color_hex(UI_COLOR_SUCCESS), 0);
            lv_obj_set_style_border_color(btn_pump, lv_color_hex(0x34d399), 0);
            lv_obj_set_style_shadow_color(btn_pump, lv_color_hex(UI_COLOR_SUCCESS), 0);
            lv_obj_set_style_shadow_opa(btn_pump, LV_OPA_30, 0);
            lv_obj_set_style_text_color(lbl_pump, lv_color_hex(0xffffff), 0);
        } else {
            lv_obj_set_style_bg_color(btn_pump, ui_theme_card(), 0);
            lv_obj_set_style_border_color(btn_pump, ui_theme_border(), 0);
            lv_obj_set_style_shadow_color(btn_pump, lv_color_hex(0x000000), 0);
            lv_obj_set_style_shadow_opa(btn_pump, state.dark_mode ? LV_OPA_30 : LV_OPA_10, 0);
            lv_obj_set_style_text_color(lbl_pump, ui_theme_text(), 0);
        }
    }

    // Drain Button
    if (btn_drain && state.drain_relay >= 0 && state.drain_relay < 8) {
        bool on = state.switch_state[state.drain_relay];
        if (on) {
            lv_obj_set_style_bg_color(btn_drain, lv_color_hex(UI_COLOR_WARNING), 0);
            lv_obj_set_style_border_color(btn_drain, lv_color_hex(0xfbbf24), 0);
            lv_obj_set_style_shadow_color(btn_drain, lv_color_hex(UI_COLOR_WARNING), 0);
            lv_obj_set_style_shadow_opa(btn_drain, LV_OPA_30, 0);
            lv_obj_set_style_text_color(lbl_drain, lv_color_hex(0xffffff), 0);
        } else {
            lv_obj_set_style_bg_color(btn_drain, ui_theme_card(), 0);
            lv_obj_set_style_border_color(btn_drain, ui_theme_border(), 0);
            lv_obj_set_style_shadow_color(btn_drain, lv_color_hex(0x000000), 0);
            lv_obj_set_style_shadow_opa(btn_drain, state.dark_mode ? LV_OPA_30 : LV_OPA_10, 0);
            lv_obj_set_style_text_color(lbl_drain, ui_theme_text(), 0);
        }
    }
}

void ui_update_water_tab() {
    for (int i = 0; i < 4; i++) {
        if (!state.tank_enabled[i]) continue;
        if (!bar_tanks[i]) continue;
        
        if (state.tank_level[i] != last_tank_level[i]) {
            lv_bar_set_value(bar_tanks[i], state.tank_level[i], LV_ANIM_ON);
            lv_label_set_text_fmt(lbl_tanks_pct[i], "%d%%", state.tank_level[i]);
            
            int max_l = state.tank_max[i] > 0 ? state.tank_max[i] : 100;
            int liters = (state.tank_level[i] * max_l) / 100;
            lv_label_set_text_fmt(lbl_tanks_liters[i], "%d / %d L", liters, max_l);
            
            last_tank_level[i] = state.tank_level[i];
        }
    }
    
    update_water_button_visuals();
}


void ui_trigger_water_anim() {
    for (int i = 0; i < 4; i++) {
        if (bar_tanks[i]) {
            lv_bar_set_value(bar_tanks[i], 0, LV_ANIM_OFF);
            last_tank_level[i] = -1; // Force re-animation on next update
        }
    }
}

void ui_build_water(lv_obj_t *parent) {
    ui_setup_tab_page(parent);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // ==========================================
    // 1. Tanks Card (Top: Height 284px)
    // ==========================================
    lv_obj_t *card_tanks = ui_create_card(parent, 440, 284);
    lv_obj_align(card_tanks, LV_ALIGN_TOP_MID, 0, 0);
    
    lv_obj_t *lbl_title = lv_label_create(card_tanks);
    lv_label_set_text_fmt(lbl_title, "%s Wassertanks", LV_SYMBOL_TINT);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_title, ui_theme_muted(), 0);
    lv_obj_align(lbl_title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_color_t tank_colors[] = {
        lv_color_hex(UI_COLOR_PRIMARY), // Frischwasser: Sky Cyan
        lv_color_hex(0x64748b),         // Grauwasser: Slate Gray
        lv_color_hex(0x718093),         // Grauwasser 2
        lv_color_hex(0x334155)          // Schwarzwasser: Deep Slate
    };

    int num_active = 0;
    for (int i = 0; i < 4; i++) if (state.tank_enabled[i]) num_active++;

    if (num_active == 0) {
        lv_obj_t *l = lv_label_create(card_tanks);
        lv_label_set_text(l, "Keine Tanks aktiviert\n(In Einstellungen anpassen)");
        lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(l, ui_theme_muted(), 0);
        lv_obj_center(l);
    } else {
        // Inner card width is 440 - 24 = 416px
        int col_width = 416 / num_active;
        int bar_w = (num_active <= 2) ? 80 : 58;
        int current_idx = 0;

        for (int i = 0; i < 4; i++) {
            if (!state.tank_enabled[i]) continue;
            last_tank_level[i] = -1;

            int col_x = current_idx * col_width;
            int center_x = col_x + (col_width / 2);
            current_idx++;

            // Tank Name (Centered exactly over column and bar)
            lv_obj_t *lbl_name = lv_label_create(card_tanks);
            lv_label_set_text(lbl_name, state.tank_names[i].c_str());
            lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_16, 0);
            lv_obj_set_style_text_color(lbl_name, ui_theme_text(), 0);
            lv_obj_set_width(lbl_name, col_width);
            lv_obj_set_style_text_align(lbl_name, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_align(lbl_name, LV_ALIGN_TOP_LEFT, col_x, 24);

            // Vertical Tank Bar (Taller: 160px height)
            bar_tanks[i] = lv_bar_create(card_tanks);
            lv_obj_set_size(bar_tanks[i], bar_w, 160);
            lv_obj_align(bar_tanks[i], LV_ALIGN_TOP_LEFT, center_x - (bar_w / 2), 48);
            lv_bar_set_range(bar_tanks[i], 0, 100);
            lv_bar_set_value(bar_tanks[i], 0, LV_ANIM_OFF);
            
            // Track styling: Recessed darker background with border for clear visibility when empty
            lv_obj_set_style_radius(bar_tanks[i], 12, LV_PART_MAIN);
            lv_obj_set_style_radius(bar_tanks[i], 12, LV_PART_INDICATOR);
            lv_obj_set_style_bg_color(bar_tanks[i], ui_theme_track(), LV_PART_MAIN);
            lv_obj_set_style_border_width(bar_tanks[i], 1, LV_PART_MAIN);
            lv_obj_set_style_border_color(bar_tanks[i], ui_theme_border(), LV_PART_MAIN);

            lv_obj_set_style_bg_color(bar_tanks[i], tank_colors[i], LV_PART_INDICATOR);
            lv_obj_set_style_bg_grad_color(bar_tanks[i], lv_color_lighten(tank_colors[i], 50), LV_PART_INDICATOR);
            lv_obj_set_style_bg_grad_dir(bar_tanks[i], LV_GRAD_DIR_VER, LV_PART_INDICATOR);
            lv_obj_set_style_anim_time(bar_tanks[i], 1200, LV_PART_MAIN);

            // Percentage Label (Centered exactly over bar)
            lbl_tanks_pct[i] = lv_label_create(card_tanks);
            lv_obj_set_style_text_font(lbl_tanks_pct[i], &lv_font_montserrat_18, 0);
            lv_obj_set_style_text_color(lbl_tanks_pct[i], ui_theme_text(), 0);
            lv_obj_set_width(lbl_tanks_pct[i], col_width);
            lv_obj_set_style_text_align(lbl_tanks_pct[i], LV_TEXT_ALIGN_CENTER, 0);
            lv_label_set_text(lbl_tanks_pct[i], "0%");
            lv_obj_align(lbl_tanks_pct[i], LV_ALIGN_TOP_LEFT, col_x, 214);

            // Liters Label (Centered exactly over bar)
            lbl_tanks_liters[i] = lv_label_create(card_tanks);
            lv_obj_set_style_text_font(lbl_tanks_liters[i], &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(lbl_tanks_liters[i], ui_theme_muted(), 0);
            lv_obj_set_width(lbl_tanks_liters[i], col_width);
            lv_obj_set_style_text_align(lbl_tanks_liters[i], LV_TEXT_ALIGN_CENTER, 0);
            lv_label_set_text(lbl_tanks_liters[i], "0 L");
            lv_obj_align(lbl_tanks_liters[i], LV_ALIGN_TOP_LEFT, col_x, 240);
        }
    }

    // ==========================================
    // 2. Action Buttons (Bottom: 64px compact height, 12px spacing)
    // ==========================================
    // Pump Button (Left)
    btn_pump = lv_btn_create(parent);
    lv_obj_set_size(btn_pump, 214, 64);
    lv_obj_align(btn_pump, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_radius(btn_pump, 14, 0);
    lv_obj_set_style_border_width(btn_pump, 1, 0);
    lv_obj_set_style_shadow_width(btn_pump, 8, 0);
    lv_obj_set_style_shadow_ofs_y(btn_pump, 3, 0);
    
    lbl_pump = lv_label_create(btn_pump);
    lv_label_set_text_fmt(lbl_pump, "%s  Wasserpumpe", LV_SYMBOL_TINT);
    lv_obj_set_style_text_font(lbl_pump, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(lbl_pump, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl_pump);

    lv_obj_add_event_cb(btn_pump, [](lv_event_t * e) {
        if (state.pump_relay >= 0 && state.pump_relay < 8) {
            state.switch_state[state.pump_relay] = !state.switch_state[state.pump_relay];
            http_publish_switch(state.pump_relay, state.switch_state[state.pump_relay]);
            state_save();
            update_water_button_visuals();
        }
    }, LV_EVENT_CLICKED, NULL);

    // Drain Button (Right)
    btn_drain = lv_btn_create(parent);
    lv_obj_set_size(btn_drain, 214, 64);
    lv_obj_align(btn_drain, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_radius(btn_drain, 14, 0);
    lv_obj_set_style_border_width(btn_drain, 1, 0);
    lv_obj_set_style_shadow_width(btn_drain, 8, 0);
    lv_obj_set_style_shadow_ofs_y(btn_drain, 3, 0);
    
    lbl_drain = lv_label_create(btn_drain);
    lv_label_set_text_fmt(lbl_drain, "%s  Abwasser", LV_SYMBOL_DOWN);
    lv_obj_set_style_text_font(lbl_drain, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(lbl_drain, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lbl_drain);

    lv_obj_add_event_cb(btn_drain, [](lv_event_t * e) {
        if (state.drain_relay >= 0 && state.drain_relay < 8) {
            state.switch_state[state.drain_relay] = !state.drain_relay ? false : !state.switch_state[state.drain_relay];
            http_publish_switch(state.drain_relay, state.switch_state[state.drain_relay]);
            state_save();
            update_water_button_visuals();
        }
    }, LV_EVENT_CLICKED, NULL);

    update_water_button_visuals();
}
