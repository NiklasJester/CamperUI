#include "ui_main.h"

namespace {

static lv_obj_t *nav_settings_screen = nullptr;
static lv_obj_t *list_cont = nullptr;

static uint8_t cur_order[TAB_COUNT];
static bool cur_enabled[TAB_COUNT];

static void render_list();

static void up_clicked(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx > 0 && idx < TAB_COUNT) {
        uint8_t tmp = cur_order[idx];
        cur_order[idx] = cur_order[idx - 1];
        cur_order[idx - 1] = tmp;
        render_list();
    }
}

static void down_clicked(lv_event_t *e) {
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx < TAB_COUNT - 1) {
        uint8_t tmp = cur_order[idx];
        cur_order[idx] = cur_order[idx + 1];
        cur_order[idx + 1] = tmp;
        render_list();
    }
}

static void switch_toggled(lv_event_t *e) {
    lv_obj_t *sw = lv_event_get_target(e);
    int tab_id = (int)(intptr_t)lv_event_get_user_data(e);
    if (tab_id >= 0 && tab_id < TAB_COUNT) {
        if (tab_id == TAB_SETTINGS) {
            lv_obj_add_state(sw, LV_STATE_CHECKED);
            cur_enabled[tab_id] = true;
        } else {
            cur_enabled[tab_id] = lv_obj_has_state(sw, LV_STATE_CHECKED);
        }
    }
}

static void render_list() {
    if (!list_cont) return;
    lv_obj_clean(list_cont);

    // Explanatory Hint at top of the list
    lv_obj_t *hint = lv_label_create(list_cont);
    lv_label_set_text(hint, "Reihenfolge mit " LV_SYMBOL_UP " / " LV_SYMBOL_DOWN " anpassen. Schalter aktiviert/deaktiviert Tabs.\nAusgeblendete Tabs sparen CPU & RAM.");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(hint, ui_theme_muted(), 0);
    lv_obj_set_width(hint, 440);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);

    for (int i = 0; i < TAB_COUNT; ++i) {
        uint8_t tab_id = cur_order[i];

        lv_obj_t *card = ui_create_card(list_cont, 440, 54);
        lv_obj_set_style_pad_all(card, 6, 0);

        // 1. Move UP Button
        lv_obj_t *btn_up = lv_btn_create(card);
        lv_obj_set_size(btn_up, 38, 38);
        lv_obj_align(btn_up, LV_ALIGN_LEFT_MID, 4, 0);
        lv_obj_set_style_radius(btn_up, 8, 0);
        lv_obj_set_style_bg_color(btn_up, ui_theme_track(), 0);
        lv_obj_set_style_border_color(btn_up, ui_theme_border(), 0);
        lv_obj_set_style_border_width(btn_up, 1, 0);
        if (i == 0) {
            lv_obj_add_state(btn_up, LV_STATE_DISABLED);
            lv_obj_set_style_bg_opa(btn_up, LV_OPA_30, LV_STATE_DISABLED);
        } else {
            lv_obj_add_event_cb(btn_up, up_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        }
        lv_obj_t *lbl_up = lv_label_create(btn_up);
        lv_label_set_text(lbl_up, LV_SYMBOL_UP);
        lv_obj_set_style_text_color(lbl_up, ui_theme_text(), 0);
        lv_obj_center(lbl_up);

        // 2. Move DOWN Button
        lv_obj_t *btn_down = lv_btn_create(card);
        lv_obj_set_size(btn_down, 38, 38);
        lv_obj_align(btn_down, LV_ALIGN_LEFT_MID, 48, 0);
        lv_obj_set_style_radius(btn_down, 8, 0);
        lv_obj_set_style_bg_color(btn_down, ui_theme_track(), 0);
        lv_obj_set_style_border_color(btn_down, ui_theme_border(), 0);
        lv_obj_set_style_border_width(btn_down, 1, 0);
        if (i == TAB_COUNT - 1) {
            lv_obj_add_state(btn_down, LV_STATE_DISABLED);
            lv_obj_set_style_bg_opa(btn_down, LV_OPA_30, LV_STATE_DISABLED);
        } else {
            lv_obj_add_event_cb(btn_down, down_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        }
        lv_obj_t *lbl_down = lv_label_create(btn_down);
        lv_label_set_text(lbl_down, LV_SYMBOL_DOWN);
        lv_obj_set_style_text_color(lbl_down, ui_theme_text(), 0);
        lv_obj_center(lbl_down);

        // 3. Tab Icon
        lv_obj_t *lbl_icon = lv_label_create(card);
        lv_label_set_text(lbl_icon, TAB_METAS[tab_id].icon);
        lv_obj_set_style_text_font(lbl_icon, &ui_font_mdi_32, 0);
        lv_obj_set_style_text_color(lbl_icon, cur_enabled[tab_id] ? lv_color_hex(UI_COLOR_PRIMARY) : ui_theme_muted(), 0);
        lv_obj_align(lbl_icon, LV_ALIGN_LEFT_MID, 96, 0);

        // 4. Tab Name
        lv_obj_t *lbl_name = lv_label_create(card);
        char buf[64];
        snprintf(buf, sizeof(buf), "%d. %s", i + 1, TAB_METAS[tab_id].name);
        lv_label_set_text(lbl_name, buf);
        lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(lbl_name, cur_enabled[tab_id] ? ui_theme_text() : ui_theme_muted(), 0);
        lv_obj_align(lbl_name, LV_ALIGN_LEFT_MID, 142, 0);

        // 5. Visibility Switch
        lv_obj_t *sw = lv_switch_create(card);
        lv_obj_align(sw, LV_ALIGN_RIGHT_MID, -8, 0);
        if (cur_enabled[tab_id]) {
            lv_obj_add_state(sw, LV_STATE_CHECKED);
        }
        if (tab_id == TAB_SETTINGS) {
            // Keep Settings always enabled so user can never lock themselves out
            lv_obj_add_state(sw, LV_STATE_DISABLED);
        } else {
            lv_obj_add_event_cb(sw, switch_toggled, LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)tab_id);
        }
    }
}

