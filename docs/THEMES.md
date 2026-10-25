# 🎨 CamperUI Theme Engine – Entwickler- & Styling-Guide

Dieser Leitfaden richtet sich an Entwickler, die neue Tabs, Karten (Cards), Buttons oder Controls zu CamperUI hinzufügen oder bestehende UI-Elemente anpassen möchten. Er beschreibt die Architektur der Theme-Engine, alle verfügbaren Farb- und Formvariablen sowie Best Practices für performantes, konsistentes Rendering.

---

## 1. Architektur der Theme-Engine

CamperUI verfügt über eine dynamische, zur Laufzeit anpassbare Theme-Engine:
- **Speicherung:** Gespeichert als `ThemeConfig` (Version 3) im Non-Volatile Storage (NVS) unter dem Schlüssel `theme_cfg`.
- **Echtzeit-Aktualisierung:** Änderungen im Web-Interface (`/settings` -> *Theme & Design*) oder über die REST-API (`POST /api/theme`) werden **ohne Neustart** in unter 100 ms live auf dem Display gerendert (`rebuild_ui()` via `lv_async_call`).
- **Kompatibilität:** Alle Farben werden als `uint32_t` (RGB hex, z. B. `0x0EA5E9`) im NVS gehalten und für LVGL transparent als `lv_color_t` bereitgestellt.

---

## 2. Theme-Funktionen & Variablen (Referenz)

