#include "temp_history.h"
#include "system_state.h"
#include <math.h>

static int16_t (*history_samples)[TEMP_HISTORY_TRACKS] = nullptr;
static int head_idx = 0;
static bool history_is_demo = false;
static uint32_t last_sample_millis = 0;
static bool has_sample = false;
// The full VanPi poll cycle takes over 30 seconds in the screensaver.
static constexpr uint32_t temperature_freshness_ms = 45000;
static uint32_t history_version = 1;

uint32_t temp_history_get_version(void) {
    return history_version;
}

static void temp_history_seed_demo(void) {
    if (!history_samples) return;
    for (int m = 0; m < TEMP_HISTORY_MAX_SAMPLES; ++m) {
        // Aussen: 24-hour curve with realistic diurnal variation
        float t_out = 14.5f + 5.5f * sinf((float)m * (2.0f * (float)M_PI / 1440.0f)) + 0.2f * sinf((float)m * 0.15f);
        // Innen: stable around 21-23 °C
        float t_in = 21.8f + 1.2f * sinf(((float)m + 240.0f) * (2.0f * (float)M_PI / 1440.0f));
        // Track 2 (z.B. Kühlbox): cyclic cooling pattern 4.5 - 7.5 °C
        float t_c1 = 6.0f + 1.6f * sinf((float)m * (2.0f * (float)M_PI / 60.0f));
        // Track 3 (z.B. Ruuvi Schlafbereich): 18.5 - 20.5 °C
        float t_c2 = 19.5f + 0.9f * cosf((float)m * (2.0f * (float)M_PI / 720.0f));

        history_samples[m][0] = (int16_t)roundf(t_out * 10.0f);
        history_samples[m][1] = (int16_t)roundf(t_in * 10.0f);
        history_samples[m][2] = (int16_t)roundf(t_c1 * 10.0f);
        history_samples[m][3] = (int16_t)roundf(t_c2 * 10.0f);
    }
    head_idx = 0;
    last_sample_millis = millis();
    has_sample = true;
}

static void reset_history(bool demo) {
    if (!history_samples) return;

    for (int i = 0; i < TEMP_HISTORY_MAX_SAMPLES; ++i) {
        for (int t = 0; t < TEMP_HISTORY_TRACKS; ++t) {
            history_samples[i][t] = INT16_MIN;
        }
    }
    head_idx = 0;
    has_sample = false;
    history_version++;

    history_is_demo = demo;
    if (demo) temp_history_seed_demo();
}

void temp_history_init(void) {
    if (!history_samples) {
        history_samples = (int16_t (*)[TEMP_HISTORY_TRACKS])heap_caps_malloc(
            sizeof(int16_t) * TEMP_HISTORY_MAX_SAMPLES * TEMP_HISTORY_TRACKS, MALLOC_CAP_SPIRAM);
        if (!history_samples) {
            history_samples = (int16_t (*)[TEMP_HISTORY_TRACKS])malloc(
                sizeof(int16_t) * TEMP_HISTORY_MAX_SAMPLES * TEMP_HISTORY_TRACKS);
        }
    }
    reset_history(state.debug_mode);
}

static int16_t get_track_current_raw(int track_idx) {
    int sensor_idx;
    if (track_idx == 0) {
        sensor_idx = state.outdoor_temp_sensor;
    } else if (track_idx == 1) {
        sensor_idx = state.indoor_temp_sensor;
    } else {
        sensor_idx = state.temps_custom_sensor[track_idx - 2];
    }
    if (sensor_idx < 0 || sensor_idx >= TEMP_SOURCE_COUNT ||
        !(state.temp_fields & (1U << sensor_idx))) {
        return INT16_MIN;
    }
    const float value = state.temp_sensors[sensor_idx];
    if (!isfinite(value) || value <= -40.0f || value >= 80.0f) return INT16_MIN;
    return (int16_t)roundf(value * 10.0f);
}

void temp_history_tick(void) {
    if (!history_samples) return;
    StateLockGuard lock;
    if (history_is_demo != state.debug_mode) reset_history(state.debug_mode);
    if (state.data_is_demo != history_is_demo) return;
    uint32_t now = millis();
    // Sample once every 60 seconds (60000 ms)
    if (has_sample && now - last_sample_millis < 60000) return;

    const bool can_sample = history_is_demo ||
        (state.vanpi_connected && now - state.temp_updated_ms <= temperature_freshness_ms);
    int16_t samples[TEMP_HISTORY_TRACKS];
    bool any_valid = false;
    for (int t = 0; t < TEMP_HISTORY_TRACKS; ++t) {
        samples[t] = can_sample ? get_track_current_raw(t) : INT16_MIN;
        any_valid |= samples[t] != INT16_MIN;
    }
    if (!has_sample && !any_valid) return;

    last_sample_millis = now;
    has_sample = true;
    for (int t = 0; t < TEMP_HISTORY_TRACKS; ++t) {
        history_samples[head_idx][t] = samples[t];
    }

    head_idx = (head_idx + 1) % TEMP_HISTORY_MAX_SAMPLES;
    history_version++;
}

bool temp_history_get_series(int track_idx, int window_hours, lv_coord_t *out_points, int out_count, float *out_min, float *out_max) {
    if (!history_samples || !out_points || out_count <= 0 || track_idx < 0 || track_idx >= TEMP_HISTORY_TRACKS) {
        return false;
    }

    // Check if custom track is disabled
    if (track_idx >= 2) {
        int custom_slot = track_idx - 2;
        if (state.temps_custom_sensor[custom_slot] < 0 || state.temps_custom_sensor[custom_slot] >= TEMP_SOURCE_COUNT) {
            for (int i = 0; i < out_count; ++i) {
                out_points[i] = LV_CHART_POINT_NONE;
            }
            if (out_min) *out_min = 0.0f;
            if (out_max) *out_max = 0.0f;
            return false;
        }
    }

    int total_minutes = constrain(window_hours, 1, 24) * 60;
    float step = (float)(total_minutes - 1) / (float)(out_count - 1);

    int16_t min_raw = 32767;
    int16_t max_raw = -32768;
    bool has_valid = false;

    for (int i = 0; i < out_count; ++i) {
        // i = 0 is oldest (total_minutes ago), i = out_count - 1 is newest (now)
        float minutes_ago = (float)(out_count - 1 - i) * step;
        int offset = (int)roundf(minutes_ago);
        int buf_idx = (head_idx - 1 - offset + TEMP_HISTORY_MAX_SAMPLES * 2) % TEMP_HISTORY_MAX_SAMPLES;

        int16_t val = history_samples[buf_idx][track_idx];
        if (val == INT16_MIN) {
            out_points[i] = LV_CHART_POINT_NONE;
        } else {
            out_points[i] = (lv_coord_t)val;
            if (val < min_raw) min_raw = val;
            if (val > max_raw) max_raw = val;
            has_valid = true;
        }
    }

    if (has_valid) {
        if (out_min) *out_min = (float)min_raw / 10.0f;
        if (out_max) *out_max = (float)max_raw / 10.0f;
        return true;
    } else {
        if (out_min) *out_min = 0.0f;
        if (out_max) *out_max = 0.0f;
        return false;
    }
}
