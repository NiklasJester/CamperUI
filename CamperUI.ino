#include <lvgl.h>
#include "Arduino_GFX_Library.h"
#include "CamperRGBDisplay.h"
#include "display_sync.h"
#include "wifi_diagnostics.h"
#include "lv_conf.h"
#include "HWCDC.h"
#include "TouchDrvGT911.hpp"
#include <Wire.h>
#include "WS_CH32_IO.h"
#include "ui_main.h"
#include "http_handler.h"
#include "system_state.h"
#include "web_ota.h"
#include "buzzer.h"
#include "debug_log.h"

// Hardware and Touch Controller
TouchDrvGT911 GT911;
int16_t x[5], y[5];
uint8_t gt911_i2c_addr = 0;
bool gt911_available = false;
bool display_is_on = true;

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

uint32_t screenWidth;
uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;

// Display Bus & RGB Panel Configuration for Waveshare ESP32-S3-Touch-LCD-4 (480x480 ST7701S)
Arduino_DataBus *bus = new Arduino_SWSPI(
    GFX_NOT_DEFINED /* DC */, 42 /* CS */,
    2 /* SCK */, 1 /* MOSI */, GFX_NOT_DEFINED /* MISO */);

CamperRGBPanel *rgbpanel = new CamperRGBPanel(
    40 /* DE */, 39 /* VSYNC */, 38 /* HSYNC */, 41 /* PCLK */,
    46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
    14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */, 9 /* G5 */,
    5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */,
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */,
    0 /* pclk_active_neg */, 12000000 /* prefer_speed: restore original panel timing */);

CamperRGBDisplay *gfx = new CamperRGBDisplay(
    480 /* width */, 480 /* height */, rgbpanel, 2 /* rotation */, true /* auto_flush */,
    bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations));

#if LV_USE_LOG != 0
void my_print(const char *buf) {
    USBSerial.printf("%s", buf);
}
#endif

// Display flushing callback
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

    // Prevent scanout drift from accumulating across LVGL refreshes.
    // This only schedules the IDF restart for VSYNC; it does not change
    // widget coordinates or reinitialize/allocate a second framebuffer.
    if (lv_disp_flush_is_last(disp_drv)) {
        esp_err_t result = rgbpanel->restartTransmission();
        static bool error_reported = false;
        if (result != ESP_OK && !error_reported) {
            error_reported = true;
            USBSerial.printf("[DISPLAY v8.7] Refresh resync failed: %s\n", esp_err_to_name(result));
        }
    }
    lv_disp_flush_ready(disp_drv);
}

void example_increase_lvgl_tick(void *arg) {
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

bool ignore_touch_until_release = false;
bool touch_input_enabled = true;

// Feedback on valid touch interactions (buttons, switches, tabs)
static void my_touchpad_feedback(lv_indev_drv_t *indev_driver, uint8_t event_code) {
    if (event_code == LV_EVENT_CLICKED) {
        buzzer_beep(3);
    }
}

// Read touch coordinates from GT911
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    // Keep release samples in range too: LVGL checks coordinates even on release.
    static lv_point_t last_valid_point = {0, 0};
    data->point = last_valid_point;
    if (!gt911_available || !touch_input_enabled) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    uint8_t touched = GT911.getPoint(x, y, GT911.getSupportTouchPoint());

    if (touched > 0) {
        if (touched > GT911.getSupportTouchPoint() || x[0] < 0 || y[0] < 0 ||
            x[0] >= gfx->width() || y[0] >= gfx->height()) {
            // Invalid input is a release, never a fabricated edge touch.
            static uint32_t last_invalid_log = 0;
            static bool invalid_logged = false;
            if (!invalid_logged || millis() - last_invalid_log >= 1000) {
                invalid_logged = true;
                last_invalid_log = millis();
                USBSerial.printf("[TOUCH v8.7] Rejected sample: count=%u raw=(%d,%d)\n",
                                 (unsigned)touched, (int)x[0], (int)y[0]);
            }
            data->state = LV_INDEV_STATE_REL;
            return;
        }
        if (!display_is_on) {
            // Wake up display, but don't pass the click to the UI!
            display_is_on = true;
            ignore_touch_until_release = true;
            uint8_t pwm = 255 - (state.display_brightness * 255 / 100);
            WS_CH32_IO::setPwm(Wire, pwm);
            lv_disp_trig_activity(NULL); // Reset LVGL inactivity timer
            data->state = LV_INDEV_STATE_REL;
            return;
        }

        if (ignore_touch_until_release) {
            data->state = LV_INDEV_STATE_REL;
            return;
        }

        int16_t touchX = x[0];
        int16_t touchY = y[0];

        switch (gfx->getRotation()) {
            case 0:
                break;
            case 1:
                touchX = y[0];
                touchY = gfx->height() - 1 - x[0];
                break;
            case 2:
                touchX = gfx->width() - 1 - x[0];
                touchY = gfx->height() - 1 - y[0];
                break;
            case 3:
                touchX = gfx->width() - 1 - y[0];
                touchY = x[0];
                break;
        }

        data->point.x = touchX;
        data->point.y = touchY;
        last_valid_point = data->point;
        data->state = LV_INDEV_STATE_PR;
    } else {
        ignore_touch_until_release = false;
        data->state = LV_INDEV_STATE_REL;
    }
}

