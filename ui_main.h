#ifndef UI_MAIN_H
#define UI_MAIN_H

#include <lvgl.h>
#include "system_state.h"
#include "ui_mdi_icons.h"
#include "ui_fonts.h"
#include <string.h>
#define CAMPERUI_VERSION "V1.0.3"

// ==========================================
// Modern Automotive / Camper Color Palette
// ==========================================
#define UI_COLOR_BG_DARK       0x14171d
#define UI_COLOR_BG_LIGHT      0xf1f5f9
#define UI_COLOR_CARD_DARK     0x1e232b
#define UI_COLOR_CARD_LIGHT    0xffffff
#define UI_COLOR_BORDER_DARK   0x2e3545
#define UI_COLOR_BORDER_LIGHT  0xe2e8f0
#define UI_COLOR_TEXT_DARK     0xf8fafc
#define UI_COLOR_TEXT_LIGHT    0x0f172a
#define UI_COLOR_MUTED_DARK    0x94a3b8
#define UI_COLOR_MUTED_LIGHT   0x64748b

#define UI_COLOR_PRIMARY       0x0ea5e9 // Sky Blue
#define UI_COLOR_SUCCESS       0x10b981 // Emerald Green
#define UI_COLOR_WARNING       0xf59e0b // Warm Amber
#define UI_COLOR_DANGER        0xef4444 // Coral Red
#define UI_COLOR_TRACK_DARK    0x12151c // Recessed slider / tank track dark
#define UI_COLOR_TRACK_LIGHT   0xdbe2ec // Recessed slider / tank track light

// Inline helpers for theme-aware colors
static inline lv_color_t ui_theme_bg() { return state.dark_mode ? lv_color_hex(UI_COLOR_BG_DARK) : lv_color_hex(UI_COLOR_BG_LIGHT); }
static inline lv_color_t ui_theme_card() { return state.dark_mode ? lv_color_hex(UI_COLOR_CARD_DARK) : lv_color_hex(UI_COLOR_CARD_LIGHT); }
static inline lv_color_t ui_theme_border() { return state.dark_mode ? lv_color_hex(UI_COLOR_BORDER_DARK) : lv_color_hex(UI_COLOR_BORDER_LIGHT); }
static inline lv_color_t ui_theme_text() { return state.dark_mode ? lv_color_hex(UI_COLOR_TEXT_DARK) : lv_color_hex(UI_COLOR_TEXT_LIGHT); }
static inline lv_color_t ui_theme_muted() { return state.dark_mode ? lv_color_hex(UI_COLOR_MUTED_DARK) : lv_color_hex(UI_COLOR_MUTED_LIGHT); }
static inline lv_color_t ui_theme_track() { return state.dark_mode ? lv_color_hex(UI_COLOR_TRACK_DARK) : lv_color_hex(UI_COLOR_TRACK_LIGHT); }

// Avoid reallocating text and invalidating a label when its display is unchanged.
static inline void ui_label_set_text_if_changed(lv_obj_t *lbl, const char *text) {
    if (lbl && strcmp(lv_label_get_text(lbl), text) != 0) lv_label_set_text(lbl, text);
}

static inline void ui_text_color_if_changed(lv_obj_t *obj, lv_color_t color,
                                            lv_style_selector_t selector = 0) {
    if (obj && lv_obj_get_style_text_color(obj, selector & LV_PART_ANY).full != color.full)
        lv_obj_set_style_text_color(obj, color, selector);
}

// Robust float-to-label formatter using C runtime snprintf
static inline void ui_label_set_float(lv_obj_t *lbl, const char *fmt, float val) {
    if (!lbl) return;
    char buf[64];
    snprintf(buf, sizeof(buf), fmt, val);
    ui_label_set_text_if_changed(lbl, buf);
}

// Global UI Initialization
void ui_init();
void ui_debug_navigation(const char *stage, bool force = false);

// Tab Updates
void ui_update_data();
void ui_update_home();
void ui_home_layout_changed();
void ui_build_home_settings(lv_obj_t *parent);
void ui_home_settings_refresh();
void ui_open_home_settings();
bool ui_home_settings_is_active();
void ui_home_settings_destroy();
void ui_sync_demo_controls();
void ui_update_visible_page();
void ui_update_power_tab();
void ui_update_water_tab();
void ui_update_climate_tab();
void ui_update_switches_tab();
void ui_update_dimmers_tab();
void ui_update_level_tab();
void ui_update_settings_tab();

// Anim Triggers
void ui_trigger_power_anim();
void ui_trigger_water_anim();
void ui_trigger_dimmers_anim();
void ui_trigger_switches_anim();
void ui_trigger_level_anim();

// Sub-Tab Builders
void ui_build_power(lv_obj_t *parent);
void ui_build_water(lv_obj_t *parent);
void ui_build_climate(lv_obj_t *parent);
void ui_build_switches(lv_obj_t *parent);
void ui_build_dimmers(lv_obj_t *parent);
void ui_build_level(lv_obj_t *parent);
void ui_build_settings(lv_obj_t *parent);

void ui_build_maxxfan(lv_obj_t *parent);
bool ui_maxxfan_running();

// UI Helper Functions
lv_obj_t* ui_create_card(lv_obj_t *parent, int w, int h);
void ui_apply_theme();
void ui_setup_tab_page(lv_obj_t *page);

struct TabMeta {
    uint8_t id;
    const char *name;
    const char *icon;
};

extern const TabMeta TAB_METAS[TAB_COUNT];

void ui_open_nav_settings();
void ui_build_nav_settings(lv_obj_t *parent);
bool ui_nav_settings_is_active();
void ui_nav_settings_destroy();

extern lv_obj_t *lbl_debug_info;
extern lv_obj_t *scr_main;
extern lv_obj_t *scr_settings;
void ui_settings_screen_init();

#endif // UI_MAIN_H
