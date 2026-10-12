#include "water_control.h"
#include "system_state.h"
#include "buzzer.h"
#include "http_handler.h"

static bool is_pump_on() {
    if (state.pump_relay >= 0 && state.pump_relay < 8) {
        return state.switch_state[state.pump_relay];
    } else if (state.pump_relay >= 8 && state.pump_relay < 16) {
        return state.wrelay_state[state.pump_relay - 8];
    }
    return false;
}

static void turn_off_pump() {
    if (state.pump_relay >= 0 && state.pump_relay < 8) {
        state.switch_state[state.pump_relay] = false;
        http_publish_switch(state.pump_relay, false);
    } else if (state.pump_relay >= 8 && state.pump_relay < 16) {
        int idx = state.pump_relay - 8;
        state.wrelay_state[idx] = false;
        http_publish_wrelay(idx, false);
    }
}

// ---------------------------------------------------------------------------
// 1. Auto-Off Timer
// ---------------------------------------------------------------------------
static uint32_t pump_on_start_ms = 0;
static bool last_pump_on = false;

static void update_auto_off() {
    bool on = is_pump_on();
    uint32_t now = millis();

    if (on && !last_pump_on) {
        pump_on_start_ms = now;
    } else if (!on) {
        pump_on_start_ms = 0;
    }
    last_pump_on = on;

    if (on && state.pump_auto_off_sec > 0 && pump_on_start_ms > 0) {
        uint32_t elapsed_s = (now - pump_on_start_ms) / 1000;
        if (elapsed_s >= (uint32_t)state.pump_auto_off_sec) {
            Serial.printf("[WATER] Auto-Off Timeout (%d s) erreicht! Pumpe wird abgeschaltet.\n", state.pump_auto_off_sec);
            turn_off_pump();
            pump_on_start_ms = 0;
            last_pump_on = false;
            buzzer_beep_force(120);
        }
    }
}

int water_control_get_remaining_sec() {
    if (!is_pump_on() || state.pump_auto_off_sec <= 0 || pump_on_start_ms == 0) {
        return 0;
    }
    uint32_t elapsed_s = (millis() - pump_on_start_ms) / 1000;
    if (elapsed_s >= (uint32_t)state.pump_auto_off_sec) return 0;
    return (int)(state.pump_auto_off_sec - elapsed_s);
}

// ---------------------------------------------------------------------------
// 2. Trockenlaufschutz (Dry-Run Protection)
// ---------------------------------------------------------------------------
static bool dry_run_active = false;

static void update_dry_run() {
    if (state.dry_run_protect_pct <= 0) {
        dry_run_active = false;
        return;
    }

    int fresh_level = state.tank_level[0];
    if (fresh_level <= state.dry_run_protect_pct) {
        dry_run_active = true;
        if (is_pump_on()) {
            Serial.printf("[WATER] Trockenlaufschutz! Frischwasser bei %d%% <= %d%%. Pumpe sofort abgeschaltet.\n",
                          fresh_level, state.dry_run_protect_pct);
            turn_off_pump();
            pump_on_start_ms = 0;
            buzzer_beep_force(150);
        }
    } else if (fresh_level > state.dry_run_protect_pct + 1) {
        dry_run_active = false;
    }
}

bool water_control_is_dry_run_active() {
    return dry_run_active;
}

// ---------------------------------------------------------------------------
// 3. Befüll-Assistent (Tank Fill Helper)
// ---------------------------------------------------------------------------
static int last_fill_level = -1;
static bool fill_assist_alerted = false;

enum FillAlertState {
    ALERT_IDLE = 0,
    ALERT_BUZZER,
    ALERT_RELAY,
    ALERT_DIMMER
};
static FillAlertState alert_state = ALERT_IDLE;
static uint32_t alert_step_ms = 0;
static int alert_step = 0;
static int orig_dimmer_val = 0;
static bool orig_relay_state = false;

