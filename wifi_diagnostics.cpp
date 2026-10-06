#include "wifi_diagnostics.h"
#include "system_state.h"
#include <WiFi.h>
#include <esp_wifi_types.h>
#include <atomic>
namespace {
std::atomic<unsigned> last_reason{0};
std::atomic<uint32_t> attempt_started{0};
const char *reason_text(unsigned reason) {
    switch (reason) {
        case WIFI_REASON_NO_AP_FOUND: return "Netz nicht gefunden: SSID, Reichweite und 2.4 GHz pruefen.";
        case WIFI_REASON_AUTH_FAIL: return "Anmeldung abgelehnt: Passwort oder Router-Zugriff pruefen.";
        case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
        case WIFI_REASON_HANDSHAKE_TIMEOUT: return "Anmeldung ohne Antwort: Passwort oder Signal pruefen.";
        case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:
        case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD: return "Router-Sicherheit passt nicht zur WLAN-Anmeldung.";
        case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD: return "WLAN-Signal zu schwach.";
        case WIFI_REASON_BEACON_TIMEOUT: return "Verbindung verloren: Signal oder Router pruefen.";
        case WIFI_REASON_ASSOC_LEAVE: return "Verbindung getrennt; neuer Verbindungsversuch.";
        default: return "Verbindung fehlgeschlagen: Router, Signal und Zugang pruefen.";
    }
}
}
void wifi_diagnostics_init() {
    static bool registered = false;
    if (registered) return;
    registered = true;
    WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
        last_reason.store(info.wifi_sta_disconnected.reason);
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t) {
        last_reason.store(0);
    }, ARDUINO_EVENT_WIFI_STA_GOT_IP);
}
void wifi_connect_configured() {
    last_reason.store(0); attempt_started.store(millis());
    WiFi.setHostname("esp32s3-camper-ui");
    WiFi.persistent(false);
    if (state.wifi_ssid.isEmpty()) return;
    WiFi.begin(state.wifi_ssid.c_str(), state.wifi_pass.c_str());
}
String wifi_connection_summary() {
    if (WiFi.status() == WL_CONNECTED) return "WLAN verbunden (" + String(WiFi.RSSI()) + " dBm)";
    if (state.wifi_ssid.isEmpty()) return "WLAN: kein Netzwerk eingestellt";
    unsigned reason = last_reason.load();
    if (reason) return "WLAN getrennt (Grund " + String(reason) + ")";
    if (WiFi.status() == WL_NO_SSID_AVAIL) return "WLAN: Netzwerk nicht gefunden";
    if (WiFi.status() == WL_CONNECT_FAILED) return "WLAN: Anmeldung fehlgeschlagen";
    return millis() - attempt_started.load() < 15000 ? "WLAN: Verbindung wird aufgebaut" : "WLAN: noch keine Verbindung";
}
String wifi_connection_details() {
    String text = wifi_connection_summary();
    if (WiFi.status() == WL_CONNECTED) {
        text += "\nNetz: " + WiFi.SSID() + "\nIP: " + WiFi.localIP().toString();
        text += "\nHostname: esp32s3-camper-ui";
    } else if (!state.wifi_ssid.isEmpty()) {
        text += "\nNetz: " + state.wifi_ssid + "\n";
        unsigned reason = last_reason.load();
        text += reason ? reason_text(reason) : "SSID, Passwort und 2.4-GHz-Netz pruefen.";
    }
    text += state.debug_mode ? "\nDaten: Dummy (WLAN-Status ist echt)" : "\nVanPi: " + String(state.vanpi_connected && WiFi.status() == WL_CONNECTED ? "erreichbar" : "noch nicht erreichbar");
    return text;
}
