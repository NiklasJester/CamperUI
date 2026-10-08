# CamperUI 🚐✨
> **Modernes, hochauflösendes Touch-Bedienpanel für VanPi & Campervans**  
> Entwickelt für das **Waveshare ESP32-S3-Touch-LCD-4** (4.0 Zoll, 480x480 RGB, kapazitiver Touch) mit **LVGL 8.4**.

---

## 📸 Screenshots & Benutzeroberfläche

| 💡 Dimmer & Licht (Home Assistant Style) | ⚡ Energie & Powerfluss |
| :---: | :---: |
| ![Dimmer Tab](docs/screenshots/tab_dimmers.png) | ![Power Tab](docs/screenshots/tab_power.png) |
| *Moderne 32px HA-Slider ohne Kugel, 4 Dimmer bündig* | *Live-Berechnung: Solar -> Batterie -> Camper-Last* |

| 💧 Wassertanks & Ventile | 🌡️ Klima & Standheizung |
| :---: | :---: |
| ![Wasser Tab](docs/screenshots/tab_water.png) | ![Klima Tab](docs/screenshots/tab_climate.png) |
| *160px hohe Füllstandsbalken & Schnellzugriff-Buttons* | *Thermostat-Bogen mit Greifknob & Modusauswahl* |

| ⚖️ Wasserwaage & Keil-Assistent | 🔌 Relais & Schalter |
| :---: | :---: |
| ![Wasserwaage Tab](docs/screenshots/tab_level.png) | ![Switches Tab](docs/screenshots/tab_switches.png) |
| *2D Dosenlibelle & 4-Rad Keilausgleich in cm* | *Kompakte 2x3 Kacheln mit Status-Rückmeldung* |

---

## 🌟 Highlights & Funktionen

### 1. 💡 Dimmer / Beleuchtung (Home Assistant Style)
* **Tile-Slider-Design:** Inspiriert von modernen Smart-Home-Dashboards (Home Assistant / Mushroom).
* **32px dicke Füllstandsbalken:** Deutlich sichtbare Führungsschiene mit dezentem Rand und sanfter Abrundung.
* **Kein Kugelknopf:** Bei 0% ist der Balken komplett leer (kein störender Knauf am Rand).
* **Optimierte Touchbedienung:** Stufenloses Wischen oder direktes Antippen der gewünschten Helligkeit.
* **Exakte Passform:** Genau 4 Dimmer füllen das 480x480 Display perfekt aus, ohne vertikales Scrollen.

### 2. ⚡ Energie & Batterie-Dashboard
* **Intelligenter Energiefluss:** Dynamische Berechnung der aktuellen Camper-Last ({\\text{last}} = P_{\\text{solar}} - P_{\\text{batt}}$) und Richtungspfeile.
* **LiFePO4 Batterie-Monitoring:** Präzise Anzeige von Ladezustand (SoC %), Spannung (V), Lade-/Entladestrom (A) und Restlaufzeit.
* **Solar-Einspeisung:** Live-Erfassung der PV-Leistung in Watt.

### 3. 💧 Wassertanks & Pumpensteuerung
* **Große 160px Füllstandsbalken:** Klare Farbtrennung (Frischwasser Cyan, Grauwasser Schiefergrau).
* **Exakte Zentrierung:** Perfekt ausgerichtete Beschriftungen, Liter- und Prozentanzeigen.
* **Schnellschalter:** Große Buttons für Wasserpumpe und elektrisches Abwasserventil.

### 4. 🌡️ Klima & Heizungssteuerung
* **Thermostat-Drehbogen:** Großzügiger Arc-Slider mit weißem, kontraststarkem Bedienknob und Schlagschatten.
* **Modus-Auswahl:** Schnellumschaltung zwischen *Heizen (Temp)*, *Heizen (Stufe)* und *Lüften*.
* **Status auf einen Blick:** Außentemperatur oben links, Ist- und Zieltemperatur zentriert im Bogen.

### 5. ⚖️ Digitale Wasserwaage & Keil-Assistent
* **2D-Dosenlibelle:** Präzise grafische Libelle mit 0.5°-Toleranz-Zielring und Längs-/Querneigung in Grad.
* **Keil-Ausgleichsassistent:** Berechnet aus Geometrie und Neigungswinkeln direkt die benötigte Unterleghöhe in **Zentimetern** für jedes einzelne Rad (*Vorne L*, *Vorne R*, *Hinten L*, *Hinten R*).
* **Tara-Funktion:** Sendet den aktuellen Nullpunkt direkt an den VanPi-Neigungssensor.

### 6. 🔌 Lastrelais / Verbraucher
* Bis zu 8 individuell benennbare Relais.
* Farbliches Zustandsfeedback (Grün aktiv, Anthrazit Standby).

### 7. 🔔 Smart Header Badges & Alarme
* **WLAN & Empfang:** Mehrstufige Signalstärkeanzeige (Grün / Gelb / Rot).
* **Dynamische Warn-Badges:** Erscheinen automatisch im Header bei:
  * ❄️ Frostgefahr (Außentemperatur unter Grenzwert)
  * 🔋 Niedriger Batteriestand (SoC unter Grenzwert)
  * 💧 Frischwassermangel (unter Minimalwert)
  * ⚠️ Abwassertank voll (über Maximalwert)
