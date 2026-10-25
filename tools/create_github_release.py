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
    return 'V1.9.0'

def create_release():
    token = get_token()
    if not token:
        print("ERROR: Could not retrieve GitHub token from git credential manager.")
        sys.exit(1)

    version = get_version()
    tag = version
    title = f"CamperUI_{version}"
    body = f"""# 🚐 CamperUI Release {version}

Release **{version}** bringt drei große Meilensteine für CamperUI: die neue **dynamische Theme-Engine mit Live-Vorschau und Sofort-Übernahme**, native **CI-Bus-Steuerung für Dometic RC10 Kühlschränke** sowie die Integration von **Webasto Standheizungen**.

---

### 🎨 1. Vollständige Theme-Engine & Web-Designer

CamperUI lässt sich ab sofort nahtlos an das Interieur und die Lichtstimmung deines Campers anpassen:
* **Webbasierter Theme-Editor mit Live-Vorschau:**
  * Unter `http://<IP-des-Displays>/settings` -> *Theme & Design* steht ein interaktiver Editor zur Verfügung, der Farb- und Geometrieänderungen in einer simulierten Display-Vorschau in Echtzeit anzeigt.
* **Umfassende Farb- & Stiloptionen:**
  * **Komponentenfarben:** Eigene Farben für Dimmer-Slider (Licht), Schalter/Relais (Aktiv), Füllstände (Frischwasser/Tanks) und UI-Buttons (z. B. Nullpunkt Tara, WLAN Scan).
  * **Oberflächen & Kontraste:** App-Hintergrund, Kachel- & Header-Hintergrund, Rahmenfarben und Regler-Schienen.
  * **Typografie:** Haupttext, gedämpfter Text, normale Button-Schrift und aktive Button-Schrift (z. B. bei eingeschalteten Relais).
  * **Form & Rundungen:** Kachelrundungen (0..24 px), Buttonrundungen (0..18 px) und Rahmenstärken (0..3 px).
* **Live-Übernahme in unter 100 ms ohne Neustart:**
  * Beim Klick auf **„Theme speichern & anwenden“** wird das Theme im NVS-Flash gespeichert und die LVGL-Displayoberfläche sofort live neu gerendert – ganz ohne Display-Freeze, ohne TCP-Verbindungsabbruch und ohne Neustart.
* **Designvorlagen (Presets):**
  * *Camper Slate (Standard Blau / Bernstein / Grün)*
  * *Nordic Amber (Warmes Bernstein / Nacht-Orange)*
  * *Forest Green (Moosgrün & Eiche)*
  * *Ocean Breeze (Marineblau & Cyan)*
  * *Modern Minimal (Tiefschwarz & Weiß, 0px Eckenrundung)*
  * *Sunset Terracotta (Warmgrau & Terracotta)*
* **Theme-Datei Import & Export:**
  * Eigene Designs können als `.json`-Datei exportiert, gesichert und auf anderen Displays hochgeladen werden.
  * Eigener Button zum einfachen Zurücksetzen auf die Werkseinstellungen.
* **Entwickler-Dokumentation:**
  * Ein ausführlicher Developer Guide in [`docs/THEMES.md`](docs/THEMES.md) beschreibt alle Theme-Variablen und Best Practices für Entwickler neuer Tabs.

---

### ❄️ 2. Dometic RC10 CI-Bus Kühlschrank-Integration

Für Dometic Kühlschränke der 10er-Serie (z. B. RC 10.4T, RCL 10.4ET) mit VanPi CI-Bus-Anbindung (`/dometic_rc10_ci/`):
* **Eigener Kühlschrank-Tab:**
  * Vollständig in die Menüleiste integriert, kann in den Einstellungen wie alle anderen Tabs beliebig verschoben, aktiviert oder deaktiviert werden.
* **Kühlstufen 1 bis 5:**
  * Große Minus-/Plus-Buttons sowie 5 direkt antippbare Segment-Balken zur gradgenauen Einstellung der Kältestufe.
* **Drei Betriebsmodi:**
  * **Performance:** Standardbetrieb mit automatischer Regelung (Stufen 1–5).
  * **Silent:** Flüsterleiser Nachtbetrieb mit gedrosseltem Kompressor und Lüfter (Kühlstufe auf max. 3 begrenzt).
  * **Turbo:** Maximale Kälteleistung (feste Stufe 5 für schnelles Herunterkühlen nach dem Einkauf).
* **Umfassende CI-Bus Live-Telemetrie:**
  * Anzeige der aktuellen Stromquelle (**12V DC**, **24V DC**, **230V AC**).
  * Live-Statusanzeige für Kompressor- und Zusatzlüfter-Aktivität.
  * Automatische Klartext-Fehlerdiagnose bei CI-Bus-Meldungen.
* **Kühlschranktemperatur-Sensor:**
  * In den Einstellungen kann ein VanPi-Temperatursensor (z. B. RuuviTag oder kabelgebundener Fühler) als Kühlschrank-Temperatur festgelegt werden. Der Messwert wird dezent oben rechts im Header des Kühlschrank-Tabs mit Schneeflocken-Symbol angezeigt.

---

### 🔥 3. Webasto Standheizungs-Integration

* **Native Webasto-Unterstützung:**
  * Auswahl von Webasto in den Heizungseinstellungen (`HEATER_TYPE_WEBASTO`).
  * Steuerung über VanPi REST-Endpunkte (`/heater/generic` bzw. `/heater`).
  * Umschaltung zwischen Heiz- und Lüftungsmodus, Sollwertvorgabe und Betriebsstatus.
  * Telemetrie- und Spannungsüberwachung (`heatvolt`).

---

### 📱 4. Weitere Detailverbesserungen & Stabilität

* **WLAN-Scan Fehlerbehebung:**
  * Das Scannen nach Netzwerken funktioniert nun auch dann zuverlässig, wenn zuvor ein noch nicht verbundenes Netzwerk eingetragen war (`wifi_prepare_for_scan` / `wifi_finish_scan`).
* **Aufgeräumter Header:**
  * Die Versionsangabe wurde aus der oberen Statusleiste entfernt, wodurch die Smart Header Badges (Frost, Batterie, WLAN etc.) nun optimal vertikal zentriert neben der Uhrzeit dargestellt werden.
* **Dynamischer System-Status:**
  * Versionsnummer und Demo-/Live-Modus werden sauber im Statusfeld am Ende des Einstellungen-Tabs angezeigt und aktualisieren sich sofort bei Umschaltung.
* **MaxxFan & Relais Theming:**
  * Der MaxxFan-Tab und alle Buttons wurden vollständig an das aktive Theme angepasst; die vorherige Verzögerung bei der Umschaltanimation von Schaltern wurde behoben.

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
