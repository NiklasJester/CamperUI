#include "system_state.h"
#include "display_sync.h"
#include <time.h>
#include <esp_sntp.h>

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

const TimezoneInfo TIMEZONES[TIMEZONE_COUNT] = {
    // UTC-11
    {"UTC-11: Midway, Pago Pago (Samoa)", "SST11"},
    // UTC-10
    {"UTC-10: Hawaii, Honolulu (HST)", "HST10"},
    // UTC-9
    {"UTC-09: Alaska, Anchorage (AKST/AKDT)", "AKST9AKDT,M3.2.0,M11.1.0"},
    // UTC-8
    {"UTC-08: Los Angeles, Vancouver (PST/PDT)", "PST8PDT,M3.2.0,M11.1.0"},
    // UTC-7
    {"UTC-07: Denver, Salt Lake City, Calgary (MST/MDT)", "MST7MDT,M3.2.0,M11.1.0"},
    {"UTC-07: Phoenix, Arizona (MST, kein DST)", "MST7"},
    // UTC-6
    {"UTC-06: Chicago, Dallas, Winnipeg (CST/CDT)", "CST6CDT,M3.2.0,M11.1.0"},
    {"UTC-06: Mexiko-Stadt, Costa Rica (CST)", "CST6"},
    // UTC-5
    {"UTC-05: New York, Miami, Toronto (EST/EDT)", "EST5EDT,M3.2.0,M11.1.0"},
    {"UTC-05: Bogota, Lima, Quito (COT/PET)", "COT5"},
    // UTC-4
    {"UTC-04: Halifax (AST/ADT)", "AST4ADT,M3.2.0,M11.1.0"},
    {"UTC-04: Santiago de Chile (CLT/CLST)", "CLT4CLST,M9.1.0/0,M4.1.0/0"},
    {"UTC-04: La Paz, Manaus, Caracas (BOT/VET)", "VET4"},
    // UTC-3:30
    {"UTC-03:30: Neufundland (St. John's, NST/NDT)", "NST3:30NDT,M3.2.0,M11.1.0"},
    // UTC-3
    {"UTC-03: Buenos Aires, Sao Paulo, Montevideo", "<-03>3"},
    // UTC-2
    {"UTC-02: Fernando de Noronha, Suedgeorgien", "<-02>2"},
    // UTC-1
    {"UTC-01: Azoren (AZOT/AZOST)", "AZOT1AZOST,M3.5.0/0,M10.5.0/1"},
    {"UTC-01: Kap Verde (CVT)", "CVT1"},
    // UTC+0
    {"UTC+00: London, Dublin, Lissabon (WET/GMT/BST)", "GMT0BST,M3.5.0/1,M10.5.0"},
    {"UTC+00: UTC (Koordinierte Weltzeit)", "UTC0"},
    // UTC+1
    {"UTC+01: Berlin, Wien, Zuerich, Rom, Paris (MEZ/MESZ)", "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"UTC+01: Algier, Tunis, Lagos (WAT)", "WAT-1"},
    // UTC+2
    {"UTC+02: Athen, Helsinki, Kiew, Bukarest (OEZ/EEST)", "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"UTC+02: Kairo (EET/EEST)", "EET-2EEST,M4.5.5/0,M10.5.5/0"},
    {"UTC+02: Johannesburg, Kapstadt (SAST)", "SAST-2"},
    {"UTC+02: Jerusalem, Tel Aviv (IST/IDT)", "IST-2IDT,M3.4.4/26,M10.5.0"},
    // UTC+3
    {"UTC+03: Moskau, St. Petersburg (MSK)", "MSK-3"},
    {"UTC+03: Istanbul (TRT)", "TRT-3"},
    {"UTC+03: Riad, Doha, Kuwait, Nairobi (AST/EAT)", "AST-3"},
    // UTC+3:30
    {"UTC+03:30: Teheran (IRST)", "<+0330>-3:30"},
    // UTC+4
    {"UTC+04: Dubai, Abu Dhabi, Maskat (GST)", "GST-4"},
    {"UTC+04: Baku, Tiflis, Jerewan (AZT/GET)", "<+04>-4"},
    // UTC+4:30
    {"UTC+04:30: Kabul (AFT)", "<+0430>-4:30"},
    // UTC+5
    {"UTC+05: Taschkent, Karatschi, Male (PKT/UZT)", "PKT-5"},
    // UTC+5:30
    {"UTC+05:30: Neu-Delhi, Mumbai, Kalkutta (IST)", "IST-5:30"},
    // UTC+5:45
    {"UTC+05:45: Kathmandu (NPT)", "<+0545>-5:45"},
    // UTC+6
    {"UTC+06: Dhaka, Almaty, Astana (BST/ALMT)", "<+06>-6"},
    // UTC+6:30
    {"UTC+06:30: Rangun / Yangon (MMT)", "<+0630>-6:30"},
    // UTC+7
    {"UTC+07: Bangkok, Jakarta, Hanoi (ICT/WIB)", "ICT-7"},
    // UTC+8
    {"UTC+08: Peking, Singapur, Hongkong, Taipeh (CST/HKT)", "CST-8"},
    {"UTC+08: Perth (AWST)", "AWST-8"},
    // UTC+9
    {"UTC+09: Tokio, Seoul (JST/KST)", "JST-9"},
    // UTC+9:30
    {"UTC+09:30: Adelaide (ACST/ACDT)", "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
    {"UTC+09:30: Darwin (ACST, kein DST)", "ACST-9:30"},
    // UTC+10
    {"UTC+10: Sydney, Melbourne, Canberra (AEST/AEDT)", "AEST-10AEDT,M10.1.0,M4.1.0/3"},
    {"UTC+10: Brisbane (Queensland, AEST, kein DST)", "AEST-10"},
    // UTC+11
    {"UTC+11: Noumea, Salomonen, Magadan", "<+11>-11"},
    // UTC+12
    {"UTC+12: Auckland, Wellington (NZST/NZDT)", "NZST-12NZDT,M9.5.0,M4.1.0/3"},
    {"UTC+12: Fidschi (FJT)", "<+12>-12"},
    // UTC+13
    {"UTC+13: Samoa (Apia), Tonga (Nuku'alofa)", "<+13>-13"},
    // UTC+14
    {"UTC+14: Kiritimati (Line Islands)", "<+14>-14"}
};

void time_apply_configuration() {
    int idx = constrain(state.time_zone_idx, 0, TIMEZONE_COUNT - 1);
    if (state.time_auto_ntp) {
        configTzTime(TIMEZONES[idx].tz_str, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
    } else {
        setenv("TZ", TIMEZONES[idx].tz_str, 1);
        tzset();
    }
}

void time_set_manual(int hour, int min) {
    state.manual_hour = constrain(hour, 0, 23);
    state.manual_min = constrain(min, 0, 59);

    int idx = constrain(state.time_zone_idx, 0, TIMEZONE_COUNT - 1);
    setenv("TZ", TIMEZONES[idx].tz_str, 1);
    tzset();

    time_t now = time(nullptr);
    struct tm tm_now;
    if (now > 1700000000) {
        localtime_r(&now, &tm_now);
    } else {
        tm_now.tm_year = 2026 - 1900;
        tm_now.tm_mon = 9; // Oct
        tm_now.tm_mday = 8;
        tm_now.tm_isdst = -1;
    }
    tm_now.tm_hour = state.manual_hour;
    tm_now.tm_min = state.manual_min;
    tm_now.tm_sec = 0;

    time_t new_time = mktime(&tm_now);
    struct timeval tv = { .tv_sec = new_time, .tv_usec = 0 };
    settimeofday(&tv, NULL);
}

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
    const int home_defaults[3] = {0, 1, 3};
    for (int i = 0; i < 3; ++i) {
        char key[16]; snprintf(key, sizeof(key), "home_temp%d", i);
        state.home_temp_source[i] = constrain(prefs.getInt(key, home_defaults[i]), -1, 3);
    }
    for (int i = 0; i < 2; ++i) {
        char key[16]; snprintf(key, sizeof(key), "home_fav%d", i);
        state.home_favorite[i] = constrain(prefs.getInt(key, i == 0 ? 1 : 3), 0, 16);
    }
    state.home_show_battery = prefs.getBool("home_bat", true);
    state.home_show_starter = prefs.getBool("home_starter", true);
    state.home_show_solar = prefs.getBool("home_solar", true);
    state.home_show_water = prefs.getBool("home_water", true);
    state.home_water_source = constrain(prefs.getInt("home_tank", 0), 0, 3);

    // Tab Order & Visibility
    size_t r_order = prefs.getBytes("tab_order", state.tab_order, sizeof(state.tab_order));
    if (r_order != sizeof(state.tab_order)) {
        for (uint8_t i = 0; i < TAB_COUNT; ++i) state.tab_order[i] = i;
    } else {
        bool seen[TAB_COUNT] = {false};
        bool valid = true;
        for (uint8_t i = 0; i < TAB_COUNT; ++i) {
            if (state.tab_order[i] >= TAB_COUNT || seen[state.tab_order[i]]) {
                valid = false;
                break;
            }
            seen[state.tab_order[i]] = true;
        }
        if (!valid) {
            for (uint8_t i = 0; i < TAB_COUNT; ++i) state.tab_order[i] = i;
        }
    }

    size_t r_en = prefs.getBytes("tab_en", state.tab_enabled, sizeof(state.tab_enabled));
    if (r_en != sizeof(state.tab_enabled)) {
        for (uint8_t i = 0; i < TAB_COUNT; ++i) state.tab_enabled[i] = true;
    }
    // Settings is always enabled so the user can never get locked out
    state.tab_enabled[TAB_SETTINGS] = true;

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
    state.solar_voltage = 0.0f;
    state.starter_voltage = 0.0f;
    state.battery_fields = state.solar_fields = state.temp_fields = state.tank_fields = 0;
    state.relay_fields = state.dimmer_fields = 0;
    state.data_is_demo = state.debug_mode;

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
        state.relay_hold_until[i] = 0;
    }
    state.heater_hold_until = 0;

    state.time_auto_ntp = prefs.getBool("time_auto", true);
    int saved_tz = prefs.getInt("time_tz_v2", -1);
    if (saved_tz >= 0 && saved_tz < TIMEZONE_COUNT) {
        state.time_zone_idx = saved_tz;
    } else {
        int old_tz = prefs.getInt("time_tz", 0);
        switch (old_tz) {
            case 0: state.time_zone_idx = 20; break; // Berlin
            case 1: state.time_zone_idx = 18; break; // London
            case 2: state.time_zone_idx = 22; break; // Athen
            case 3: state.time_zone_idx = 27; break; // Istanbul
            case 4: state.time_zone_idx = 19; break; // UTC
            case 5: state.time_zone_idx = 8;  break; // New York
            case 6: state.time_zone_idx = 3;  break; // Los Angeles
            default: state.time_zone_idx = TIMEZONE_DEFAULT_INDEX; break;
        }
        prefs.putInt("time_tz_v2", state.time_zone_idx);
    }
    state.manual_hour = prefs.getInt("man_hour", 12);
    state.manual_min = prefs.getInt("man_min", 0);

    state.show_wrelay = prefs.getBool("show_wrelay", false);
    for (int i = 0; i < 8; i++) {
        char key_w_nm[16], key_w_vis[16];
        sprintf(key_w_nm, "wr%d_nm", i);
        sprintf(key_w_vis, "wr%d_vis", i);
        state.wrelay_names[i] = prefs.getString(key_w_nm, "W-Relais " + String(i + 1));
        state.wrelay_visible[i] = prefs.getBool(key_w_vis, true);
        state.wrelay_state[i] = false;
        state.wrelay_hold_until[i] = 0;
    }
    state.wrelay_fields = 0;

    time_apply_configuration();
}

void state_save() {
    for (int i = 0; i < 3; ++i) {
        char key[16]; snprintf(key, sizeof(key), "home_temp%d", i);
        prefs.putInt(key, state.home_temp_source[i]);
    }
    for (int i = 0; i < 2; ++i) {
        char key[16]; snprintf(key, sizeof(key), "home_fav%d", i);
        prefs.putInt(key, state.home_favorite[i]);
    }
    prefs.putBool("home_bat", state.home_show_battery);
    prefs.putBool("home_starter", state.home_show_starter);
    prefs.putBool("home_solar", state.home_show_solar);
    prefs.putBool("home_water", state.home_show_water);
    prefs.putInt("home_tank", state.home_water_source);
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
    prefs.putBytes("tab_order", state.tab_order, sizeof(state.tab_order));
    prefs.putBytes("tab_en", state.tab_enabled, sizeof(state.tab_enabled));

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

    prefs.putBool("time_auto", state.time_auto_ntp);
    prefs.putInt("time_tz_v2", state.time_zone_idx);
    prefs.putInt("time_tz", state.time_zone_idx);
    prefs.putInt("man_hour", state.manual_hour);
    prefs.putInt("man_min", state.manual_min);

    prefs.putBool("show_wrelay", state.show_wrelay);
    for (int i = 0; i < 8; i++) {
        char key_w_nm[16], key_w_vis[16];
        sprintf(key_w_nm, "wr%d_nm", i);
        sprintf(key_w_vis, "wr%d_vis", i);
        prefs.putString(key_w_nm, state.wrelay_names[i]);
        prefs.putBool(key_w_vis, state.wrelay_visible[i]);
    }

    display_request_resync();
}

