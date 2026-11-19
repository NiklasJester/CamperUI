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

Release **{version}** bringt echtes **Hardware-Double-Buffering (Anti-Tearing)** für spürbar flüssigeres Scrollen, kompaktere Relais- und Dimmer-Ansichten sowie alle Bugfixes und Erweiterungen aus [PR #8](https://github.com/NiklasJester/CamperUI/pull/8) (vielen Dank an **@RV3Dtech**!).

---

### 🚀 1. Display-Performance & Anti-Tearing
* **Hardware-Double-Buffering (`num_fbs = 2`) mit Frame-End-Sync:**
  * LVGL rendert jeden Frame vollständig in einen zweiten Framebuffer im 8 MB Octal-PSRAM; der Umschaltvorgang erfolgt atomar am exakten Frame-Ende (`on_frame_buf_complete`).
  * Vertikales und horizontales Scrollen läuft dadurch komplett **ohne Tearing** und sichtbar geschmeidiger.
  * Optimierter Dirty-Range-Sync überträgt nach dem Swap nur den tatsächlich veränderten Zeilenbereich.

---

### 🎨 2. Kompaktere UI, Einstellungen & Theme-Vereinheitlichung
* **Relais-Tab:** Kacheln auf `212 × 82 px` (16 pt) optimiert – alle **8 Relais** (4 × 2) passen nun ohne Scrollen auf eine Seite (gilt auch für WLAN-Relais).
* **Dimmer-Tab:** Kompaktere Kacheln (`54 px` hoch, 14 pt) mit großer Touch-Fläche – **6 Dimmer** passen gleichzeitig ohne Scrollen auf eine Seite.
* **Dynamische Sichtbarkeits-Einstellungen:** Deaktivierte Bereiche (*out-x-in Pro*, *WiFi-Relais*, *Dimmy Pro*) reservieren keinen leeren Platz mehr; typografische Gedankenstriche durch saubere ASCII-Bindestriche (`9-16`, `8-15`) ersetzt.
* **Master-Schalter „Dimmer anzeigen“:** Synchronisierte Ein-/Ausblendung des Dimmer-Tabs in Display-Einstellungen und Web-UI unter Erhalt der Einzelauswahl.
* **Navigationsleiste mit Scrollpfeilen:** Bei mehr als 6 aktiven Tabs erleichtern seitliche Pfeiltasten die Navigation.
* **Einheitliches Theming:** Home, Victron GX (inkl. scrollbarer Ansicht), Energie-Verläufe, Autoterm-Diagnose und Einstellungen übernehmen konsequent alle Farben, Rundungen und Rahmenstärken aus dem Theme-Editor.

---

### 🛠️ 3. Bugfixes aus dem Forum (PR #8)
* **VanPi SOC-Dezimalwerte:** Numerische Dezimal-SOC-Werte im `/batt`-Endpoint (z. B. `96.5`) werden über `jsonBatterySoc()` korrekt verarbeitet statt auf `0 %` zu springen.
* **WLAN-Relais Ausschalten:** `/wrelay/`-PUT-Befehle senden `Content-Type: text/plain; charset=utf-8` mit Body `{{}}`, um Node-RED `.includes()`-Fehler beim Ausschalten zu vermeiden.
* **Webasto & Autoterm Telemetrie:** Autoterm-Diagnose (Stethoskop) wird nur noch bei ausgewählter Autoterm-Heizung eingeblendet; Webasto-Temperatur und -Spannung werden auf Aktualität (45 s) und Gültigkeit (`> 5 V`) geprüft.
* **Energie- & Temperaturverläufe:** Aktualitätsprüfung auf 45 s angepasst (kompatibel mit Bildschirmschoner-Polling); Temperaturverlauf startet im Live-Modus ohne Demo-Kurven und markiert isolierte Messpunkte.

---

### 🔋 4. Display-Akku & Laufzeittest (Waveshare V4)
* Optionale **Display-Akku-Kachel** auf dem Home-Dashboard für die lokale Waveshare-CH32-Akkuspannung inkl. manuellem Laufzeittest (NVS-Sicherung alle 5 min) und Diagnose-Overlay.
* Optionale Statusleisten-Anzeige (`DA XX%`) in den Home-Einstellungen aktivierbar.

---

### 📦 Installation & Update

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
