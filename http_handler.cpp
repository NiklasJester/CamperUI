#include "http_handler.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "system_state.h"
#include "HWCDC.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

extern HWCDC USBSerial;
#define Serial USBSerial

struct HttpCmd {
    char path[64];
};
static QueueHandle_t http_cmd_queue = NULL;

static void queue_cmd(const char* path) {
    if (state.debug_mode || !http_cmd_queue) return;
    HttpCmd cmd;
    strncpy(cmd.path, path, sizeof(cmd.path) - 1);
    cmd.path[sizeof(cmd.path) - 1] = '\0';
    xQueueSend(http_cmd_queue, &cmd, 0);
}

unsigned long last_http_poll = 0;
static int poll_step = 0;

// Replace UTF-8 encoded German umlauts with ASCII equivalents
// because the LVGL font does not include umlaut glyphs.
// We iterate byte-by-byte looking for the 2-byte UTF-8 sequences.
static String fix_umlauts(const String &in) {
    String out;
    out.reserve(in.length());
    for (unsigned int i = 0; i < in.length(); i++) {
        uint8_t c = (uint8_t)in.charAt(i);
        if (c == 0xC3 && (i + 1) < in.length()) {
            uint8_t c2 = (uint8_t)in.charAt(i + 1);
            switch (c2) {
                case 0xA4: out += "ae"; i++; continue; // ä
                case 0xB6: out += "oe"; i++; continue; // ö
                case 0xBC: out += "ue"; i++; continue; // ü
                case 0x84: out += "Ae"; i++; continue; // Ä
                case 0x96: out += "Oe"; i++; continue; // Ö
                case 0x9C: out += "Ue"; i++; continue; // Ü
                case 0x9F: out += "ss"; i++; continue; // ß
                default: break;
            }
        }
        out += in.charAt(i);
    }
    return out;
}

static String get_url() {
    return "http://" + state.vanpi_ip + ":1880";
}

// Node-RED globals may be stored as strings or numbers.
// These helpers handle both: "13.34" (string) and 13.34 (number), including German comma format.
static float jsonFloat(JsonVariant v) {
    if (v.isNull()) return 0.0f;
    String s = v.as<String>();
    s.trim();
    s.replace(',', '.');
    return s.toFloat();
}

static int jsonInt(JsonVariant v) {
    if (v.isNull()) return 0;
    String s = v.as<String>();
    s.trim();
    return s.toInt();
}

void http_init() {
    if (!http_cmd_queue) {
        http_cmd_queue = xQueueCreate(16, sizeof(HttpCmd));
    }
}

void http_fetch_names() {
    // Names come embedded in /relay and /dimmer responses
}

// --- Parsers ---
// GET /batt -> { "VoltB": float|str, "Ampere": float|str, "battsoc": int|str }
static void parse_batt_json(String payload) {
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, payload)) return;

    if (doc.containsKey("VoltB")) { state.bat_voltage = jsonFloat(doc["VoltB"]); state.battery_fields |= 1; }
    if (doc.containsKey("Ampere")) { state.bat_current = jsonFloat(doc["Ampere"]); state.battery_fields |= 2; }
    if (doc.containsKey("battsoc")) { state.bat_soc = jsonInt(doc["battsoc"]); state.battery_fields |= 4; }
    // Optional extension, not assumed to exist in the standard VanPi /batt response.
    if (doc.containsKey("starter_voltage")) { state.starter_voltage = jsonFloat(doc["starter_voltage"]); state.battery_fields |= 8; }
}

// GET /mppt/ -> { "mppt_pv_watts": float, "mppt_pv_amps": float, "mppt_pv_volts": float }
static void parse_mppt_json(String payload) {
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, payload)) return;

    if (doc.containsKey("mppt_pv_watts")) { state.solar_power = jsonFloat(doc["mppt_pv_watts"]); state.solar_fields |= 1; }
    if (doc.containsKey("mppt_pv_amps")) { state.solar_current = jsonFloat(doc["mppt_pv_amps"]); state.solar_fields |= 2; }
    if (doc.containsKey("mppt_pv_volts")) { state.solar_voltage = jsonFloat(doc["mppt_pv_volts"]); state.solar_fields |= 4; }
}

