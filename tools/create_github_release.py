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

Mit Version **{version}** erhält CamperUI ein umfassendes Upgrade des Energie-Dashboards mit Mini-Arcs und dynamischer Flussanzeige, automatische Uhrzeitsynchronisation über VanPi `/info`, einen energiesparenden Nacht-Screensaver mit 1% Dimmung sowie Schaltersperren.

---

### 🚀 Highlights & Neuerungen

#### ⚡ 1. Überarbeitetes Energie-Dashboard & Produzenten-Arcs
* **Zentrales Batterie-Element:**
  * Großformatige Watt-Anzeige mit direkt danebenliegender Ampere-Anzeige (Lade-/Entladestrom) und Spannung.
  * Vergrößerter Ladezustands-Arc (SoC %) mit zentrierter Prozentanzeige.
  * Dynamische Flusslinien: Sämtliche Verbindungslinien und der Batterierahmen färben sich bei positivem Gesamtladestrom grün ein.
* **Visuelle Leistungs-Arcs für Produzenten:**
  * Solar, Landstrom (Shore) und Lichtmaschine (Alternator) verfügen nun über eigene farbige Mini-Arcs, die die aktuelle Leistung relativ zur konfigurierten Maximalleistung visualisieren.
  * Konfigurierbare Maximalwerte in den Einstellungen: `Solar Max Watt`, `Landstrom Max Watt`, `Lichtmaschine Max Watt`.
* **Flexible Produzentenauswahl:**
  * Nicht vorhandene Erzeuger können in den Display- und Web-Einstellungen deaktiviert werden. Die verbleibenden Karten zentrieren sich automatisch sauber auf dem Bildschirm.
* **Direkter Verlaufsaufruf (History Modal):**
  * Ein Fingertipp auf die Batterie- oder Solarkarte öffnet sofort das 24h-Verlaufsdiagramm.

#### 🕒 2. Automatische Uhrzeitsynchronisation via VanPi `/info`
* **Kein manuelles Einstellen mehr:**
  * Die Uhrzeit im Header oben links synchronisiert sich automatisch über den VanPi Endpoint `/info` (`current_date`).
  * Übernahme der Restlaufzeit (`ttgo` - Time-to-go) direkt aus der VanPi-Telemetrie.
  * Manuelle Zeiteinstellungen und NTP-Routinen wurden komplett entfernt, was wertvollen Flash-Speicher und RAM freigibt.

#### 🌙 3. Nacht-Screensaver (Ruhemodus mit 1% Helligkeit)
* **Augenschonende Nachtansicht:**
  * Nach konfigurierbarem Inaktivitäts-Timeout schaltet das Display auf einen schwarzen Bildschirm mit dunkelroter Digitaluhr (`#dc2626`) und dezenter Time-to-go Anzeige (`#b91c1c`).
  * Automatische Dimmung der Display-Hintergrundbeleuchtung auf minimal sichtbare 1% Helligkeit.
  * Sicheres Aufwecken bei Touch-Berührung ohne versehentliches Auslösen darunterliegender Schaltflächen.
  * Aktivierbar in den Display-Einstellungen sowie über das Webinterface.

#### 🔒 4. Schalter- & Relais-Sperre (Switch Locks)
* **Schutz vor versehentlichem Schalten:**
  * Einzelne Schalter und Relais können mit einer Sperrfunktion versehen werden (sichtbar durch Schloss-Symbol), um versehentliches Deaktivieren wichtiger Verbraucher wie Kühlschrank oder Wasserpumpe zu verhindern.

#### 🔥 5. Truma Combi Integration
* **Modellunterscheidung:**
  * Saubere Trennung zwischen `Truma (Gas/Diesel)` und `Truma + Elektro (Combi E)`. Elektrische Heizstufen und Landstrom-Hinweise werden bei reinen Gas/Diesel-Modellen automatisch ausgeblendet.

---

### 💾 Installation & Update

* **Web-OTA (Empfohlen):** Im Browser `http://<IP-des-Displays>/update` aufrufen und die Datei `CamperUI-{version}.bin` hochladen.
* **USB-Erstinstallation:** `CamperUI-{version}-merged.bin` mit dem ESP Flash Tool, Web-Flasher oder Arduino IDE ab Offset `0x0` flashen.
* **GitHub Pages Web-Flasher:** Direkt im Browser flashen.
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