* **Alle Grenzwerte frei konfigurierbar** und im NVS-Flash gespeichert.

### 8. ⚙️ Einstellungen & Offline-Simulation
* On-Screen-Tastatur zum Konfigurieren von WLAN-SSID, Passwort und VanPi-IP.
* Display-Timeout, Helligkeit und Dark/Light-Mode.
* **Integrierter Debug-Modus:** Simuliert realistische Sensordaten per Knopfdruck – ideal zum Testen ohne Fahrzeug.

---

## 🛠️ Hardware-Anforderungen

* **Board:** [Waveshare ESP32-S3-Touch-LCD-4](https://www.waveshare.com/esp32-s3-touch-lcd-4.htm)
* **Display:** 4.0 Zoll IPS, 480 × 480 Pixel, RGB-Interface (ST7701S Treiber)
* **Touchpanel:** Kapazitiver I2C-Touchcontroller (Goodix GT911)
* **Speicher:** 16 MB Quad-SPI Flash, 8 MB Octal-SPI PSRAM
* **Konnektivität:** 2.4 GHz Wi-Fi & Bluetooth 5 (LE)

---

## 💻 Kompilierung & Flashen

### Empfohlene Einstellungen (Arduino IDE 2.x / CLI)

| Option | Einstellung |
| :--- | :--- |
| **Board** | ESP32S3 Dev Module |
| **USB CDC On Boot** | Enabled |
| **Flash Size** | 16MB (128Mb) |
| **Partition Scheme** | 16M Flash (3MB APP/9.9MB FATFS) (pp3M_fat9M_16MB) |
| **PSRAM** | OPI PSRAM |

### LVGL-Konfiguration installieren

Die getestete Konfiguration für LVGL 8.4 liegt unter `config/lv_conf.h`.

1. Den Sketchbook-Speicherort unter **Datei → Voreinstellungen** in der Arduino IDE prüfen.
2. Eine vorhandene `libraries/lv_conf.h` sichern.
3. `config/lv_conf.h` in den `libraries`-Ordner des Sketchbooks kopieren, direkt neben den Ordner `lvgl`.
4. CamperUI erneut kompilieren und hochladen.

Die Konfiguration verwendet 128 KiB LVGL-Speicher statt der 48 KiB
aus dem getesteten Waveshare-Paket. Auf einem Waveshare Rev04 behob
diese Änderung einen Startabsturz beim Aufbau der CamperUI-Oberfläche.

LVGL-Logging ist für die Diagnose aktiviert. Die serielle Ausgabe
kann mit 115200 Baud gelesen werden.

Die Datei unter `config/` wird nicht automatisch von Arduino verwendet.
Nach Änderungen muss sie erneut in den Sketchbook-Ordner kopiert werden.



### Flashen via Arduino CLI

`powershell
arduino-cli compile --upload -p COM4 --fqbn esp32:esp32:esp32s3:CDCOnBoot=cdc,PSRAM=opi,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB CamperUI.ino
`

---

## 📁 Projektstruktur

`	ext
CamperUI/
├── CamperUI.ino            # Hauptprogramm, Setup & Main-Loop
├── ui_main.cpp / .h        # LVGL-Initialisierung, Status-Bar, Navigation, Theme
├── ui_power.cpp            # Tab 1: Batterie & Energiefluss
├── ui_water.cpp            # Tab 2: Wassertanks & Pumpensteuerung
├── ui_climate.cpp          # Tab 3: Thermostat & Standheizung
├── ui_switches.cpp         # Tab 0 & 4: Dimmer & Schalter
├── ui_level.cpp            # Tab 5: Wasserwaage & 4-Rad Keilassistent
├── ui_settings.cpp         # Tab 6: Systemeinstellungen & Kalibrierung
├── ui_mdi_icons.c / .h     # 32px & 18px Material Design Vektor-Icon-Fonts
├── http_handler.cpp / .h   # VanPi REST-Client & HTTP-Publisher
├── system_state.cpp / .h   # Datenmodell & NVS-Flash-Persistenz
├── tools/                  # Hilfswerkzeuge
│   ├── gen_mdi_script.py   # Icon-Font Generator
│   ├── simulate_vanpi.py   # Lokaler VanPi Mock-Server
│   └── materialdesignicons-webfont.ttf
└── docs/                   # Dokumentation & Assets
    ├── screenshots/        # 480x480 Display-Vorschauen
    └── flows.json          # Node-RED Referenz-Flows
`

---

## 🔗 VanPi Schnittstellen & Endpunkte

CamperUI kommuniziert bidirektional über das VanPi HTTP REST-Interface:
* GET /api/v1/ bzw. GET /state: Abfrage aller Live-Sensoren (Batterie, Solar, Tanks, Relais, Klima).
* POST /api/v1/relays: Schalten der Relais und Lastkreise.
* POST /api/v1/dimmers: Einstellen von Helligkeitswerten (0-100%).
* POST /position_sensor/?request=calibrate: Nullpunkt-Kalibrierung der Wasserwaage.

---

## 📄 Lizenz
Open-Source (MIT License). Entwickelt für die VanPi Camper-Community.
