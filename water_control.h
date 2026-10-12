#ifndef WATER_CONTROL_H
#define WATER_CONTROL_H

#include <Arduino.h>

void water_control_init();
void water_control_loop();

// Status helpers
int water_control_get_remaining_sec();
bool water_control_is_dry_run_active();
bool water_control_is_fill_assist_alerting();

#endif // WATER_CONTROL_H