// GET /relay -> { "Relay1": { "state": bool, "name": str, ... }, "Relay2": ... }
static void parse_relay_json(String payload) {
    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, payload)) return;

    for (int i = 0; i < 8; i++) {
        String key = "Relay" + String(i + 1);
        if (!doc.containsKey(key)) continue;
        JsonObject r = doc[key];
        if (r.containsKey("state")) {
            if ((int32_t)(millis() - state.relay_hold_until[i]) >= 0) state.switch_state[i] = r["state"].as<bool>();
            state.relay_fields |= (1 << i);
        }
        if (r.containsKey("name")) {
            String n = r["name"].as<String>();
            if (n.length() > 0 && n != key) state.switch_names[i] = fix_umlauts(n);
        }
    }
}

// GET /dimmer -> { "dimmer1": { "state": int(0-100), "name": str, ... }, "dimmer2": ... }
static void parse_dimmer_json(String payload) {
    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, payload)) return;

    for (int i = 0; i < 8; i++) {
        String key = "dimmer" + String(i + 1);
        if (!doc.containsKey(key)) continue;
        JsonObject d = doc[key];
        if (d.containsKey("state") && (int32_t)(millis() - state.dimmer_hold_until[i]) >= 0) {
            state.dimmer_val[i] = constrain(jsonInt(d["state"]), 0, 100);
            state.dimmer_fields |= (1 << i);
        }
        if (d.containsKey("name")) {
            String n = d["name"].as<String>();
            if (n.length() > 0 && n != key) state.dimmer_names[i] = fix_umlauts(n);
        }
    }
}

// GET /level -> { "level1": { "state": int, "name": str }, ... }
static void parse_level_json(String payload) {
    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, payload)) return;

    for (int i = 0; i < 4; i++) {
        String key = "level" + String(i + 1);
        if (!doc.containsKey(key)) continue;
        JsonObject lvl = doc[key];
        if (lvl.containsKey("state")) { state.tank_level[i] = constrain(jsonInt(lvl["state"]), 0, 100); state.tank_fields |= (1 << i); }
        if (lvl.containsKey("name")) {
            String n = lvl["name"].as<String>();
            if (n.length() > 0) state.tank_names[i] = fix_umlauts(n);
        }
    }
}

// GET /temp -> { "temp1": { "state": "23.5", "name": str }, "temp2": ... }
static void parse_temp_json(String payload) {
    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, payload)) return;

    for (int i = 0; i < 4; i++) {
        String key = "temp" + String(i + 1);
        if (!doc.containsKey(key)) continue;
        JsonObject t = doc[key];
        if (t.containsKey("name")) {
            String n = t["name"].as<String>();
            if (n.length() > 0 && n != key) state.temp_sensor_names[i] = fix_umlauts(n);
        }
        if (t.containsKey("state")) {
            state.temp_sensors[i] = jsonFloat(t["state"]);
            state.temp_fields |= (1 << i);
        }
    }

    int out_idx = constrain(state.outdoor_temp_sensor, 0, 3);
    state.outdoor_temp = state.temp_sensors[out_idx];
    int in_idx = (out_idx == 0) ? 1 : 0;
    state.indoor_temp = state.temp_sensors[in_idx];
    if (out_idx != 2 && in_idx != 2) {
        state.indoor_humidity = state.temp_sensors[2];
    }
}

