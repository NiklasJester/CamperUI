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

    tag = "V1.5.0"
    title = "CamperUI_V1.5.0"
    body = """# 🚐 CamperUI Release V1.5.0

Mit Version **V1.5.0** bringt CamperUI die vollständige Integration der **Truma Combi (inkl. CP Plus)** Heiz- und Warmwassersysteme, ein überarbeitetes modulares Heizungs-UI, verbesserte Navigationsleisten-Gesten sowie komfortables Flashen via Improv Wi-Fi im Web Flasher.

---

### 🚀 Highlights & Neuerungen

#### 🔥 1. Vollständige Truma Combi & CP Plus Integration
* **Dedizierter Truma-Tab:** Sobald in den Einstellungen als Heizungstyp `Truma (Combi/VarioHeat)` ausgewählt ist, wird anstelle des Autoterm-Klima-Tabs der neue, maßgeschneiderte Truma-Bedientab aktiviert.
* **Raumheizungs-Steuerung:** 
  * Großer Start/Stop-Schalter zum Ein- und Ausschalten der Heizung.
  * Taktile `[-]` und `[+]` Stepper-Tasten für die Zieltemperatur (5 bis 30 °C).
  * 4 Schnellwahltasten für gängige Temperaturen (`18 °C`, `20 °C`, `22 °C`, `24 °C`).
* **Warmwasser-Boiler:**
  * 3-Stufen-Auswahl für den Warmwasserbetrieb (`Aus`, `Eco 40 °C`, `Hot 60 °C`) mit direkter optischer Rückmeldung des aktiven Status.
* **Energiequelle & Energiemix:**
  * Reine Brennstoffnutzung (`Gas / Diesel`).
  * Kombibetrieb mit 230V Elektro-Heizstab (`Mix 1 (900W)` und `Mix 2 (1800W)`).
  * Rein elektrischer Betrieb (`Elektro 1 (900W)` und `Elektro 2 (1800W)`).
  * Automatischer Warnhinweis, falls Elektro-/Mix-Modus ohne angeschlossenen 230V Landstrom gewählt wird.
* **Diagnostik & Status-Header:**
  * Modellanzeige (z. B. `Truma Combi D 4`).
  * Aktuelle Raumtemperatur aus VanPi.
  * Badges für Online/Offline-Status, 230V Landstrom (`⚡ 230V`) und Fehlercodes bei aktiven Störungen.
* **Nicht-blockierende Anbindung:**
  * Steuerbefehle werden über eine asynchrone HTTP-Queue direkt an Node-RED gesendet (`/truma_ci/air/:temp`, `/truma_ci/water/:mode`, `/truma_ci/energymix/:val`).
  * 4-Sekunden-Hold-Timer schützt vor dem Überschreiben lokaler Eingaben durch zeitgleiche HTTP-Polls.

#### ⏱️ 2. Verbesserter Settings-Notfallzugriff (5-Sekunden-Hold in der Tableiste)
* **Zuverlässige Geste:** Da das Halten der kleinen Uhrzeit auf manchen Touch-Displays unhandlich war, lässt sich das Einstellungsmenü bei ausgeblendetem Settings-Tab nun durch ein **5 Sekunden langes Gedrückthalten an beliebiger Stelle in der unteren Tableiste** öffnen.
* Akustische Bestätigung durch den Touch-Buzzer bei Aktivierung.

#### ⚡ 3. Web Flasher: Improv Wi-Fi & direkte Navigation
* **Direktes WLAN-Setup nach dem Flashen:** Der browserbasierte Web Flasher unterstützt nun das Improv Wi-Fi Serial-Protokoll, sodass nach dem Flashen direkt die Zugangsdaten übermittelt werden können.
* **Auto-Navigation:** Nach erfolgreicher WLAN-Verbindung ermittelt der Web Flasher die zugewiesene IP-Adresse und bietet direkte Links zum CamperUI Webinterface.

#### 🌐 4. Webinterface & Tab-Management Sync
* Im Webinterface (`/settings`) werden der `Truma`-Tab sowie der `Einstellungen`-Tab im Tab-Sortiermenü vollständig unterstützt.
* Die Umschaltung des Heizungstyps synchronisiert die Tab-Aktivierung automatisch.

---

### 💾 Installation & Update

* **Web-OTA (Empfohlen):** Im Browser `http://<IP-des-Displays>/update` aufrufen und die Datei `CamperUI-V1.5.0.bin` hochladen.
* **USB-Erstinstallation:** `CamperUI-V1.5.0-merged.bin` mit dem ESP Flash Tool, Web-Flasher oder Arduino IDE ab Offset `0x0` flashen.
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
        ("CamperUI-V1.5.0.bin", "./build/CamperUI-V1.5.0.bin"),
        ("CamperUI-V1.5.0-merged.bin", "./build/CamperUI-V1.5.0-merged.bin"),
        ("bootloader-V1.5.0.bin", "./build/bootloader-V1.5.0.bin"),
        ("partitions-V1.5.0.bin", "./build/partitions-V1.5.0.bin"),
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