static void save_and_restart_clicked(lv_event_t *) {
    // Commit new order and visibility to system state
    for (int i = 0; i < TAB_COUNT; ++i) {
        state.tab_order[i] = cur_order[i];
        state.tab_enabled[i] = cur_enabled[i];
    }
    // Safety check: Settings must remain enabled
    state.tab_enabled[TAB_SETTINGS] = true;

    // Save permanently to NVS
    state_save();

    // Show reboot message
    lv_obj_t *mbox = lv_msgbox_create(NULL, "Gespeichert", "Menue-Konfiguration gespeichert!\nCamperUI startet neu...", NULL, false);
    lv_obj_center(mbox);
    lv_timer_handler();

    delay(600);
    ESP.restart();
}

} // namespace

void ui_open_nav_settings() {
    // Copy current state to working buffers
    memcpy(cur_order, state.tab_order, sizeof(cur_order));
    memcpy(cur_enabled, state.tab_enabled, sizeof(cur_enabled));
    cur_enabled[TAB_SETTINGS] = true;

    if (!nav_settings_screen) {
        nav_settings_screen = lv_obj_create(nullptr);
        lv_obj_clear_flag(nav_settings_screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(nav_settings_screen, ui_theme_bg(), 0);

        // 1. Top Header Bar
        lv_obj_t *header = lv_obj_create(nav_settings_screen);
        lv_obj_set_pos(header, 0, 0);
        lv_obj_set_size(header, 480, 50);
        lv_obj_set_style_bg_color(header, ui_theme_card(), 0);
        lv_obj_set_style_border_color(header, ui_theme_border(), 0);
        lv_obj_set_style_border_width(header, 1, 0);
        lv_obj_set_style_radius(header, 0, 0);
        lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

        // Back Button
        lv_obj_t *back = lv_btn_create(header);
        lv_obj_set_size(back, 96, 36);
        lv_obj_align(back, LV_ALIGN_LEFT_MID, 8, 0);
        lv_obj_set_style_radius(back, 8, 0);
        lv_obj_set_style_bg_color(back, ui_theme_track(), 0);
        lv_obj_set_style_border_color(back, ui_theme_border(), 0);
        lv_obj_set_style_border_width(back, 1, 0);
        lv_obj_add_event_cb(back, [](lv_event_t *) { lv_scr_load(scr_main); }, LV_EVENT_CLICKED, nullptr);

        lv_obj_t *back_lbl = lv_label_create(back);
        lv_label_set_text(back_lbl, LV_SYMBOL_LEFT " Zurueck");
        lv_obj_set_style_text_color(back_lbl, ui_theme_text(), 0);
        lv_obj_center(back_lbl);

        // Title
        lv_obj_t *title = lv_label_create(header);
        lv_label_set_text(title, "Menueleiste anpassen");
        lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(title, ui_theme_text(), 0);
        lv_obj_align(title, LV_ALIGN_CENTER, 20, 0);

        // 2. Middle Scrollable List Container
        list_cont = lv_obj_create(nav_settings_screen);
        lv_obj_set_pos(list_cont, 0, 52);
        lv_obj_set_size(list_cont, 480, 360);
        lv_obj_set_style_bg_opa(list_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(list_cont, 0, 0);
        lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(list_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_hor(list_cont, 16, 0);
        lv_obj_set_style_pad_ver(list_cont, 8, 0);
        lv_obj_set_style_pad_row(list_cont, 8, 0);
        lv_obj_set_scroll_dir(list_cont, LV_DIR_VER);
        lv_obj_set_scrollbar_mode(list_cont, LV_SCROLLBAR_MODE_AUTO);

        // 3. Bottom Bar with Save & Restart Button
        lv_obj_t *footer = lv_obj_create(nav_settings_screen);
        lv_obj_set_pos(footer, 0, 416);
        lv_obj_set_size(footer, 480, 64);
        lv_obj_set_style_bg_color(footer, ui_theme_card(), 0);
        lv_obj_set_style_border_color(footer, ui_theme_border(), 0);
        lv_obj_set_style_border_width(footer, 1, 0);
        lv_obj_set_style_radius(footer, 0, 0);
        lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *btn_save = lv_btn_create(footer);
        lv_obj_set_size(btn_save, 420, 46);
        lv_obj_center(btn_save);
        lv_obj_set_style_radius(btn_save, 12, 0);
        lv_obj_set_style_bg_color(btn_save, lv_color_hex(UI_COLOR_SUCCESS), 0);
        lv_obj_set_style_shadow_width(btn_save, 10, 0);
        lv_obj_set_style_shadow_color(btn_save, lv_color_hex(UI_COLOR_SUCCESS), 0);
        lv_obj_set_style_shadow_opa(btn_save, LV_OPA_30, 0);
        lv_obj_add_event_cb(btn_save, save_and_restart_clicked, LV_EVENT_CLICKED, nullptr);

        lv_obj_t *lbl_save = lv_label_create(btn_save);
        lv_label_set_text(lbl_save, LV_SYMBOL_SAVE "  Speichern & Neustarten");
        lv_obj_set_style_text_font(lbl_save, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(lbl_save, lv_color_hex(0xffffff), 0);
        lv_obj_center(lbl_save);
    }

    render_list();
    lv_scr_load(nav_settings_screen);
}

bool ui_nav_settings_is_active() {
    return nav_settings_screen && lv_scr_act() == nav_settings_screen;
}

void ui_nav_settings_destroy() {
    if (nav_settings_screen) {
        lv_obj_del(nav_settings_screen);
        nav_settings_screen = nullptr;
        list_cont = nullptr;
    }
}