Verwende in UI-Dateien (`ui_*.cpp`) **niemals hartcodierte Farben** (wie `lv_color_hex(0x0ea5e9)` oder `lv_palette_main(...)`), sondern stets die globalen Theme-Getter aus [ui_main.h](file:///c:/Users/njest/Documents/projects/CamperUI/ui_main.h):

### Oberflächen & Hintergründe
| Funktion | Standard (Camper Slate) | Verwendung |
| :--- | :--- | :--- |
| `ui_theme_bg()` | `#101318` | Gesamthintergrund der Anwendung (`scr_main`, Settings-Hintergrund). |
| `ui_theme_card()` | `#181C24` | Hintergrund für Kacheln (`card`), Inaktive Buttons, Statusleiste. |
| `ui_theme_track()` | `#12151C` | Hintergrund für Slider-Schienen, inaktive Balken und Segment-Tracks. |

### Rahmen & Geometrie (Rundungen)
| Funktion | Typ / Werte | Verwendung |
| :--- | :--- | :--- |
| `ui_theme_border()` | `lv_color_t` (`#272D3B`) | Rahmenfarbe für Kacheln, Buttons, Trennlinien und Dialoge. |
| `ui_border_width()` | `uint8_t` (0 .. 3 px) | Rahmenstärke für Cards und Buttons (`lv_obj_set_style_border_width`). |
| `ui_radius_card()` | `uint8_t` (0 .. 30 px) | Eckenrundung für Kacheln/Cards (`lv_obj_set_style_radius`). |
| `ui_radius_button()` | `uint8_t` (0 .. 20 px) | Eckenrundung für Tasten, Buttons und Slider (`lv_obj_set_style_radius`). |

### Komponenten- & Akzentfarben
| Funktion | Standard (Camper Slate) | Verwendung |
| :--- | :--- | :--- |
| `ui_theme_primary()` | `#0EA5E9` (Blau) | Primäre UI-Aktionen (z. B. Nullpunkt Tara, WLAN Scan, ausgewählte Modi/Tabs). |
| `ui_theme_dimmer()` | `#F59E0B` (Bernstein) | Beleuchtung, Licht-Schieberegler (Dimmer) und aktive Licht-Zustände. |
| `ui_theme_relay()` | `#10B981` (Grün) | Schalter & Relais im Zustand **EIN**, Wasserpumpe aktiv, CI-Bus Power EIN. |
| `ui_theme_tank()` | `#06B6D4` (Cyan) | Füllstandsanzeigen für Frischwasser und sonstige Tanks. |

### Typografie & Schriftfarben
| Funktion | Standard | Verwendung |
| :--- | :--- | :--- |
| `ui_theme_text()` | `#F1F5F9` | Hauptüberschriften, Messwerte, primäre Beschriftungen. |
| `ui_theme_muted()` | `#94A3B8` | Sekundäre Hinweise, Einheiten, inaktive Symbole, Zeitstempel. |
| `ui_theme_btn_text()` | `#F1F5F9` | Textfarbe auf **inaktiven** oder allgemeinen GUI-Buttons. |
| `ui_theme_btn_active_text()` | `#FFFFFF` | Textfarbe auf **aktiven / farbigen** Buttons (z. B. Relais EIN, Stufe aktiv). |

---

## 3. Best Practices für neue Tabs & Widgets

### A. Kacheln (Cards) erstellen
Nutze immer `ui_create_card()`. Die Funktion wendet automatisch Hintergrund, Rahmen und Radius des aktuellen Themes an:

```cpp
// Erstellt eine standardkonforme Kachel:
lv_obj_t *card = ui_create_card(parent, 440, 160);
lv_obj_set_pos(card, 0, 0);
lv_obj_set_style_pad_all(card, 12, 0);
```

### B. Buttons & Schalter gestalten
Ein Button hat in der Regel zwei Zustände (Inaktiv / Aktiv):

```cpp
// 1. Erstellung im Builder (z. B. ui_build_my_tab):
lv_obj_t *btn = lv_btn_create(card);
lv_obj_set_size(btn, 120, 50);
lv_obj_set_style_radius(btn, ui_radius_button(), 0);
lv_obj_set_style_border_width(btn, ui_border_width(), 0);
lv_obj_set_style_shadow_width(btn, 0, 0);

lv_obj_t *lbl = lv_label_create(btn);
lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
lv_obj_center(lbl);

// 2. Dynamische Aktualisierung (z. B. in ui_update_my_tab):
if (is_active) {
    ui_bg_color_if_changed(btn, ui_theme_relay());
    ui_border_color_if_changed(btn, ui_theme_relay());
    ui_text_color_if_changed(lbl, ui_theme_btn_active_text());
} else {
    ui_bg_color_if_changed(btn, ui_theme_card());
    ui_border_color_if_changed(btn, ui_theme_border());
    ui_text_color_if_changed(lbl, ui_theme_btn_text());
}
```

### C. Slider & Fortschrittsbalken
```cpp
lv_obj_t *slider = lv_slider_create(card);
lv_obj_set_style_radius(slider, ui_radius_button(), 0);

// Schiene (Track):
lv_obj_set_style_bg_color(slider, ui_theme_track(), LV_PART_MAIN);
lv_obj_set_style_border_color(slider, ui_theme_border(), LV_PART_MAIN);
lv_obj_set_style_border_width(slider, ui_border_width(), LV_PART_MAIN);

// Aktiver Balken (Indicator):
// Für Dimmer: ui_theme_dimmer()
// Für Tanks:   ui_theme_tank()
// Für Menü:    ui_theme_primary()
lv_obj_set_style_bg_color(slider, ui_theme_dimmer(), LV_PART_INDICATOR);

// Knopf (Knob):
lv_obj_set_style_bg_color(slider, ui_theme_text(), LV_PART_KNOB);
```

### D. Vermeidung von Redraw-Lags (`*_if_changed`)
Das ESP32-S3 RGB-Display rendert mit 50–60 FPS. Um unnötige Zeichenoperationen im 1-Sekunden-Aktualisierungszyklus zu vermeiden, müssen Änderungen immer über die bedingten Helferfunktionen ausgeführt werden:
- `ui_label_set_text_if_changed(label, text)`
- `ui_bg_color_if_changed(obj, color)`
- `ui_border_color_if_changed(obj, color)`
- `ui_text_color_if_changed(obj, color)`

---

## 4. Lifecycle bei Theme-Wechsel (Live-Rebuild)

Wenn der Benutzer ein neues Theme speichert, ruft die Theme-Engine [`rebuild_ui()`](file:///c:/Users/njest/Documents/projects/CamperUI/ui_main.cpp#L288) auf. Damit ein neuer Tab sauber neu initialisiert wird, muss er zwei Funktionen bereitstellen:

1. **`ui_<name>_reset()`:**
   Setzt alle statischen Widget-Zeiger (`lv_obj_t*`) auf `nullptr`.
2. **`ui_build_<name>(lv_obj_t *parent)`:**
   Baut die Widgets unter dem übergebenen Parent-Tab neu auf.

Diese müssen in `ui_main.cpp` registriert werden:
- In `rebuild_ui()`: `ui_<name>_reset();`
- In `ui_init()` im Tab-Switch: `case TAB_<NAME>: ui_build_<name>(pages[i]); break;`
- In `ui_update_visible_page()`: `case TAB_<NAME>: ui_update_<name>(); break;`

---

## 5. Web-Interface Integration

Neue Theme-Variablen werden wie folgt durchgängig integriert:
1. **`ThemeConfig` in `system_state.h`:** Feld hinzufügen.
2. **Default-Initialisierung in `system_state.h`:** Standardwert in `theme_config_init_default()`.
3. **NVS-Persistierung in `system_state.cpp`:** Sicherstellen, dass die struct-Größe passt (`put_bytes_if_changed`).
4. **REST-API in `web_settings.cpp`:** In `GET /api/theme` und `POST /api/theme` serialisieren/deserialisieren.
5. **Web-UI in `web_settings_html.h`:** Color-Picker / Slider anlegen und in `updateThemePreview()`, `themePresets` sowie `getThemePayload()` einbinden.

