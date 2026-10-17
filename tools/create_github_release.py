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

Release **{version}** bringt umfangreiche Verbesserungen für das Energie-Dashboard, separate REST-Endpoints für Landstrom und Booster, einen neuen XXL-Nachtbildschirmschoner sowie flexible Verbraucher-Berechnungsoptionen.

---

### 🚀 Highlights & Neuerungen in {version}

#### ⚡ 1. Energie-Dashboard & Erzeuger-Verrechnung
* **Neue Option „Verbrauch mit Erzeugern verrechnen“:**
  * Wählbar zwischen **Gesamtlast** ($P_\\text{{load}} = P_\\text{{gen}} - P_\\text{{bat}}$) und **reiner Batterie-Entnahme** (nur was tatsächlich aus `/batt` fließt).
  * Umschaltbar sowohl direkt am Display (*Einstellungen → Sichtbarkeit*) als auch über das Webinterface.
* **Separate REST-Endpoints (`/shore` & `/booster`):**
  * Volle Unterstützung für modulare Node-RED-Endpoints für 230V Landstrom und B2B-Ladebooster inkl. mitgeliefertem Flow und Anleitung.
* **Eindeutiger `Off`-Zustand:**
  * Ist ein Erzeuger ausgeschaltet oder getrennt, zeigt das Display nun deutlich **`Off`** in dezentem Grau anstelle von missverständlichem `0 W`.
  * Starterbatteriespannung bleibt auch bei inaktivem Booster sichtbar.
* **Automatische Erkennung & Ausgrauen:**
  * Werden Endpoints nicht gefunden (404), werden die Toggles in den Einstellungen automatisch ausgegraut (mit 30s Hysterese gegen Netzwerk-Aussetzer).
* **Reine VanPi Restzeit (`ttgo`):**
  * Keine fehlerhaften internen Restzeit-Schätzungen mehr – das Display nutzt direkt das Shunt-Feld `ttgo` aus `/info`.

#### 🌙 2. XXL Nacht-Bildschirmschoner
* **120-Pixel Uhrzeitanzeige:** Übergroße Ziffern für perfekte Lesbarkeit bei Nacht aus jeder Position im Camper.
* **Große Statuszeilen:** Restzeit (Time to go) und Batteriespannung/SoC in 28px bzw. 20px, farblich passend in Rot (`#dc2626`).
* **Eigene Nacht-Helligkeit:** Die Dimm-Stufe des Screensavers kann separat in den Display- und Web-Einstellungen festgelegt werden (Default 3% für blendfreies Schlafen).

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
