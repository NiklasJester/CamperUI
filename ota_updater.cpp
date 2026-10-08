#include "ota_updater.h"
#include "system_state.h"
#include "ui_main.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>

static TaskHandle_t ota_task_handle = NULL;

static bool is_version_newer(const String &cur_ver, const String &new_ver) {
    int cur_maj = 0, cur_min = 0, cur_pat = 0;
    int new_maj = 0, new_min = 0, new_pat = 0;

    const char *c = cur_ver.c_str();
    while (*c && !isdigit(*c)) c++;
    sscanf(c, "%d.%d.%d", &cur_maj, &cur_min, &cur_pat);

    const char *n = new_ver.c_str();
    while (*n && !isdigit(*n)) n++;
    sscanf(n, "%d.%d.%d", &new_maj, &new_min, &new_pat);

    if (new_maj > cur_maj) return true;
    if (new_maj < cur_maj) return false;
    if (new_min > cur_min) return true;
    if (new_min < cur_min) return false;
    return new_pat > cur_pat;
}

static void ota_check_task(void *pvParameters) {
    if (WiFi.status() != WL_CONNECTED) {
        state.ota_state = OTA_STATE_FAILED;
        state.ota_status_msg = "WLAN nicht verbunden";
        ota_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    state.ota_state = OTA_STATE_CHECKING;
    state.ota_status_msg = "Pruefe auf Updates...";

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(8000);
    http.begin(client, "https://api.github.com/repos/NiklasJester/CamperUI/releases/latest");
    http.setUserAgent("CamperUI-ESP32");
    http.addHeader("Accept", "application/vnd.github.v3+json");

    int httpCode = http.GET();
    if (httpCode == 200) {
        String payload = http.getString();
        DynamicJsonDocument doc(12288);
        DeserializationError err = deserializeJson(doc, payload);
        if (!err) {
            String tag = doc["tag_name"].as<String>();
            if (tag.length() > 0) {
                String download_url = "";
                JsonArray assets = doc["assets"].as<JsonArray>();
                for (JsonObject a : assets) {
                    String name = a["name"].as<String>();
                    if (name.endsWith(".bin") || name.equalsIgnoreCase("CamperUI.bin")) {
                        download_url = a["browser_download_url"].as<String>();
                        break;
                    }
                }
                if (download_url.length() == 0) {
                    download_url = "https://github.com/NiklasJester/CamperUI/releases/download/" + tag + "/CamperUI.bin";
                }

                state.ota_latest_version = tag;
                state.ota_download_url = download_url;

                if (is_version_newer(CAMPERUI_VERSION, tag)) {
                    state.ota_state = OTA_STATE_AVAILABLE;
                    state.ota_status_msg = "Update verfuegbar: " + tag;
                } else {
                    state.ota_state = OTA_STATE_UP_TO_DATE;
                    state.ota_status_msg = "System ist aktuell (" + String(CAMPERUI_VERSION) + ")";
                }
            } else {
                state.ota_state = OTA_STATE_UP_TO_DATE;
                state.ota_status_msg = "Keine Version gefunden";
            }
        } else {
            state.ota_state = OTA_STATE_FAILED;
            state.ota_status_msg = "JSON Antwort fehlerhaft";
        }
    } else if (httpCode == 404) {
        state.ota_state = OTA_STATE_UP_TO_DATE;
        state.ota_status_msg = "Kein neueres Release vorhanden";
    } else {
        state.ota_state = OTA_STATE_FAILED;
        state.ota_status_msg = "Server-Fehler: HTTP " + String(httpCode);
    }

    http.end();
    ota_task_handle = NULL;
    vTaskDelete(NULL);
}

static void ota_download_task(void *pvParameters) {
    if (WiFi.status() != WL_CONNECTED || state.ota_download_url.length() < 10) {
        state.ota_state = OTA_STATE_FAILED;
        state.ota_status_msg = "Keine gueltige Download-URL";
        ota_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    state.ota_state = OTA_STATE_DOWNLOADING;
    state.ota_progress = 0;
    state.ota_status_msg = "Verbinde mit GitHub...";

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(25000);
    http.begin(client, state.ota_download_url);
    http.setUserAgent("CamperUI-ESP32");

    int httpCode = http.GET();
    if (httpCode != 200) {
        state.ota_state = OTA_STATE_FAILED;
        state.ota_status_msg = "Download fehlgeschlagen (HTTP " + String(httpCode) + ")";
        http.end();
        ota_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) {
        state.ota_state = OTA_STATE_FAILED;
        state.ota_status_msg = "Ungueltige Dateigroesse";
        http.end();
        ota_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    if (!Update.begin(contentLength, U_FLASH)) {
        state.ota_state = OTA_STATE_FAILED;
        state.ota_status_msg = "Flash-Fehler: " + String(Update.errorString());
        http.end();
        ota_task_handle = NULL;
        vTaskDelete(NULL);
        return;
    }

    WiFiClient *stream = http.getStreamPtr();
    size_t written = 0;
    uint8_t buff[2048];
    state.ota_status_msg = "Lade Update: 0%";

    while (http.connected() && (written < (size_t)contentLength)) {
        size_t available = stream->available();
        if (available > 0) {
            int toRead = available > sizeof(buff) ? sizeof(buff) : available;
            int c = stream->readBytes(buff, toRead);
            if (c > 0) {
                Update.write(buff, c);
                written += c;
                int pct = (int)((written * 100) / contentLength);
                state.ota_progress = pct;
                state.ota_status_msg = "Lade Update: " + String(pct) + "%";
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    if (written == (size_t)contentLength && Update.end(true)) {
        if (Update.isFinished()) {
            state.ota_state = OTA_STATE_SUCCESS;
            state.ota_progress = 100;
            state.ota_status_msg = "Update erfolgreich! Neustart...";
            vTaskDelay(pdMS_TO_TICKS(1500));
            ESP.restart();
        } else {
            state.ota_state = OTA_STATE_FAILED;
            state.ota_status_msg = "Flash-Ende nicht bestaetigt";
        }
    } else {
        Update.abort();
        state.ota_state = OTA_STATE_FAILED;
        state.ota_status_msg = "Download unvollstaendig";
    }

    http.end();
    ota_task_handle = NULL;
    vTaskDelete(NULL);
}

void ota_updater_init() {
    state.ota_state = OTA_STATE_IDLE;
    state.ota_progress = 0;
    state.ota_latest_version = "";
    state.ota_download_url = "";
    state.ota_status_msg = "Bereit";
}

void ota_check_now() {
    if (ota_task_handle != NULL) return;
    if (state.ota_state == OTA_STATE_DOWNLOADING || state.ota_state == OTA_STATE_FLASHING) return;
    xTaskCreatePinnedToCore(ota_check_task, "ota_check", 8192, NULL, 1, &ota_task_handle, 0);
}

void ota_start_update() {
    if (ota_task_handle != NULL) return;
    if (state.ota_state == OTA_STATE_DOWNLOADING || state.ota_state == OTA_STATE_FLASHING) return;
    xTaskCreatePinnedToCore(ota_download_task, "ota_download", 8192, NULL, 1, &ota_task_handle, 0);
}

void ota_updater_loop() {
    if (!state.auto_update_check) return;
    if (WiFi.status() != WL_CONNECTED) return;
    if (ota_task_handle != NULL) return;

    static uint32_t last_check_ms = 0;
    static bool boot_check_done = false;

    // Check 20 seconds after boot once WiFi has stabilized
    if (!boot_check_done && millis() > 20000) {
        boot_check_done = true;
        last_check_ms = millis();
        ota_check_now();
        return;
    }

    // Check periodically every 12 hours
    if (boot_check_done && (millis() - last_check_ms > 43200000UL)) {
        last_check_ms = millis();
        ota_check_now();
    }
}
