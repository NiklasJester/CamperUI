import subprocess
import json
import urllib.request
import os
import sys

def get_token():
    p = subprocess.Popen(['git', 'credential', 'fill'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
    out, _ = p.communicate('protocol=https\nhost=github.com\n\n')
    creds = dict(l.split('=', 1) for l in out.splitlines() if '=' in l)
    return creds.get('password')

def create_release():
    token = get_token()
    if not token:
        print("ERROR: Could not retrieve GitHub token from git credential manager.")
        sys.exit(1)

    tag = "V1.6.0"
    title = "CamperUI_V1.6.0"
    body = """# 🚐 CamperUI Release V1.6.0

Mit Version **V1.6.0** erhält CamperUI ein komplett überarbeitetes, modernes **Energiefluss-Dashboard (Power Tab)**, dynamische Erzeuger-Konfiguration in den Einstellungen, Touch-Verlaufsanzeigen für Solar und Batterie sowie erweiterte VanPi HTTP-Telemetrie.

---

### 🚀 Highlights & Neuerungen

#### ⚡ 1. Überarbeitetes Energiefluss-Dashboard (Power Tab)
* **Zentrale Batterie- & Shunt-Einheit:**
  * Großzügiger Circular Arc Gauge mit zentrierter SoC-Prozentanzeige.
  * Watt-Leistung prominent im Fokus, flankiert vom Shunt-Strom (`+189 W (+14.2 A)` / `-88 W (-6.5 A)`).
  * Batteriespannung übersichtlich unterhalb der Leistungszeile platziert (`13.3 V`).
  * Farbcodierter Rahmen (Smaragdgrün bei Ladung, Amber bei Entladung, Neutral im Standby).
* **Dynamische Erzeuger-Leiste (Oben):**
  * Unterstützung für **Solar**, **Landstrom (230V)** und **Lichtmaschine (B2B Booster)**.
  * Automatische, saubere Zentrierung bei Teilbelegung (1, 2 oder 3 Kacheln).
  * Symmetrische Fluss-Verbindungslinien zur zentralen Batterieeinheit.
* **Intelligente Energiefluss-Linien:**
  * Bei aktivem Gesamtladevorgang leuchten alle aktiven Flusslinien (auch zum Verbraucher) in Smaragdgrün auf.
* **Minimalistische Verbraucher-Karte (Unten):**
  * Schlankes 74px-Design passend zu den Erzeugern.
  * Leistung im Fokus und dynamische Restlaufzeit- bzw. Ladedauerberechnung.

#### 📈 2. Direkter Touch-Zugriff auf Verläufe (24h History)
* **Solar-Verlauf per Fingertipp:** Ein Tippen auf die Solar-Kachel öffnet sofort das 24h-Leistungsdiagramm der Solaranlage (1h, 6h, 12h, 24h).
* **Batterie-Verlauf per Fingertipp:** Ein Tippen auf die zentrale Batteriekarte öffnet direkt den Batterie-Verlauf (SoC & Lade-/Entladeleistung).

#### ⚙️ 3. Erzeuger-Auswahl in den Einstellungen
* Im Einstellungsmenü unter **Tab 3 („Sichtbarkeit“)** können die vorhandenen Energieerzeuger individuell aktiviert oder abgewählt werden.
* Persistente Speicherung im NVS-Flash und Synchronisation über das Webinterface.

#### 🌐 4. Erweiterte VanPi HTTP-API & Simulation
* `/batt` wertet jetzt auch `VoltS` / `starter_voltage`, `shore_power` / `shore_connected` sowie `alt_power` / `alt_active` aus.
* Im Demo-/Simulationsmodus laufen alle 3 Produzenten live mit realistischen Wechselwerten.

---

### 💾 Installation & Update

* **Web-OTA (Empfohlen):** Im Browser `http://<IP-des-Displays>/update` aufrufen und die Datei `CamperUI-V1.6.0.bin` hochladen.
* **USB-Erstinstallation:** `CamperUI-V1.6.0-merged.bin` mit dem ESP Flash Tool, Web-Flasher oder Arduino IDE ab Offset `0x0` flashen.
* **GitHub Pages Web-Flasher:** Direkt im Chrome-/Edge-Browser flashen ohne Installation von Treibern oder Tools.
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
        ("CamperUI-V1.6.0.bin", "./build/CamperUI-V1.6.0.bin"),
        ("CamperUI-V1.6.0-merged.bin", "./build/CamperUI-V1.6.0-merged.bin"),
        ("bootloader-V1.6.0.bin", "./build/bootloader-V1.6.0.bin"),
        ("partitions-V1.6.0.bin", "./build/partitions-V1.6.0.bin"),
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
