#include "system_state.h"
#include "display_sync.h"

// Optional local credentials; builds without this file remain supported.
#if __has_include("config/wifi_secrets.h")
#include "config/wifi_secrets.h"
#endif
#ifndef CAMPERUI_DEFAULT_WIFI_SSID
#define CAMPERUI_DEFAULT_WIFI_SSID ""
#endif
#ifndef CAMPERUI_DEFAULT_WIFI_PASSWORD
#define CAMPERUI_DEFAULT_WIFI_PASSWORD ""
#endif

SystemState state;
Preferences prefs;

void state_init() {
    prefs.begin("camperui", false);

    // Load persisted settings
    state.wifi_ssid = prefs.getString("wifi_ssid", "");
    state.wifi_pass = prefs.getString("wifi_pass", "");
    // Seed only an unconfigured device. Saved settings always take priority.
    if (state.wifi_ssid.length() == 0 && CAMPERUI_DEFAULT_WIFI_SSID[0] != '\0') {
        state.wifi_ssid = CAMPERUI_DEFAULT_WIFI_SSID;
        state.wifi_pass = CAMPERUI_DEFAULT_WIFI_PASSWORD;
        prefs.putString("wifi_ssid", state.wifi_ssid);
        prefs.putString("wifi_pass", state.wifi_pass);
    }
    state.vanpi_ip = prefs.getString("vanpi_ip", "100.80.161.23");

    
state.display_brightness = prefs.getInt("disp_bright", 100);
    if (state.display_brightness > 100) state.display_brightness = 100;
    
    state.display_timeout = prefs.getInt("disp_timeout", 30);
    state.display_rotation = prefs.getInt("disp_rot", 2);
    state.dark_mode = prefs.getBool("dark_mode", true);
    state.battery_icon_mode = prefs.getBool("bat_icn_md", true);
    state.debug_mode = prefs.getBool("dbg_sim", true);
    
    state.bat_capacity_ah = prefs.getFloat("bat_cap_ah", 100.0f);
    state.solar_max_w = prefs.getFloat("solar_max_w", 200.0f);

    state.warn_frost_temp = prefs.getFloat("w_frost", 3.0f);
    state.warn_bat_soc = prefs.getInt("w_bat", 20);
    state.warn_fresh_min = prefs.getInt("w_fresh", 15);
    state.warn_waste_max = prefs.getInt("w_waste", 85);

    state.outdoor_temp_sensor = prefs.getInt("out_temp_idx", 1); // Default to Temp 2 (Aussen)

    for (int i = 0; i < 4; i++) {
        char key_max[16], key_en[16], key_nm[16];
        sprintf(key_max, "tank%d_max", i);
        sprintf(key_en, "tank%d_en", i);
        sprintf(key_nm, "tank%d_nm", i);
        state.tank_max[i] = prefs.getInt(key_max, 100);
        state.tank_enabled[i] = prefs.getBool(key_en, (i < 2)); // Default first 2 tanks on
        
        String def_name = "";
        if (i==0) def_name = "Frisch";
        else if (i==1) def_name = "Abwasser 1";
        else if (i==2) def_name = "Abwasser 2";
        else def_name = "Schwarz";
        state.tank_names[i] = prefs.getString(key_nm, def_name);
    }

    for (int i = 0; i < 8; i++) {
        char key_sw[16], key_dim[16];
        char key_sw_vis[16], key_dim_vis[16];
        sprintf(key_sw, "sw%d_nm", i);
        sprintf(key_dim, "dim%d_nm", i);
        sprintf(key_sw_vis, "sw%d_vis", i);
        sprintf(key_dim_vis, "dim%d_vis", i);
        state.switch_names[i] = prefs.getString(key_sw, "CH " + String(i + 1));
        state.dimmer_names[i] = prefs.getString(key_dim, "PWM " + String(i + 1));
        state.switch_visible[i] = prefs.getBool(key_sw_vis, true);
        state.dimmer_visible[i] = prefs.getBool(key_dim_vis, true);
    }
    state.pump_relay = prefs.getInt("pump_relay", -1);
    state.drain_relay = prefs.getInt("drain_relay", -1);

    // Initialize defaults for volatile data
    state.wifi_connected = false;
    
    state.wifi_rssi = 0;
    state.vanpi_connected = false;

    state.bat_voltage = 12.0f;
    state.bat_current = 0.0f;
    state.bat_soc = 0;
    state.solar_power = 0.0f;
    state.solar_current = 0.0f;

    for (int i = 0; i < 4; i++) {
        state.tank_level[i] = 0;
        state.temp_sensors[i] = 0.0f;
        state.temp_sensor_names[i] = "Sensor " + String(i + 1);
    }

    state.indoor_temp = 0.0f;
    state.outdoor_temp = 0.0f;
    state.indoor_humidity = 0.0f;
    state.target_temp = 20.0f;
    state.heating_on = false;
    state.heater_vent_mode = false;
    state.heater_power_mode = false;
    state.heater_power_level = 5;
    state.heater_status = "";
    state.heater_error = "no";
    state.fan_on = false;

    state.pitch_angle = 0.0f;
    state.roll_angle = 0.0f;

    for (int i = 0; i < 8; i++) {
        state.switch_state[i] = false;
        state.dimmer_val[i] = 0;
        state.dimmer_hold_until[i] = 0;
    }
    state.heater_hold_until = 0;
}

