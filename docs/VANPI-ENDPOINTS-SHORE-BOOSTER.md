# CamperUI: VanPi REST-Endpoints für Landstrom & Ladebooster (Option B)

Diese Anleitung beschreibt die Bereitstellung und Anbindung der dedizierten REST-Endpoints **`GET /shore`** (Landstrom) und **`GET /booster`** (Ladebooster / Lichtmaschine) auf dem VanPi Raspberry Pi (Node-RED) für das CamperUI Display.

---

## 1. Übersicht & Funktionsweise

In CamperUI werden Energie-Produzenten getrennt erfasst und im **Energie-Dashboard** visualisiert:
- **Solar**: MPPT-Endpoint `/mppt/`
- **Landstrom (230V)**: Neuer REST-Endpoint `GET /shore`
- **Ladebooster / Lichtmaschine**: Neuer REST-Endpoint `GET /booster`

### Automatisches Abschalten bei Nicht-Erreichbarkeit
CamperUI fragt beide Endpoints über fehlertolerante Abfragen ab (`fetch_optional_endpoint`):
- Ist der Endpoint **erreichbar (HTTP 200)**: Die Werte für Watt, Volt, Ampere und Status werden live dargestellt.
- Ist der Endpoint **nicht erreichbar (z.B. HTTP 404, Flow pausiert, Timeout)**:
  - CamperUI schaltet die Elemente automatisch **ab** (`0 W`, inaktive Flusslinie, "Getrennt" bzw. "Aus").
  - Es entsteht **kein Verbindungsabbruch** zu den übrigen VanPi-Diensten (Batterie, Relais, Dimmer laufen ungestört weiter).
- Über die Display-Einstellungen oder die CamperUI Web-Oberfläche können Landstrom und Booster zudem jederzeit vollständig ausgeblendet werden.

---

## 2. JSON-Spezifikation der Endpoints

### `GET http://<vanpi-ip>:1880/shore`
Gibt den aktuellen Status der 230V-Landstrom-Einspeisung zurück:

```json
{
  "connected": true,
  "power": 450.0,
  "voltage": 230.0,
  "current": 1.95
}
```

| Feld | Typ | Beschreibung |
|---|---|---|
| `connected` | Boolean | `true`, wenn 230V Landstrom anliegt, sonst `false`. (Wird bei `power > 5W` automatisch als `true` interpretiert). |
| `power` | Number (Float) | Aktuelle Leistung in **Watt** (W). |
| `voltage` | Number (Float) | Netzspannung in **Volt** (V), typisch `230.0`. |
| `current` | Number (Float) | Stromstärke in **Ampere** (A). Kann weggelassen werden (wird dann aus `power / voltage` errechnet). |

---

### `GET http://<vanpi-ip>:1880/booster`
Gibt den aktuellen Status des B2B-Ladeboosters bzw. der Lichtmaschine zurück:

```json
{
  "active": true,
  "power": 350.0,
  "voltage": 14.2,
  "current": 24.6
}
```

| Feld | Typ | Beschreibung |
|---|---|---|
| `active` | Boolean | `true`, wenn der Booster lädt oder die Lichtmaschine aktiv ist. |
| `power` | Number (Float) | Ladeleistung in **Watt** (W). |
| `voltage` | Number (Float) | Spannung in **Volt** (V) – z.B. Starterbatterie- oder Ladespannung. |
| `current` | Number (Float) | Ladestrom in **Ampere** (A) zur Aufbaubatterie. |

---

## 3. Node-RED Flow importieren

