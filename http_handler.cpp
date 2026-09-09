#include "http_handler.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "system_state.h"
#include "HWCDC.h"

extern HWCDC USBSerial;
#define Serial USBSerial

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
    // Nothing special needed
}

void http_fetch_names() {
    // Names come embedded in /relay and /dimmer responses
}

// --- Parsers ---
// GET /batt -> { "VoltB": float|str, "Ampere": float|str, "battsoc": int|str }
static void parse_batt_json(String payload) {
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, payload)) return;

    if (doc.containsKey("VoltB"))   state.bat_voltage = jsonFloat(doc["VoltB"]);
    if (doc.containsKey("Ampere"))  state.bat_current = jsonFloat(doc["Ampere"]);
    if (doc.containsKey("battsoc")) state.bat_soc     = jsonInt(doc["battsoc"]);
}

// GET /mppt/ -> { "mppt_pv_watts": float, "mppt_pv_amps": float, "mppt_pv_volts": float }
static void parse_mppt_json(String payload) {
    DynamicJsonDocument doc(2048);
    if (deserializeJson(doc, payload)) return;

    if (doc.containsKey("mppt_pv_watts")) state.solar_power   = jsonFloat(doc["mppt_pv_watts"]);
    if (doc.containsKey("mppt_pv_amps"))  state.solar_current  = jsonFloat(doc["mppt_pv_amps"]);
}

// GET /relay -> { "Relay1": { "state": bool, "name": str, ... }, "Relay2": ... }
static void parse_relay_json(String payload) {
    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, payload)) return;

    for (int i = 0; i < 8; i++) {
        String key = "Relay" + String(i + 1);
        if (!doc.containsKey(key)) continue;
        JsonObject r = doc[key];
        if (r.containsKey("state")) state.switch_state[i] = r["state"].as<bool>();
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
        if (d.containsKey("state")) state.dimmer_val[i] = jsonInt(d["state"]);
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
        if (lvl.containsKey("state")) state.tank_level[i] = jsonInt(lvl["state"]);
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
        if (t.containsKey("state")) {
            float val = jsonFloat(t["state"]);
            if (i == 0) state.indoor_temp = val;
            else if (i == 1) state.outdoor_temp = val;
            else if (i == 2) state.indoor_humidity = val;
        }
    }
}

