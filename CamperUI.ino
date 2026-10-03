#include <lvgl.h>
#include "Arduino_GFX_Library.h"
#include "lv_conf.h"
#include "HWCDC.h"
#include "TouchDrvGT911.hpp"
#include <Wire.h>
#include <SPI.h>
#include "WS_CH32_IO.h"
#include "ui_main.h"
#include "http_handler.h"
#include "system_state.h"

// Hardware and Touch Controller
TouchDrvGT911 GT911;
int16_t x[5], y[5];
uint8_t gt911_i2c_addr = 0;
bool gt911_available = false;
bool display_is_on = true;

HWCDC USBSerial;
#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

uint32_t screenWidth;
uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;

// Display Bus & RGB Panel Configuration for Waveshare ESP32-S3-Touch-LCD-4 (480x480 ST7701S)
Arduino_DataBus *bus = new Arduino_SWSPI(
    GFX_NOT_DEFINED /* DC */, 42 /* CS */,
    2 /* SCK */, 1 /* MOSI */, GFX_NOT_DEFINED /* MISO */);

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40 /* DE */, 39 /* VSYNC */, 38 /* HSYNC */, 41 /* PCLK */,
    46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
    14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */, 9 /* G5 */,
    5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */,
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */,
    0 /* pclk_active_neg */, 12000000 /* prefer_speed */);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    480 /* width */, 480 /* height */, rgbpanel, 2 /* rotation */, true /* auto_flush */,
    bus, GFX_NOT_DEFINED /* RST */, st7701_type1_init_operations, sizeof(st7701_type1_init_operations));

#if LV_USE_LOG != 0
void my_print(const char *buf) {
    USBSerial.printf("%s", buf);
    USBSerial.flush();
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

    lv_disp_flush_ready(disp_drv);
}

void example_increase_lvgl_tick(void *arg) {
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

bool ignore_touch_until_release = false;

// Read touch coordinates from GT911
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    if (!gt911_available) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    uint8_t touched = GT911.getPoint(x, y, GT911.getSupportTouchPoint());

    if (touched > 0) {
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
                touchY = gfx->height() - x[0];
                break;
            case 2:
                touchX = gfx->width() - x[0];
                touchY = gfx->height() - y[0];
                break;
            case 3:
                touchX = gfx->width() - y[0];
                touchY = x[0];
                break;
        }

        data->point.x = touchX;
        data->point.y = touchY;
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
    delay(500);
    USBSerial.println("\n--- Waveshare ESP32-S3 CamperUI Starting ---");
    USBSerial.printf("CPU frequency: %u MHz\n", (unsigned)getCpuFrequencyMhz());

    state_init();
    http_init();

    // Initialize CH32V003 IO expander
    if (!WS_CH32_IO::begin(Wire, WS_CH32_IO::DEFAULT_I2C_SDA, WS_CH32_IO::DEFAULT_I2C_SCL,
                           WS_CH32_IO::DEFAULT_I2C_FREQ, &USBSerial)) {
        USBSerial.println("CH32V003 IO expander init failed");
    } else {
        USBSerial.println("CH32V003 IO expander initialized");
        delay(50);
        WS_CH32_IO::initDisplayPower(Wire);
        delay(20);
        USBSerial.printf("Setting PWM to %d\n", state.display_brightness);
        uint8_t pwm = 255 - (state.display_brightness * 255 / 100);
        WS_CH32_IO::setPwm(Wire, pwm);
    }

    // Initialize Touch Controller GT911
    gt911_available = init_gt911_with_probe(15, 7);
    if (gt911_available) {
        GT911.setMaxTouchPoint(1);
    } else {
        USBSerial.println("GT911 touch not detected.");
    }

    // Initialize Display
    gfx->begin();
    screenWidth = gfx->width();
    screenHeight = gfx->height();

    // Initialize LVGL
    lv_init();

    // Use a SINGLE buffer in fast internal RAM to prevent PSRAM DMA starvation
    // and keep LVGL fast! (1/4 screen = 115KB)
    uint32_t buf_size = screenWidth * screenHeight / 4;
    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!buf1) buf1 = (lv_color_t *)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

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
    disp_drv.sw_rotate = 1;
    lv_disp_drv_register(&disp_drv);

    if (gt911_available) {
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = my_touchpad_read;
        lv_indev_drv_register(&indev_drv);
    }

    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);

    WiFi.begin(state.wifi_ssid.c_str(), state.wifi_pass.c_str());

    ui_init();

    // Start background network worker on Core 0 (Core 1 is 100% dedicated to UI)
    http_start_task();

    USBSerial.println("CamperUI ready!");
}

static uint32_t last_ui_update = 0;

void loop() {
    lv_timer_handler();

    if (display_is_on && state.display_timeout > 0 && lv_disp_get_inactive_time(NULL) > (uint32_t)(state.display_timeout * 1000)) {
        display_is_on = false;
        WS_CH32_IO::setPwm(Wire, 255); // 255 is OFF for inverted PWM
    }

    if (USBSerial.available()) {
        char c = USBSerial.read();
        if (c == 'd' || c == 'D') {
            state.debug_mode = !state.debug_mode;
            state_save();
            USBSerial.printf("[COMMAND] Debug Simulation Mode: %s\n", state.debug_mode ? "AKTIV (Dummy-Daten)" : "INAKTIV (Live HTTP)");
        }
    }

    // Update UI dynamically
    static uint32_t last_ui_ms = 0;
    if (display_is_on && millis() - last_ui_ms > 1000) {
        last_ui_ms = millis();
        ui_update_data();
        
        // Also call sub-updates
        extern void ui_update_power_tab();
        extern void ui_update_water_tab();
        extern void ui_update_climate_tab();
        extern void ui_update_switches_tab();
        extern void ui_update_dimmers_tab();
        extern void ui_update_level_tab();
        extern void ui_update_settings_tab();
        
        ui_update_power_tab();
        ui_update_water_tab();
        ui_update_climate_tab();
        ui_update_switches_tab();
        ui_update_dimmers_tab();
        ui_update_level_tab();
        ui_update_settings_tab();
    }
    
    delay(2);
}