void state_save() {
    prefs.putString("wifi_ssid", state.wifi_ssid);
    prefs.putString("wifi_pass", state.wifi_pass);
    prefs.putString("vanpi_ip", state.vanpi_ip);
    prefs.putInt("disp_bright", state.display_brightness);
    prefs.putInt("disp_timeout", state.display_timeout);
    prefs.putInt("disp_rot", state.display_rotation);
    prefs.putBool("dark_mode", state.dark_mode);
    prefs.putBool("bat_icn_md", state.battery_icon_mode);
    prefs.putBool("dbg_sim", state.debug_mode);
    
    prefs.putFloat("bat_cap_ah", state.bat_capacity_ah);
    prefs.putFloat("solar_max_w", state.solar_max_w);

    prefs.putFloat("w_frost", state.warn_frost_temp);
    prefs.putInt("w_bat", state.warn_bat_soc);
    prefs.putInt("w_fresh", state.warn_fresh_min);
    prefs.putInt("w_waste", state.warn_waste_max);

    prefs.putInt("out_temp_idx", state.outdoor_temp_sensor);

    for (int i = 0; i < 4; i++) {
        char key_max[16], key_en[16], key_nm[16];
        sprintf(key_max, "tank%d_max", i);
        sprintf(key_en, "tank%d_en", i);
        sprintf(key_nm, "tank%d_nm", i);
        prefs.putInt(key_max, state.tank_max[i]);
        prefs.putBool(key_en, state.tank_enabled[i]);
        prefs.putString(key_nm, state.tank_names[i]);
    }
    
    for (int i = 0; i < 8; i++) {
        char key_sw[16], key_dim[16];
        char key_sw_vis[16], key_dim_vis[16];
        sprintf(key_sw, "sw%d_nm", i);
        sprintf(key_dim, "dim%d_nm", i);
        sprintf(key_sw_vis, "sw%d_vis", i);
        sprintf(key_dim_vis, "dim%d_vis", i);
        prefs.putString(key_sw, state.switch_names[i]);
        prefs.putString(key_dim, state.dimmer_names[i]);
        prefs.putBool(key_sw_vis, state.switch_visible[i]);
        prefs.putBool(key_dim_vis, state.dimmer_visible[i]);
    }
    prefs.putInt("pump_relay", state.pump_relay);
    prefs.putInt("drain_relay", state.drain_relay);
    display_request_resync();
}

