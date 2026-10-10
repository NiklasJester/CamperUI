#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <Arduino.h>
#include <Preferences.h>

enum TabId : uint8_t {
    TAB_HOME = 0,
    TAB_DIMMERS = 1,
    TAB_POWER = 2,
    TAB_WATER = 3,
    TAB_CLIMATE = 4,
    TAB_MAXXFAN = 5,
    TAB_SWITCHES = 6,
    TAB_LEVEL = 7,
    TAB_SETTINGS = 8,
    TAB_COUNT = 9
};

// Global State Structure to hold all sensor data and system settings
static constexpr int TEMP_SOURCE_COUNT = 13; // temp1..4, ruuvitag0..8 (stable persisted indices)
struct SystemState {
    // --- Settings (Persisted) ---
    String wifi_ssid;
    String wifi_pass;
    String vanpi_ip;
    
    // Menu / Navigation Customization (Persisted)
    uint8_t tab_order[TAB_COUNT];
    bool tab_enabled[TAB_COUNT];
    
    int display_brightness; // 10-100
    int display_timeout;
    int display_rotation; // 0, 1, 2, 3
    bool dark_mode;
    bool battery_icon_mode; // true = icon, false = percent
    bool buzzer_enabled;    // Touch-tone / haptic buzzer feedback
    
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
    int home_temp_source[3]; // -1 hidden, otherwise /temp sensor 0..12 (4..12 = Ruuvi 0..8)
    int home_favorite[2]; // 0 hidden, 1..8 relays, 9..16 dimmers
    bool home_show_battery;
    bool home_show_starter;
    bool home_show_solar;
    bool home_show_water;
    int home_water_source; // /level index 0..3

    // Tank Capacities (Liters)
    int tank_max[4]; 
    bool tank_enabled[4];
    String tank_names[4];
    
    // Custom Names
    String switch_names[8];
    String dimmer_names[8];

    // Time & Zone Settings (Persisted)
    bool time_auto_ntp;     // true: Internet (NTP), false: Manual (Offline)
    int time_zone_idx;      // Index in TIMEZONES
    int manual_hour;        // 0..23
    int manual_min;         // 0..59

    // WiFi Relays (Persisted)
    bool show_wrelay;
    bool wrelay_visible[8];
    String wrelay_names[8];

    // Firmware Update / Web OTA (Volatile)
    bool web_ota_active;
    int web_ota_progress;
    String web_ota_msg;

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
    float solar_voltage;
    float starter_voltage; // optional /batt extension: starter_voltage
    uint8_t battery_fields; // VoltB=1, Ampere=2, battsoc=4, optional starter_voltage=8
    uint8_t solar_fields; // watts=1, amps=2, volts=4
    uint16_t temp_fields;
    uint8_t tank_fields;
    uint8_t relay_fields;
    uint8_t dimmer_fields;
    bool data_is_demo; // source of the received fields, not the requested mode

    // Water
    int tank_level[4]; // 0-100%

    // Climate
    float indoor_temp;
    float outdoor_temp;
    float indoor_humidity;
    float target_temp;
    float temp_sensors[TEMP_SOURCE_COUNT];
    bool temp_is_humidity[TEMP_SOURCE_COUNT];
    String temp_sensor_names[TEMP_SOURCE_COUNT];
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
    uint32_t relay_hold_until[8];
    uint32_t heater_hold_until;

    // Visibility and roles
    bool switch_visible[8];
    bool dimmer_visible[8];
    int pump_relay; // 0-7, or -1 for none
    int drain_relay; // 0-7, or -1 for none

    // WiFi Relays (Volatile State)
    bool wrelay_state[8];
    uint32_t wrelay_hold_until[8];
    uint8_t wrelay_fields;
};

struct TimezoneInfo {
    const char *name;
    const char *tz_str;
};

#define TIMEZONE_COUNT 51
#define TIMEZONE_DEFAULT_INDEX 20
extern const TimezoneInfo TIMEZONES[TIMEZONE_COUNT];

void time_apply_configuration();
void time_set_manual(int hour, int min);

extern SystemState state;
extern Preferences prefs;
extern SemaphoreHandle_t state_mutex;

inline void state_lock() {
    if (state_mutex) xSemaphoreTake(state_mutex, portMAX_DELAY);
}

inline void state_unlock() {
    if (state_mutex) xSemaphoreGive(state_mutex);
}

class StateLockGuard {
public:
    StateLockGuard() { state_lock(); }
    ~StateLockGuard() { state_unlock(); }
};

void state_init();
void state_save();

#endif // SYSTEM_STATE_H