// GET /heater -> { "autoterm1": { "heatertoggle": bool, "heatstatus": str, "heaterror": str,
//   "targettemp_vanpi": num, "mode": str, "powerlevel": int, "fanspeed": int }, ... }
// Real VanPi values (see docs/flows.json):
//   mode:       "temp mode" | "power mode" | "fan only" | "" / "off" (heater not running)
//   heatstatus: "standby" | "heating" | "ventilation" | "only fan" | "ignition 1" |
//               "heating glow plug1" | "cooling down" | "shutting down" | ...
static void parse_heater_json(String payload) {
    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, payload)) return;

    bool has_at = doc.containsKey("autoterm1");
    JsonObject at;
    if (has_at) at = doc["autoterm1"];

    // Status text (always updated, never user-controlled)
    String hs = "";
    if (has_at && at.containsKey("heatstatus")) {
        hs = at["heatstatus"].as<String>();
    } else if (doc.containsKey("heatstatus")) {
        hs = doc["heatstatus"].as<String>();
    }
    hs.trim();
    state.heater_status = hs;

    if (has_at && at.containsKey("heaterror")) {
        state.heater_error = at["heaterror"].as<String>();
    } else if (doc.containsKey("heaterror")) {
        state.heater_error = doc["heaterror"].as<String>();
    }

    // User is currently interacting with the heater controls -> don't overwrite
    if ((int32_t)(millis() - state.heater_hold_until) < 0) return;

    // Mode (only overwrite when the heater reports an active mode, so the
    // locally selected mode is kept while the heater is off)
    String m = "";
    if (has_at && at.containsKey("mode")) m = at["mode"].as<String>();
    else if (doc.containsKey("mode")) m = doc["mode"].as<String>();
    m.toLowerCase();
    if (m.indexOf("fan") >= 0 || m.indexOf("vent") >= 0) {
        state.heater_vent_mode = true;
        state.heater_power_mode = false;
    } else if (m.indexOf("power") >= 0) {
        state.heater_vent_mode = false;
        state.heater_power_mode = true;
    } else if (m.indexOf("temp") >= 0) {
        state.heater_vent_mode = false;
        state.heater_power_mode = false;
    }

    // Running state: Check all indicators from VanPi / Autoterm
    bool is_on = false;

    // 1. Check autoterm1 heatertoggle
    if (has_at && at.containsKey("heatertoggle")) {
        String ht = at["heatertoggle"].as<String>();
        ht.toLowerCase();
        if (ht == "true" || ht == "1") is_on = true;
    }
    // 2. Check top-level heatertoggle (from main VanPi dashboard/app)
    if (!is_on && doc.containsKey("heatertoggle")) {
        String ht = doc["heatertoggle"].as<String>();
        ht.toLowerCase();
        if (ht == "true" || ht == "1") is_on = true;
    }

    // 3. Check heatstatus string (active states: heating, running, ventilation, ignition, etc.)
    auto is_active_status = [](const String &s) {
        String l = s; l.toLowerCase();
        return (l.length() > 0 && l != "standby" && l != "wait" && l != "heater off" &&
                l != "unknown status" && l != "0" && l.indexOf("shutting") < 0 &&
                l.indexOf("error") < 0 && l.indexOf("flame-out") < 0 &&
                l.indexOf("no fuel") < 0);
    };

    if (is_active_status(hs)) {
        is_on = true;
    }
    if (!is_on && doc.containsKey("heatstatus") && is_active_status(doc["heatstatus"].as<String>())) {
        is_on = true;
    }

    state.heating_on = is_on;

    if (has_at && at.containsKey("targettemp_vanpi")) {
        float t = jsonFloat(at["targettemp_vanpi"]);
        if (t > 0) state.target_temp = t;
    } else if (doc.containsKey("targettemp_vanpi")) {
        float t = jsonFloat(doc["targettemp_vanpi"]);
        if (t > 0) state.target_temp = t;
    }

    if (state.heater_vent_mode) {
        if (has_at && at.containsKey("fanspeed")) {
            int f = jsonInt(at["fanspeed"]);
            if (f > 0) state.heater_power_level = f;
        }
    } else if (has_at && at.containsKey("powerlevel")) {
        int p = jsonInt(at["powerlevel"]);
        if (p > 0) state.heater_power_level = p;
    }
}

// GET /position_sensor/?request=true -> { "x_angle": float|str, "y_angle": float|str }
static void parse_position_json(String payload) {
    DynamicJsonDocument doc(1024);
    if (deserializeJson(doc, payload)) return;

    if (doc.containsKey("x_angle")) state.roll_angle  = jsonFloat(doc["x_angle"]);
    if (doc.containsKey("y_angle")) state.pitch_angle = jsonFloat(doc["y_angle"]);
}


// --- HTTP transport ---

static unsigned long vanpi_fail_backoff_until = 0;