Ein vollständiger, einsatzbereiter Flow ist in CamperUI hinterlegt unter:
📁 [`docs/flows/vanpi_energy_producers_flow.json`](file:///c:/Users/njest/Documents/projects/CamperUI/docs/flows/vanpi_energy_producers_flow.json)

### Schritt-für-Schritt Installation in VanPi:

1. Öffne die Node-RED Oberfläche deines VanPi im Browser:  
   `http://<vanpi-ip>:1880`
2. Klicke oben rechts auf das **Burger-Menü (≡)** -> **Import** (oder drücke `Strg + I`).
3. Wähle den Reiter **Clipboard** und füge den Inhalt der Datei [`vanpi_energy_producers_flow.json`](file:///c:/Users/njest/Documents/projects/CamperUI/docs/flows/vanpi_energy_producers_flow.json) ein.
4. Klicke auf **Import**. Ein neuer Flow-Tab namens **"CamperUI - Energie Endpoints"** wird angelegt.
5. Klicke oben rechts auf den roten Button **Deploy**, um die neuen Endpoints zu aktivieren.

Test im Browser:
- Rufe `http://<vanpi-ip>:1880/shore` auf -> Es sollte `{"connected":false,"power":0,"voltage":0,"current":0}` zurückkommen.
- Rufe `http://<vanpi-ip>:1880/booster` auf -> Es sollte `{"active":false,"power":0,"voltage":0,"current":0}` zurückkommen.

---

## 4. Daten einspeisen (Live-Anbindung)

Der importierte Flow liest die Daten aus globalen Node-RED Variablen (`global.get(...)`). Du kannst deine realen Messgeräte auf verschiedene Arten anbinden:

### Variante A: In bestehenden Flows per Function-Node

Wenn du in einem bestehenden Flow (z.B. Victron, Shunt, Shelly, Tasmota) Daten empfängst, füge einfach folgende Zeilen in deine bestehende Function-Node ein:

#### Für Landstrom:
```javascript
// Beispiel: Daten von 230V Shunt, Shelly Plug oder Wechselrichter/Netzlader
global.set("shore_connected", msg.payload.power > 10.0);
global.set("shore_power", Number(msg.payload.power));       // Watt
global.set("shore_voltage", Number(msg.payload.voltage));   // Volt (z.B. 230)
global.set("shore_current", Number(msg.payload.current));   // Ampere
return msg;
```

#### Für Ladebooster / Lichtmaschine:
```javascript
// Beispiel: Daten vom Votronic / Victron Orion-Tr / D+ Signal
global.set("booster_active", msg.payload.power > 10.0 || msg.payload.dplus == 1);
global.set("booster_power", Number(msg.payload.power));     // Watt
global.set("booster_voltage", Number(msg.payload.starter)); // Starterbatterie-Volt (z.B. 14.2)
global.set("booster_current", Number(msg.payload.current)); // Ladestrom Ampere
return msg;
```

---

### Variante B: Shelly Plug / Shelly Plus 1PM (Landstrom-Erkennung)

Wenn du einen Shelly an der 230V-Einspeisung hast:
1. Füge eine **MQTT in** Node hinzu: Topic z.B. `shellies/shellyplug-shore/relay/0/power` oder `shellyplus1pm/status/switch:0`.
2. Verbinde sie mit der mitgelieferten Vorlage-Node **"Beispiel: Shelly Plug / 230V Messung -> Shore"** im importierten Flow.
3. Die Leistung und der Verbindungsstatus werden sofort automatisch aktualisiert.

---

### Variante C: D+ Signal über VanPi GPIO (Ladebooster)

Wenn du das D+ Signal der Lichtmaschine an einen digitalen Eingang des Raspberry Pi / VanPi angeschlossen hast:
1. Nutze die VanPi **rpi-gpio in** Node für den entsprechenden Pin.
2. Wenn Pin = 1 (Lichtmaschine läuft):
   ```javascript
   global.set("booster_active", true);
   global.set("booster_voltage", 14.2); // Typische Lima-Spannung
   ```
3. Wenn Pin = 0 (Motor aus):
   ```javascript
   global.set("booster_active", false);
   global.set("booster_power", 0);
   global.set("booster_current", 0);
   ```

---

### Variante D: Sofortiges Testen im Flow (Inject-Buttons)

Der Flow enthält vier vordefinierte Test-Buttons:
- **"Landstrom AN (450W)"**: Simuliert 450W Landstrom -> Display zeigt sofort 450W, 230V, blaue/grüne Flusslinie.
- **"Landstrom AUS"**: Schaltet Landstrom ab -> Display zeigt "0 W", "Getrennt", graue Flusslinie.
- **"Booster AN (350W)"**: Simuliert 350W Booster -> Display zeigt 350W, violette/grüne Flusslinie.
- **"Booster AUS"**: Schaltet Booster ab -> Display zeigt "0 W", "Aus", graue Flusslinie.

Klicke einfach auf den linken Button der Inject-Node in Node-RED, um den jeweiligen Zustand sofort auszulösen!