static void trigger_fill_alert() {
    Serial.printf("[WATER] Befüll-Assistent: Ziel %d%% erreicht! Ausgelöste Aktion: %d\n",
                  state.fill_assist_target_pct, state.fill_assist_action_type);
    alert_step = 0;
    alert_step_ms = millis();

    if (state.fill_assist_action_type == 0) {
        // Buzzer: 5 Doppel-Pieptöne
        alert_state = ALERT_BUZZER;
    } else if (state.fill_assist_action_type == 1) {
        // Relais: für 5 Sekunden aktivieren
        alert_state = ALERT_RELAY;
        int r = state.fill_assist_relay;
        if (r >= 0 && r < 8) {
            orig_relay_state = state.switch_state[r];
            state.switch_state[r] = true;
            http_publish_switch(r, true);
        } else if (r >= 8 && r < 16) {
            orig_relay_state = state.wrelay_state[r - 8];
            state.wrelay_state[r - 8] = true;
            http_publish_wrelay(r - 8, true);
        }
    } else if (state.fill_assist_action_type == 2) {
        // Dimmer: 5x pulsieren
        alert_state = ALERT_DIMMER;
        int d = constrain(state.fill_assist_dimmer, 0, 3);
        orig_dimmer_val = state.dimmer_val[d];
        state.dimmer_hold_until[d] = millis() + 8000;
    }
}

static void update_fill_alert_state_machine() {
    if (alert_state == ALERT_IDLE) return;
    uint32_t now = millis();

    if (alert_state == ALERT_BUZZER) {
        int cycle = alert_step / 4;
        int sub = alert_step % 4;

        if (cycle >= 5) {
            alert_state = ALERT_IDLE;
            return;
        }

        if (sub == 0 && now >= alert_step_ms) {
            buzzer_beep_force(50);
            alert_step_ms = now + 120;
            alert_step++;
        } else if (sub == 1 && now >= alert_step_ms) {
            alert_step_ms = now;
            alert_step++;
        } else if (sub == 2 && now >= alert_step_ms) {
            buzzer_beep_force(50);
            alert_step_ms = now + 400; // Pause bis zum nächsten Doppelton
            alert_step++;
        } else if (sub == 3 && now >= alert_step_ms) {
            alert_step_ms = now;
            alert_step++;
        }
    } else if (alert_state == ALERT_RELAY) {
        if (now - alert_step_ms >= 5000) {
            int r = state.fill_assist_relay;
            if (r >= 0 && r < 8) {
                state.switch_state[r] = orig_relay_state;
                http_publish_switch(r, orig_relay_state);
            } else if (r >= 8 && r < 16) {
                state.wrelay_state[r - 8] = orig_relay_state;
                http_publish_wrelay(r - 8, orig_relay_state);
            }
            alert_state = ALERT_IDLE;
        }
    } else if (alert_state == ALERT_DIMMER) {
        int d = constrain(state.fill_assist_dimmer, 0, 3);
        if (now >= alert_step_ms) {
            if (alert_step < 10) {
                bool high = (alert_step % 2 == 0);
                int target_val = high ? 100 : 0;
                http_publish_dimmer(d, target_val);
                alert_step++;
                alert_step_ms = now + 500;
            } else {
                http_publish_dimmer(d, orig_dimmer_val);
                alert_state = ALERT_IDLE;
            }
        }
    }
}

static void update_fill_assist() {
    if (!state.fill_assist_enabled) {
        fill_assist_alerted = false;
        last_fill_level = -1;
        return;
    }

    int level = state.tank_level[0];
    int target = state.fill_assist_target_pct;

    if (last_fill_level < 0) {
        last_fill_level = level;
        return;
    }

    if (last_fill_level < target && level >= target && !fill_assist_alerted) {
        fill_assist_alerted = true;
        trigger_fill_alert();
    }

    if (level <= (target - 3)) {
        fill_assist_alerted = false;
    }

    last_fill_level = level;
}

void water_control_init() {
    pump_on_start_ms = 0;
    last_pump_on = false;
    dry_run_active = false;
    fill_assist_alerted = false;
    last_fill_level = -1;
    alert_state = ALERT_IDLE;
}

void water_control_loop() {
    update_auto_off();
    update_dry_run();
    update_fill_assist();
    update_fill_alert_state_machine();
}

bool water_control_is_fill_assist_alerting() {
    return (alert_state != ALERT_IDLE);
}
