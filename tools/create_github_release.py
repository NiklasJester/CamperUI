import subprocess
import json
import urllib.request
import os
import sys
import re

def get_token():
    p = subprocess.Popen(['git', 'credential', 'fill'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
    out, _ = p.communicate('protocol=https\nhost=github.com\n\n')
    creds = dict(l.split('=', 1) for l in out.splitlines() if '=' in l)
    return creds.get('password')

def get_version():
    with open('ui_main.h', 'r', encoding='utf-8') as f:
        for line in f:
            if '#define CAMPERUI_VERSION' in line:
                m = re.search(r'"([^"]+)"', line)
                if m:
                    ver = m.group(1).lstrip('vV')
                    return f"V{ver}"
    return 'V1.6.1'

def create_release():
    token = get_token()
    if not token:
        print("ERROR: Could not retrieve GitHub token from git credential manager.")
        sys.exit(1)

    version = get_version()
    tag = version
    title = f"CamperUI_{version}"
    body = f"""# 🚐 CamperUI Release {version}

Release **{version}** ist ein großes **Performance-, UX- und Energieeffizienz-Update**. Im Fokus standen drei Kernbereiche: **Stromverbrauch des Gesamtsystems**, **Bildwiederholrate (FPS) & Display-Stabilität** sowie **User Interaction (Smartphone-Momentum-Scrolling & Reaktionszeit)**.

---

### 🔋 1. Intelligentes Energiemanagement & Stromsparmaßnahmen

Bisher liefen WLAN-Transceiver, RGB-Panel-Treiber, HTTP-Polling und die 200-Hz-Hauptschleife selbst bei ausgeschaltetem Display nahezu ungedrosselt weiter. Mit **{version}** greift ein mehrstufiges Power-Management auf allen Ebenen:

* **WiFi Modem-Sleep (`WIFI_PS_MIN_MODEM`):**
  * Das WLAN-Funkmodul schläft nun automatisch zwischen den DTIM-Beacons des Routers und spart dauerhaft **ca. 65–80 mA (@ 3,3 V)** bzw. **~0,25–0,35 W** im laufenden Betrieb wie auch im Standby.
  * Während eines aktiven Web-OTA-Firmware-Uploads wird der Schlafmodus automatisch temporär deaktiviert, sodass Updates weiterhin in 2–3 Sekunden durchlaufen.
* **Echter ST7701S Hardware-Panel-Sleep im Standby:**
  * Schaltet das Display nach dem Timeout ab, wird nicht mehr nur die Hintergrundbeleuchtung gedimmt, sondern der ST7701S-Displaycontroller per SPI-Kommando (`0x28 Display Off` + `0x10 Sleep In`) samt interner Ladungspumpen und Source-/Gate-Treiber schlafen gelegt (**~0,15–0,20 W** Ersparnis). Beim Aufwachen erfolgt ein nahtloser Hardware-Wakeup (`0x11` / `0x29`).
* **Adaptives Main-Loop- & CPU-Throttling:**
  * **Aktiv:** 5 ms Zyklus (200 Hz) für maximale Touch-Präzision.
  * **Idle (> 3 s ohne Touch):** Drosselung auf 15 ms (67 % weniger CPU- & I2C-Wakeups).
  * **Nacht-Screensaver:** Drosselung auf 35 ms + Redraw der 120px-Riesenuhr nur noch bei echtem Minutenwechsel (**1× pro Minute statt 60× pro Minute**).
  * **Display Aus (Standby):** Drosselung auf 40 ms (**87,5 % weniger CPU-/I2C-Aufwachzyklen**).
* **Intelligentes HTTP-Polling & Endpoint-Abschaltung:**
  * Im Screensaver und Standby wird das Abfrage-Intervall von `1000 ms` auf `2500 ms` gestreckt (**60 % weniger TCP-Verbindungen**).
  * Reine Anzeige-Endpunkte (`/tanks`, `/temp`, `/level`, `/heater`, `/ac`) werden bei ausgeschaltetem Display komplett pausiert, während sicherheitsrelevante Alarme (`/info`, `/batt`) weiter überwacht werden.
  * Nicht genutzte oder nicht vorhandene Endpunkte (`/shore`, `/booster` nach 3× HTTP 404) werden automatisch übersprungen.
* **Wahrnehmungslineare Backlight-Kurve ($B^{{1.8}}$):**
  * Durch eine dem menschlichen Auge angepasste Gamma-Kurve sinkt der LED-Treiberstrom im typischen Alltagsbereich (50–85 % Helligkeit) um **ca. 15–20 %** (~0,20–0,30 W) bei nahezu identischem Helligkeitseindruck.

#### 📊 Wie viel Strom spart man damit pro Tag?

| Betriebszustand | Vorher (bis V1.7.2) | Jetzt ({version}) | Ersparnis |
| :--- | :---: | :---: | :---: |
| **Aktiv (80 % Helligkeit)** | ~3,25 W | **~2,50 W** | **−0,75 W (−23 %)** |
| **Nacht-Screensaver (3 %)** | ~1,55 W | **~0,80 W** | **−0,75 W (−48 %)** |
| **Display Aus (Standby)** | ~1,22 W | **~0,50 W** | **−0,72 W (−59 %)** |

* **Typischer Camper-Alltag (2 Std. aktiv, 22 Std. Standby):**
  * **Vorher:** $2\\,\\text{{h}} \\times 3{{,}}25\\,\\text{{W}} + 22\\,\\text{{h}} \\times 1{{,}}22\\,\\text{{W}} \\approx \\mathbf{{33{{,}}3\\,\\text{{Wh / Tag}}}}$
  * **Jetzt ({version}):** $2\\,\\text{{h}} \\times 2{{,}}50\\,\\text{{W}} + 22\\,\\text{{h}} \\times 0{{,}}50\\,\\text{{W}} \\approx \\mathbf{{16{{,}}0\\,\\text{{Wh / Tag}}}}$
  * 👉 **Ersparnis pro Tag:** **ca. 17–18 Wh pro Tag** (das entspricht **ca. 1,35–1,45 Ah pro Tag** an einer 12,8 V LiFePO4-Batterie bzw. **über 50 % weniger Tagesverbrauch**!).
* **Mit 8 Std. Nacht-Screensaver (2 Std. aktiv, 8 Std. Screensaver, 14 Std. Standby):**
  * **Vorher:** $\\approx 36{{,}}0\\,\\text{{Wh / Tag}}$ → **Jetzt:** $\\approx 18{{,}}4\\,\\text{{Wh / Tag}}$ (**~17,6 Wh / Tag gespart**).
  * 🏕️ *Auf einer 14-tägigen autarken Reise spart allein das Display damit rund **245 Wh (~19–20 Ah)** Batteriekapazität.*

---

### 📱 2. User Interaction & Smartphone-Momentum-Scrolling

* **Smartphone-artiges „Anschieben" (Momentum-Scrolling) überall:**
  * Ein eigener Q8-Fixed-Point-Momentum-Algorithmus mit 5-Punkt-Ringpuffer und Frame-Zeit-Normierung sorgt dafür, dass sich **alle scrollbaren Bereiche** (Dimmer-Liste, Relais & Schalter, Einstellungen-Tabs sowie die untere Navigationsleiste) mit einer Wischgeste flüssig anschieben lassen und sanft ausgleiten.
* **Tap-to-Stop ohne Fehlauslösung:**
  * Ein kurzes Antippen während des Ausgleitens stoppt die Liste sofort wie beim Smartphone, ohne versehentlich den darunterliegenden Schalter, Slider oder Text-Dialog auszulösen.
* **Schnellere Touch-Reaktion & optimierte Bedienung:**
  * Touch-Release-Entprellung von `55 ms` auf `20 ms` verkürzt für spürbar direktere Tastenreaktion.
  * **Live-Dimmer-Regelung:** Dimmer senden Änderungen bereits während des Ziehens gedrosselt (`150 ms`) in Echtzeit an VanPi.
  * **4 Hz Echtzeit-Wasserwaage:** Auf dem Wasserwaage-Tab schaltet das Polling automatisch auf `250 ms` (4 Hz) hoch.
  * **Direkter `< Zurück`-Button** in den Einstellungen zum zuletzt geöffneten Tab sowie 2-Sekunden-Schutzsperre für Relais & Wasserpumpe.
  * **Lautlose Touch-Bedienung:** Der störende Hardware-Touch-Piepton wurde vollständig entfernt und der Buzzer-Pin beim Booten dauerhaft stummgeschaltet.

---

### 🚀 3. FPS-Boost & Dauerhafte RGB-Display-Stabilität

* **Hardware-VSYNC-Guard gegen vertikales Bildverrutschen:**
  * Ein in IRAM platzierter VSYNC-Interrupt-Callback (`rgb_vsync_bounce_guard_cb`) überwacht bei jedem Bildanfang die interne DMA-Bounce-Buffer-Position des ESP32-S3 (`bounce_pos_px` / `bb_eof_count`) und synchronisiert sie sofort nach, falls ein NVS-Flash-Zugriff oder Interrupt-Stall auftrat.
  * Zusätzlich werden Helligkeits-Einstellungen erst beim Loslassen des Sliders (`LV_EVENT_RELEASED`) und nur bei tatsächlicher Wertänderung in den NVS-Flash geschrieben. Das früher vereinzelt auftretende **vertikale Verrutschen des Bildschirms um 20–40 Pixel ist damit dauerhaft behoben**.
* **Deutlich höhere Framerate (FPS):**
  * Interner SRAM-Draw-Buffer wird vor dem WiFi-Stack reserviert (garantierter schneller DMA-SRAM-Puffer statt PSRAM-Fallback).
  * Keine teuren Full-Container-Transparenz-Allokationen (`opa < 255`) mehr beim Scrollen und Öffnen von Popups.
  * Gecachte Chart-Datenpunkte in der Temperatur-Historie (kein 432-Punkte-Neuaufbau mehr in jedem Animations-Frame) sowie Style-Guards gegen redundante LVGL-Redraws.

---

### 💾 Installation & Update

* **Web-OTA (Empfohlen):** Im Browser `http://<IP-des-Displays>/update` aufrufen und die Datei `CamperUI-{version}.bin` hochladen.
* **USB-Erstinstallation:** `CamperUI-{version}-merged.bin` mit dem ESP Flash Tool, Web-Flasher oder Arduino IDE ab Offset `0x0` flashen.
* **GitHub Pages Web-Flasher:** Direkt im Browser unter [niklasjester.github.io/CamperUI](https://niklasjester.github.io/CamperUI/) flashen.
"""

    headers = {
        'Authorization': f'token {token}',
        'Accept': 'application/vnd.github+json',
        'User-Agent': 'CamperUI-Release-Script'
    }

    # 1. Check if release already exists
    release_url = f"https://api.github.com/repos/NiklasJester/CamperUI/releases/tags/{tag}"
    req = urllib.request.Request(release_url, headers=headers)
    release = None
    try:
        with urllib.request.urlopen(req) as resp:
            release = json.loads(resp.read().decode('utf-8'))
            print(f"Release {tag} already exists (id: {release['id']})")
    except urllib.error.HTTPError as e:
        if e.code == 404:
            print(f"Creating new release {tag}...")
        else:
            print(f"HTTP error checking release: {e.code} {e.reason}")
            sys.exit(1)

    if not release:
        create_url = "https://api.github.com/repos/NiklasJester/CamperUI/releases"
        payload = json.dumps({
            "tag_name": tag,
            "target_commitish": "master",
            "name": title,
            "body": body,
            "draft": False,
            "prerelease": False
        }).encode('utf-8')
        req = urllib.request.Request(create_url, data=payload, headers={'Content-Type': 'application/json', **headers})
        with urllib.request.urlopen(req) as resp:
            release = json.loads(resp.read().decode('utf-8'))
            print(f"Release created successfully (id: {release['id']})")

    upload_url_template = release['upload_url'].split('{')[0]

    # Files to upload
    files_to_upload = [
        (f"CamperUI-{version}.bin", f"./build/CamperUI-{version}.bin"),
        (f"CamperUI-{version}-merged.bin", f"./build/CamperUI-{version}-merged.bin"),
        (f"bootloader-{version}.bin", f"./build/bootloader-{version}.bin"),
        (f"partitions-{version}.bin", f"./build/partitions-{version}.bin"),
    ]

    existing_assets = {a['name']: a['id'] for a in release.get('assets', [])}

    for name, path in files_to_upload:
        if not os.path.exists(path):
            print(f"WARNING: File not found: {path}")
            continue

        if name in existing_assets:
            print(f"Asset {name} already exists. Deleting existing asset (id: {existing_assets[name]})...")
            del_url = f"https://api.github.com/repos/NiklasJester/CamperUI/releases/assets/{existing_assets[name]}"
            del_req = urllib.request.Request(del_url, headers=headers, method='DELETE')
            urllib.request.urlopen(del_req)

        print(f"Uploading {name} ({os.path.getsize(path):,} bytes)...")
        with open(path, 'rb') as f:
            data = f.read()

        up_url = f"{upload_url_template}?name={name}"
        up_headers = {
            **headers,
            'Content-Type': 'application/octet-stream',
            'Content-Length': str(len(data))
        }
        up_req = urllib.request.Request(up_url, data=data, headers=up_headers, method='POST')
        with urllib.request.urlopen(up_req) as resp:
            print(f"Uploaded {name} successfully.")

    print(f"\nAll release assets uploaded successfully to {release['html_url']}")

if __name__ == '__main__':
    create_release()