static void fetch_endpoint(const char* endpoint, void (*parser)(String)) {
    if (WiFi.status() != WL_CONNECTED) {
        state.vanpi_connected = false;
        return;
    }
    if (state.vanpi_ip.length() < 7) {
        state.vanpi_connected = false;
        return;
    }

    // If host is unreachable, don't block loop() repeatedly; wait before retrying
    if (millis() < vanpi_fail_backoff_until) {
        return;
    }

    HTTPClient http;
    http.begin(get_url() + endpoint);
    // Short connect timeout (200ms) - in LAN WiFi, 200ms is more than enough
    // to fail fast if host is unreachable.
    http.setConnectTimeout(200);
    http.setTimeout(800);
    int code = http.GET();
    if (code == 200) {
        state.vanpi_connected = true;
        vanpi_fail_backoff_until = 0;
        String payload = http.getString();
        parser(payload);
    } else {
        state.vanpi_connected = false;
        // Host unreachable / refused: back off for 5 seconds before next network call
        vanpi_fail_backoff_until = millis() + 5000;
        Serial.printf("[HTTP] GET %s -> %d (pause 5s)\n", endpoint, code);
    }
    http.end();
}

void http_loop() {
    state.wifi_connected = WiFi.status() == WL_CONNECTED;
    state.wifi_rssi = state.wifi_connected ? WiFi.RSSI() : 0;
    static bool demo_relays_seeded = false;
    static bool demo_dimmers_seeded = false;
    if (state.data_is_demo != state.debug_mode) {
        state.battery_fields = state.solar_fields = state.temp_fields = state.tank_fields = 0;
        state.relay_fields = state.dimmer_fields = 0;
        state.bat_voltage = state.bat_current = state.starter_voltage = 0;
        state.bat_soc = 0;
        state.solar_power = state.solar_current = state.solar_voltage = 0;
        state.indoor_temp = state.outdoor_temp = 0;
        state.heating_on = state.fan_on = false;
        for (int i = 0; i < 4; ++i) { state.temp_sensors[i] = 0; state.tank_level[i] = 0; }
        for (int i = 0; i < 8; ++i) {
            state.dimmer_hold_until[i] = state.relay_hold_until[i] = 0;
            state.switch_state[i] = false; state.dimmer_val[i] = 0;
        }
        demo_relays_seeded = demo_dimmers_seeded = false;
        poll_step = 0;
        last_http_poll = 0;
        state.vanpi_connected = false;
        state.data_is_demo = state.debug_mode;
    }
    // =========================================================================
    // Simulation / Debug Mode: Use realistic dummy JSON payloads with strings
    // =========================================================================
    if (state.debug_mode) {

        if (millis() - last_http_poll > 500) {
            switch (poll_step) {
                case 0: {
                    // String-encoded float/int values
                    String dummy = "{\"VoltB\":\"13.4\",\"Ampere\":\"-2.1\",\"battsoc\":\"88\",\"starter_voltage\":\"12.7\"}";
                    parse_batt_json(dummy);
                    break;
                }
                case 1: {
                    String dummy = "{\"Relay1\":{\"state\":true,\"name\":\"Licht Bank\"},\"Relay2\":{\"state\":false,\"name\":\"Kuehlschrank\"},\"Relay3\":{\"state\":true,\"name\":\"Wasserpumpe\"},\"Relay4\":{\"state\":false,\"name\":\"Abwasserventil\"},\"Relay5\":{\"state\":false,\"name\":\"Relais 5\"},\"Relay6\":{\"state\":false,\"name\":\"Relais 6\"},\"Relay7\":{\"state\":false,\"name\":\"Relais 7\"},\"Relay8\":{\"state\":false,\"name\":\"Relais 8\"}}";
                    if (!demo_relays_seeded) { parse_relay_json(dummy); demo_relays_seeded = true; }
                    break;
                }
                case 2: {
                    String dummy = "{\"dimmer1\":{\"state\":\"75\",\"name\":\"Deckenlampe\"},\"dimmer2\":{\"state\":\"40\",\"name\":\"Kueche\"},\"dimmer3\":{\"state\":\"0\",\"name\":\"Leselicht\"},\"dimmer4\":{\"state\":\"0\",\"name\":\"Dimmer 4\"},\"dimmer5\":{\"state\":\"0\",\"name\":\"Dimmer 5\"},\"dimmer6\":{\"state\":\"0\",\"name\":\"Dimmer 6\"},\"dimmer7\":{\"state\":\"0\",\"name\":\"Dimmer 7\"},\"dimmer8\":{\"state\":\"0\",\"name\":\"Dimmer 8\"}}";
                    if (!demo_dimmers_seeded) { parse_dimmer_json(dummy); demo_dimmers_seeded = true; }
                    break;
                }
                case 3: {
                    String dummy = "{\"level1\":{\"state\":\"68\",\"name\":\"Frischwasser\"},\"level2\":{\"state\":\"32\",\"name\":\"Grauwasser\"},\"level3\":{\"state\":\"50\",\"name\":\"Tank 3\"},\"level4\":{\"state\":\"0\",\"name\":\"Tank 4\"}}";
                    parse_level_json(dummy);
                    break;
                }
                case 4: {
                    String dummy = "{\"temp1\":{\"state\":\"22.4\",\"name\":\"Innen\"},\"temp2\":{\"state\":\"14.6\",\"name\":\"Aussen\"},\"temp3\":{\"state\":\"52.0\",\"name\":\"Feuchte\"},\"temp4\":{\"state\":\"7.8\",\"name\":\"Kuehlbox\"}}";
                    parse_temp_json(dummy);
                    break;
                }
                case 5: {
                    String dummy = "{\"autoterm1\":{\"heatertoggle\":true,\"heatstatus\":\"heating\",\"heaterror\":\"no\",\"targettemp_vanpi\":\"21.5\",\"powerlevel\":\"5\",\"fanspeed\":0,\"mode\":\"temp mode\"}}";
                    parse_heater_json(dummy);
                    break;
                }
                case 6: {
                    String dummy = "{\"mppt_pv_watts\":\"148.5\",\"mppt_pv_amps\":\"8.3\",\"mppt_pv_volts\":\"17.9\"}";
                    parse_mppt_json(dummy);
                    break;
                }
                case 7: {
                    String dummy = "{\"x_angle\":\"-0.8\",\"y_angle\":\"1.4\"}";
                    parse_position_json(dummy);
                    break;
                }
            }

            // Print UART diagnostic line on every full cycle for validation
            if (poll_step == 7) {
                Serial.printf("[CAMPER_UART] Batt: %.2fV, %.2fA, %d%% | Solar: %.1fW | TempIn: %.1fC, Out: %.1fC | Level: Pitch=%.1f°, Roll=%.1f°\n",
                              state.bat_voltage, state.bat_current, state.bat_soc,
                              state.solar_power,
                              state.indoor_temp, state.outdoor_temp,
                              state.pitch_angle, state.roll_angle);
            }

            poll_step++;
            if (poll_step > 7) poll_step = 0;
            last_http_poll = millis();
        }
        return;
    }

    // =========================================================================
    // Normal Live Mode: Poll VanPi via HTTP
    // =========================================================================
    if (WiFi.status() != WL_CONNECTED) {
        state.wifi_connected = false;
        state.vanpi_connected = false;
        return;
    }
    state.wifi_connected = true;
    state.wifi_rssi = WiFi.RSSI();

    if (millis() < vanpi_fail_backoff_until) {
        state.vanpi_connected = false;
        return;
    }

    if (millis() - last_http_poll > 500) {
        switch (poll_step) {
            case 0: fetch_endpoint("/batt",    parse_batt_json);    break;
            case 1: fetch_endpoint("/relay",   parse_relay_json);   break;
            case 2: fetch_endpoint("/dimmer",  parse_dimmer_json);  break;
            case 3: fetch_endpoint("/level",   parse_level_json);   break;
            case 4: fetch_endpoint("/temp",    parse_temp_json);    break;
            case 5:
                if (state.tab_enabled[TAB_CLIMATE]) fetch_endpoint("/heater", parse_heater_json);
                break;
            case 6: fetch_endpoint("/mppt/",   parse_mppt_json);    break;
            case 7:
                if (state.tab_enabled[TAB_LEVEL]) fetch_endpoint("/position_sensor/?request=true", parse_position_json);
                break;
        }

        if (poll_step == 7) {
            Serial.printf("[CAMPER_UART] Batt: %.2fV, %.2fA, %d%% | Solar: %.1fW | TempIn: %.1fC | Level: Pitch=%.1f°, Roll=%.1f°\n",
                          state.bat_voltage, state.bat_current, state.bat_soc,
                          state.solar_power,
                          state.indoor_temp,
                          state.pitch_angle, state.roll_angle);
            Serial.printf("[CAMPER_UART] Heater: on=%d status='%s' err='%s' mode=%s target=%.1f lvl=%d | Dim: %d %d %d %d\n",
                          state.heating_on, state.heater_status.c_str(), state.heater_error.c_str(),
                          state.heater_vent_mode ? "vent" : (state.heater_power_mode ? "power" : "temp"),
                          state.target_temp, state.heater_power_level,
                          state.dimmer_val[0], state.dimmer_val[1], state.dimmer_val[2], state.dimmer_val[3]);
        }

        poll_step++;
        if (poll_step > 7) poll_step = 0;
        last_http_poll = millis();
    }
}

