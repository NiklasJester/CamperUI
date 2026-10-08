#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <Arduino.h>

void ota_updater_init();
void ota_updater_loop();
void ota_check_now(bool manual_trigger = false);
void ota_start_update();

#endif // OTA_UPDATER_H
