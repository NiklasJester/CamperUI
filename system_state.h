#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <Arduino.h>
#include <Preferences.h>

// Global State Structure to hold all sensor data and system settings
struct SystemState {
    // --- Settings (Persisted) ---
    String wifi_ssid;
    String wifi_pass;
    String vanpi_ip;
    
    int display_brightness; // 10-100
    int display_timeout;
    int display_rotation; // 0, 1, 2, 3
    bool dark_mode;
    bool battery_icon_mode; // true = icon, false = percent
    
    // Limits & Capacities
    float bat_capacity_ah;
    float solar_max_w;

    // Smart Header Alarm Thresholds (Persisted)
    float warn_frost_temp;
    int warn_bat_soc;
    int warn_fresh_min;
    int warn_waste_max;

    // Sensor Selection (Persisted)
    int outdoor_temp_sensor; // 0..3 (index of temp sensor for outdoor temp, default 1)

    // Tank Capacities (Liters)
    int tank_max[4]; 
    bool tank_enabled[4];
    String tank_names[4];
    
    // Custom Names
    String switch_names[8];
    String dimmer_names[8];
    
    // --- Live Sensor Data (Volatile) ---
    // Status
    bool wifi_connected;
    int wifi_rssi;
    bool vanpi_connected;
    

    // Power
    float bat_voltage;
    float bat_current;
    int bat_soc; // 0-100%
    float solar_power; // W
    float solar_current;

    // Water
    int tank_level[4]; // 0-100%

    // Climate
    float indoor_temp;
    float outdoor_temp;
    float indoor_humidity;
    float target_temp;
    float temp_sensors[4];
    String temp_sensor_names[4];
    bool heating_on;
    bool heater_vent_mode;
    bool heater_power_mode;
    int heater_power_level;
    String heater_status;   // raw "heatstatus" from VanPi (e.g. "standby", "heating")
    String heater_error;    // raw "heaterror" from VanPi ("no" = no error)
    bool fan_on;

    // Inclinometer / Leveling
    float pitch_angle;
    float roll_angle;

    bool debug_mode;

    // Switches & Dimmers
    bool switch_state[8];
    int dimmer_val[8]; // 0-100%

    // Local-interaction hold-off (millis timestamps, not persisted):
    // while millis() < *_hold_until, polled values are ignored so the
    // remote state does not fight with a value the user is just changing.
    uint32_t dimmer_hold_until[8];
    uint32_t heater_hold_until;

    // Visibility and roles
    bool switch_visible[8];
    bool dimmer_visible[8];
    int pump_relay; // 0-7, or -1 for none
    int drain_relay; // 0-7, or -1 for none
};

extern SystemState state;
extern Preferences prefs;

void state_init();
void state_save();

#endif // SYSTEM_STATE_H