// --- Publishing (Non-blocking: pushes to FreeRTOS queue, processed on Core 0) ---

static void http_put(String endpoint) {
    if (state.debug_mode) return;
    if (WiFi.status() != WL_CONNECTED) return;
    if (state.vanpi_ip.length() < 7) return;

    HTTPClient http;
    http.begin(get_url() + endpoint);
    http.setConnectTimeout(400);
    http.setTimeout(1500);
    int code = http.PUT("");
    if (code != 200) {
        Serial.printf("[HTTP] PUT %s -> %d\n", endpoint.c_str(), code);
    }
    http.end();
}

void http_publish_switch(int index, bool on) {
    if (index < 0 || index >= 8) return;
    state.switch_state[index] = on;
    state.relay_hold_until[index] = millis() + 2000;
    char path[64];
    snprintf(path, sizeof(path), "/relay/%d/%s", index + 1, on ? "true" : "false");
    queue_cmd(path);
}

void http_publish_dimmer(int index, int val) {
    if (index < 0 || index >= 8) return;
    val = constrain(val, 0, 100);
    state.dimmer_val[index] = val;
    char path[64];
    snprintf(path, sizeof(path), "/dimmer/%d/%d", index + 1, val);
    queue_cmd(path);
}

void http_publish_heater_cmd(String mode, int value) {
    char path[64];
    snprintf(path, sizeof(path), "/autoterm/%s/%d", mode.c_str(), value);
    queue_cmd(path);
}