// GET /heater -> { "autoterm1": { "heatertoggle": bool, "heatstatus": str,
//   "targettemp_vanpi": float, "mode": str, "powerlevel": int, "fanspeed": int }, ... }
static void parse_heater_json(String payload) {
    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, payload)) return;

    if (doc.containsKey("autoterm1")) {
        JsonObject at = doc["autoterm1"];

        // Mode
        if (at.containsKey("mode")) {
            String m = at["mode"].as<String>();
            if (m == "vent") {
                state.heater_vent_mode = true;
                state.heater_power_mode = false;
            } else if (m == "power") {
                state.heater_vent_mode = false;
                state.heater_power_mode = true;
            } else if (m == "temp") {
                state.heater_vent_mode = false;
                state.heater_power_mode = false;
            }
        }

        // Running state
        if (at.containsKey("heatstatus")) {
            String hs = at["heatstatus"].as<String>();
            state.heating_on = (hs == "run" || hs == "start" || hs == "ventilation");
        } else if (at.containsKey("heatertoggle")) {
            state.heating_on = at["heatertoggle"].as<bool>();
        }

        if (at.containsKey("targettemp_vanpi")) state.target_temp        = jsonFloat(at["targettemp_vanpi"]);
        if (at.containsKey("powerlevel"))       state.heater_power_level = jsonInt(at["powerlevel"]);
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

static void fetch_endpoint(const char* endpoint, void (*parser)(String)) {
    if (WiFi.status() != WL_CONNECTED) return;
    if (state.vanpi_ip.length() < 7) return;

    HTTPClient http;
    http.begin(get_url() + endpoint);
    http.setConnectTimeout(800);
    http.setTimeout(3000);
    int code = http.GET();
    if (code == 200) {
        String payload = http.getString();
        parser(payload);
    } else {
        Serial.printf("[HTTP] GET %s -> %d\n", endpoint, code);
    }
    http.end();
}

void http_loop() {
    // =========================================================================
    // Simulation / Debug Mode: Use realistic dummy JSON payloads with strings
    // =========================================================================
    if (state.debug_mode) {
        state.wifi_connected = true;
        state.wifi_rssi = -55;

        if (millis() - last_http_poll > 500) {
            switch (poll_step) {
                case 0: {
                    // String-encoded float/int values
                    String dummy = "{\"VoltB\":\"13.4\",\"Ampere\":\"-2.1\",\"battsoc\":\"88\"}";
                    parse_batt_json(dummy);
                    break;
                }
                case 1: {
                    String dummy = "{\"Relay1\":{\"state\":true,\"name\":\"Licht Bank\"},\"Relay2\":{\"state\":false,\"name\":\"Kuehlschrank\"},\"Relay3\":{\"state\":true,\"name\":\"Wasserpumpe\"},\"Relay4\":{\"state\":false,\"name\":\"Abwasserventil\"}}";
                    parse_relay_json(dummy);
                    break;
                }
                case 2: {
                    String dummy = "{\"dimmer1\":{\"state\":\"75\",\"name\":\"Deckenlampe\"},\"dimmer2\":{\"state\":\"40\",\"name\":\"Kueche\"},\"dimmer3\":{\"state\":\"0\",\"name\":\"Leselicht\"}}";
                    parse_dimmer_json(dummy);
                    break;
                }
                case 3: {
                    String dummy = "{\"level1\":{\"state\":\"68\",\"name\":\"Frischwasser\"},\"level2\":{\"state\":\"32\",\"name\":\"Grauwasser\"}}";
                    parse_level_json(dummy);
                    break;
                }
                case 4: {
                    String dummy = "{\"temp1\":{\"state\":\"22.4\",\"name\":\"Innen\"},\"temp2\":{\"state\":\"14.6\",\"name\":\"Aussen\"},\"temp3\":{\"state\":\"52.0\",\"name\":\"Feuchte\"}}";
                    parse_temp_json(dummy);
                    break;
                }
                case 5: {
                    String dummy = "{\"autoterm1\":{\"heatertoggle\":true,\"heatstatus\":\"run\",\"targettemp_vanpi\":\"21.5\",\"powerlevel\":\"5\",\"mode\":\"temp\"}}";
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
        return;
    }
    state.wifi_connected = true;
    state.wifi_rssi = WiFi.RSSI();

    if (millis() - last_http_poll > 500) {
        switch (poll_step) {
            case 0: fetch_endpoint("/batt",    parse_batt_json);    break;
            case 1: fetch_endpoint("/relay",   parse_relay_json);   break;
            case 2: fetch_endpoint("/dimmer",  parse_dimmer_json);  break;
            case 3: fetch_endpoint("/level",   parse_level_json);   break;
            case 4: fetch_endpoint("/temp",    parse_temp_json);    break;
            case 5: fetch_endpoint("/heater",  parse_heater_json);  break;
            case 6: fetch_endpoint("/mppt/",   parse_mppt_json);    break;
            case 7: fetch_endpoint("/position_sensor/?request=true", parse_position_json); break;
        }

        if (poll_step == 7) {
            Serial.printf("[CAMPER_UART] Batt: %.2fV, %.2fA, %d%% | Solar: %.1fW | TempIn: %.1fC | Level: Pitch=%.1f°, Roll=%.1f°\n",
                          state.bat_voltage, state.bat_current, state.bat_soc,
                          state.solar_power,
                          state.indoor_temp,
                          state.pitch_angle, state.roll_angle);
        }

        poll_step++;
        if (poll_step > 7) poll_step = 0;
        last_http_poll = millis();
    }
}

// --- Publishing ---

static void http_put(String endpoint) {
    if (WiFi.status() != WL_CONNECTED) return;
    if (state.vanpi_ip.length() < 7) return;

    HTTPClient http;
    http.begin(get_url() + endpoint);
    http.setConnectTimeout(800);
    http.setTimeout(3000);
    int code = http.PUT("");
    if (code != 200) {
        Serial.printf("[HTTP] PUT %s -> %d\n", endpoint.c_str(), code);
    }
    http.end();
}

void http_publish_switch(int index, bool on) {
    String val = on ? "true" : "false";
    http_put("/relay/" + String(index + 1) + "/" + val);
    state.switch_state[index] = on;
}

void http_publish_dimmer(int index, int val) {
    http_put("/dimmer/" + String(index + 1) + "/" + String(val));
    state.dimmer_val[index] = val;
}

void http_publish_heater_cmd(String mode, int value) {
    http_put("/autoterm/" + mode + "/" + String(value));
}

void http_calibrate_position() {
    Serial.println("[HTTP] Calibrate position sensor requested");
    fetch_endpoint("/position_sensor/?request=calibrate", [](String) {});
}
