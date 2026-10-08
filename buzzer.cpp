#include "buzzer.h"
#include "WS_CH32_IO.h"
#include "system_state.h"
#include <Wire.h>
#include <HWCDC.h>

extern TwoWire Wire;
extern HWCDC USBSerial;

static uint32_t buzzer_off_ms = 0;
static bool buzzer_is_active = false;

void buzzer_init() {
    // Ensure buzzer is muted at startup
    WS_CH32_IO::writeRegister(Wire, WS_CH32_IO::REG_OUTPUT, WS_CH32_IO::OUT_DISPLAY_ON);
}

void buzzer_beep(uint16_t duration_ms) {
    if (!state.buzzer_enabled) return;

    if (duration_ms <= 10) {
        // Immediate precise micro-pulse for a minimal crisp haptic click (no loop/render jitter)
        WS_CH32_IO::writeRegister(Wire, WS_CH32_IO::REG_OUTPUT, WS_CH32_IO::OUT_DISPLAY_ON | WS_CH32_IO::PIN_BEE_EN);
        delayMicroseconds((uint32_t)duration_ms * 1000);
        WS_CH32_IO::writeRegister(Wire, WS_CH32_IO::REG_OUTPUT, WS_CH32_IO::OUT_DISPLAY_ON);
        buzzer_is_active = false;
    } else {
        // Asynchronous longer beep for alerts/diagnostics
        buzzer_off_ms = millis() + duration_ms;
        if (!buzzer_is_active) {
            buzzer_is_active = true;
            WS_CH32_IO::writeRegister(Wire, WS_CH32_IO::REG_OUTPUT, WS_CH32_IO::OUT_DISPLAY_ON | WS_CH32_IO::PIN_BEE_EN);
        }
    }
}

void buzzer_loop() {
    if (buzzer_is_active && millis() >= buzzer_off_ms) {
        buzzer_is_active = false;
        WS_CH32_IO::writeRegister(Wire, WS_CH32_IO::REG_OUTPUT, WS_CH32_IO::OUT_DISPLAY_ON);
    }
}