void i2c_scan() {
    USBSerial.println("Scanning I2C bus...");
    byte error, address;
    int nDevices = 0;

    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            USBSerial.print("I2C device found at 0x");
            if (address < 16) USBSerial.print("0");
            USBSerial.println(address, HEX);
            nDevices++;

            if (address == GT911_SLAVE_ADDRESS_L || address == GT911_SLAVE_ADDRESS_H) {
                gt911_i2c_addr = address;
                USBSerial.print("Found GT911 candidate: 0x");
                USBSerial.println(address, HEX);
            }
        }
    }
    if (nDevices == 0) {
        USBSerial.println("No I2C devices found");
    }
}

bool init_gt911_with_probe(int sda_pin, int scl_pin) {
    Wire.begin(sda_pin, scl_pin);
    delay(100);

    i2c_scan();

    if (gt911_i2c_addr == 0) {
        USBSerial.println("GT911 not found in I2C scan");
        return false;
    }

    GT911.setPins(-1, -1);
    if (GT911.begin(Wire, gt911_i2c_addr, sda_pin, scl_pin)) {
        USBSerial.print("GT911 initialized at 0x");
        USBSerial.println(gt911_i2c_addr, HEX);
        return true;
    } else {
        USBSerial.println("Failed to init GT911");
        return false;
    }
}

#include <WiFi.h>

void setup() {
    // 240 MHz full dual-core speed for maximum smooth LVGL UI rendering & touch responsiveness
    setCpuFrequencyMhz(240);

    USBSerial.begin(115200);
    USBSerial.setTxTimeoutMs(0); // Never block if USB CDC host is not actively reading
    delay(200);
    USBSerial.println("\n--- Waveshare ESP32-S3 CamperUI Starting ---");
    USBSerial.printf("CPU frequency: %u MHz\n", (unsigned)getCpuFrequencyMhz());

    state_init();
    http_init();

    // Initialize CH32V003 IO expander (powers on LCD and resets GT911/ST7701S)
    if (!WS_CH32_IO::begin(Wire, WS_CH32_IO::DEFAULT_I2C_SDA, WS_CH32_IO::DEFAULT_I2C_SCL,
                           WS_CH32_IO::DEFAULT_I2C_FREQ, &USBSerial)) {
        USBSerial.println("CH32V003 IO expander init failed");
    } else {
        USBSerial.println("CH32V003 IO expander initialized");
        delay(20);
        USBSerial.printf("Setting PWM to %d\n", state.display_brightness);
        uint8_t pwm = 255 - (state.display_brightness * 255 / 100);
        WS_CH32_IO::setPwm(Wire, pwm);
        buzzer_init();
    }

    // Initialize Touch Controller GT911
    gt911_available = init_gt911_with_probe(15, 7);
    if (gt911_available) {
        GT911.setMaxTouchPoint(1);
    } else {
        USBSerial.println("GT911 touch not detected.");
    }

    // Initialize Display
    if (!gfx->begin()) {
        USBSerial.println("[DISPLAY v8.7] FATAL: RGB display initialization failed");
        while (true) delay(1000);
    }
    screenWidth = gfx->width();
    screenHeight = gfx->height();

    // Reserve WiFi and data-worker allocations before the optional larger
    // draw buffer. Otherwise the UI may start while its data task cannot.
    // Credentials already persist in CamperUI Preferences; avoid a second
    // Flash write by the WiFi driver when reconnecting from Settings.
    wifi_diagnostics_init();
    wifi_connect_configured();
    const bool data_worker_started = http_start_task();
    USBSerial.printf("[DATA v8.7] Source=%s worker=%s\n",
                     state.debug_mode ? "DEMO" : "LIVE HTTP",
                     data_worker_started ? "RUNNING" : "FAILED");

    // Initialize LVGL
    lv_init();

    // Keep LVGL's working buffer separate from the RGB framebuffer in PSRAM.
    // A small strip avoids a silent PSRAM fallback and leaves internal RAM
    // available for the RGB driver's bounce buffers and network worker.
    const uint32_t preferred_bytes = screenWidth * 40 * sizeof(lv_color_t);
    const uint32_t internal_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    // Reserve headroom for later UI/network allocations; never use PSRAM.
    const bool has_headroom = data_worker_started && heap_caps_get_free_size(internal_caps) >= preferred_bytes + 40960 &&
                              heap_caps_get_largest_free_block(internal_caps) >= preferred_bytes;
    uint32_t buf_size = screenWidth * (has_headroom ? 20 : 16);
    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!buf1 && buf_size == screenWidth * 20) {
        buf_size = screenWidth * 16;
        buf1 = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), internal_caps);
    }
    if (!buf1) {
        USBSerial.println("[DISPLAY v8.7] FATAL: internal LVGL draw buffer allocation failed");
        // Never register a null buffer or silently increase PSRAM contention.
        while (true) delay(1000);
    }
    USBSerial.printf("[DISPLAY v8.7] LVGL draw buffer: %u bytes, internal RAM only\n",
                     (unsigned)(buf_size * sizeof(lv_color_t)));