void http_calibrate_position() {
    Serial.println("[HTTP] Calibrate position sensor requested");
    queue_cmd("/position_sensor/?request=calibrate");
}

// Dedicated FreeRTOS background task running on Core 0
static void http_background_task(void *pvParameters) {
    for (;;) {
        // 1. Process any pending outgoing commands with immediate priority
        HttpCmd cmd;
        while (http_cmd_queue && xQueueReceive(http_cmd_queue, &cmd, 0) == pdTRUE) {
            http_put(String(cmd.path));
        }

        // 2. Poll endpoints
        http_loop();

        // Short sleep so other Core 0 tasks (WiFi driver, idle) get time
        vTaskDelay(pdMS_TO_TICKS(15));
    }
}

bool http_start_task() {
    static TaskHandle_t worker = NULL;
    if (worker) return true;
    http_init();
    if (!http_cmd_queue) {
        Serial.println("[HTTP] ERROR: command queue allocation failed; data worker not started");
        return false;
    }
    // Pin to Core 0 (Core 1 is 100% dedicated to UI / LVGL!)
    BaseType_t result = xTaskCreatePinnedToCore(
        http_background_task,
        "http_task",
        10240,       // 10 KB stack
        NULL,
        1,           // Priority 1
        &worker,
        0            // Core 0
    );
    if (result != pdPASS) {
        worker = NULL;
        Serial.println("[HTTP] ERROR: worker allocation failed; Demo/Live data unavailable");
        return false;
    }
    Serial.println("[HTTP] Background worker pinned to Core 0");
    return true;
}
