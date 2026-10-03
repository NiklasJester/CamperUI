#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include <Arduino.h>

void http_init();
void http_start_task();
void http_loop();

void http_fetch_names();
void http_publish_switch(int index, bool on);
void http_publish_dimmer(int index, int val);

void http_publish_heater_cmd(String mode, int value);
void http_calibrate_position();

#endif