#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print);
#endif

    // Only pass buf1 (single buffer mode)
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, buf_size);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.sw_rotate = 0; // Handled directly in fast driver layer with 32-bit burst writes
    lv_disp_drv_register(&disp_drv);

    if (gt911_available) {
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = my_touchpad_read;
        indev_drv.feedback_cb = my_touchpad_feedback;
        lv_indev_drv_register(&indev_drv);
    }

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);



    ui_fonts_init();
    ui_init();
    web_ota_init();
    display_request_resync();

    // Report memory after UI construction; the data worker started earlier.

    USBSerial.printf("[DISPLAY v8.7] Internal heap after startup: free=%u largest=%u bytes\n",
                     (unsigned)heap_caps_get_free_size(internal_caps),
                     (unsigned)heap_caps_get_largest_free_block(internal_caps));

    USBSerial.println("CamperUI ready!");
    USBSerial.println("[DISPLAY v8.7] Adaptive internal draw buffer active; f=framebuffer cache flush, s=RGB resync");
    USBSerial.println("[DISPLAY v8.7] RGB pixel clock: 12 MHz (original panel timing)");
    USBSerial.println("[TOUCH v8.7] n=geometry/memory, t=touch on/off, r=full screen redraw");
}

static uint32_t last_ui_update = 0;
static bool display_resync_pending = false;

void display_request_resync() {
    display_resync_pending = true;
}

void loop() {
    if (web_ota_is_updating()) {
        web_ota_loop();
        delay(1);
        return;
    }

    if (display_resync_pending) {
        display_resync_pending = false;
        // Flash writes have returned and cache access is available again.
        // The IDF driver applies the restart at the next VSYNC, not mid-frame.
        gfx->flush(true);
        esp_err_t result = rgbpanel->restartTransmission();
        USBSerial.printf("[DISPLAY v8.7] RGB resync scheduled after save/manual command: %s\n",
                         esp_err_to_name(result));
    }
    uint32_t wait_ms = lv_timer_handler();

    if (display_is_on && state.display_timeout > 0 && lv_disp_get_inactive_time(NULL) > (uint32_t)(state.display_timeout * 1000)) {
        display_is_on = false;
        WS_CH32_IO::setPwm(Wire, 255); // 255 is OFF for inverted PWM
    }

    if (USBSerial.available()) {
        char c = USBSerial.read();
        if (c == 't' || c == 'T') {
            touch_input_enabled = !touch_input_enabled;
            ignore_touch_until_release = true;
            lv_indev_reset(NULL, NULL);
            USBSerial.printf("[TOUCH v8.7] Touch %s (temporary, until reboot)\n",
                             touch_input_enabled ? "ON" : "OFF");
        }
        if (c == 's' || c == 'S') display_request_resync();
        if (c == 'f' || c == 'F') {
            gfx->flush(true);
            USBSerial.println("[DISPLAY v8.7] Framebuffer cache flush requested (no redraw)");
        }
        if (c == 'r' || c == 'R') {
            lv_obj_invalidate(lv_scr_act());
            USBSerial.println("[NAV v8.7] Full screen redraw requested");
        }
        if (c == 'n' || c == 'N') {
            ui_debug_navigation("serial", true);
            USBSerial.printf("[NAV v8] LVGL pool integrity: %s\n", lv_mem_test() == LV_RES_OK ? "OK" : "FAILED");
        }
        if (c == 'd' || c == 'D') {
            state.debug_mode = !state.debug_mode;
            state_save();
            USBSerial.printf("[COMMAND] Debug Simulation Mode: %s\n", state.debug_mode ? "AKTIV (Dummy-Daten)" : "INAKTIV (Live HTTP)");
        }
        if (c == 'w' || c == 'W') {
            USBSerial.printf("[WIFI] Status: %d, SSID: '%s', IP: %s, RSSI: %d dBm | FreeInternal: %u, FreePSRAM: %u\n",
                             WiFi.status(), state.wifi_ssid.c_str(), WiFi.localIP().toString().c_str(),
                             WiFi.RSSI(),
                             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        }
        if (c == 'b' || c == 'B') {
            buzzer_beep(50);
            USBSerial.println("[BUZZER] Test-Beep (50ms)");
        }
    }

    buzzer_loop();

    // Update UI dynamically
    static uint32_t last_ui_ms = 0;
    if (display_is_on && millis() - last_ui_ms > 1000) {
        last_ui_ms = millis();
        {
            StateLockGuard lock;
            ui_update_data();
            ui_update_visible_page();
            // Complete asynchronous WLAN scans even after leaving Settings.
            ui_update_settings_tab();
        }
    }
    
    web_ota_loop();
    if (wait_ms > 5) wait_ms = 5;
    if (wait_ms > 0) {
        delay(wait_ms);
    } else {
        yield();
    }
}
