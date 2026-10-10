# VanPi: HTTP-Datenkatalog für CamperUI

Stand: 06.10.2026, CamperUI V8.7. Grundlage: die mitgelieferte `docs/flows.json` und `http_handler.cpp`.
Dies ist ein Katalog dieses Flow-Exports, keine Garantie für jede VanPi-Version oder deine aktive Installation.
Es wurden keine Geräte abgefragt, umgeschaltet oder Node-RED-Funktionen ausgeführt.

Basisadresse: `http://<VanPi-IP>:1880`. Die unten angegebenen Wege werden daran angehängt.
HTTP liefert JSON, sofern die entsprechende Integration aktiv ist. Einzelne Felder können fehlen,
Ersatzwerte enthalten oder als String statt Zahl kommen. Ein im Flow registrierter Weg bedeutet nicht,
dass das dazugehörige Gerät angeschlossen ist. Interne Node-RED-Variablen sind nicht automatisch per HTTP verfügbar.

## Übersicht

| Bereich | Abfrage | CamperUI V8.7 |
|---|---|---|
| Batterie | `/batt` | Spannung, Strom, SOC |
| Solar | `/mppt/` | Spannung, Strom, Leistung; Gesamtwert noch ungenutzt |
| Temperaturen | `/temp` | Nur temp1–4; Dimmy, NTC und Ruuvi noch ungenutzt |
| Tanks | `/level` | level1–4 |
| Relais | `/relay/`, `/relay/:input` | Nur Relay1–8 und deren state/name |
| WLAN-Relais | `/wrelay/`, `/wrelay/:input` | Noch ungenutzt |
| Dimmer | `/dimmer/`, `/dimmer/:input` | Nur dimmer1–8 und deren state/name |
| Namen | `/names` | Kein regulärer Poll; Namen kommen aus den anderen Antworten |
| BMS | `/bms` | Noch ungenutzt |
| Heizung | `/heater` | Ausgewählte generische/Autoterm-1-Werte |
| Lüfter | `/fan/`, `/maxxfan/`, `/bayernluefter/` | Noch ungenutzt; MaxxFan-Seite ist lokal |
| Boiler | `/boiler/`, `/smartboil` | Noch ungenutzt |
| Kühlschrank | `/dometic_rc10_ci/` | Noch ungenutzt |
| Timberline | `/timberline` | Noch ungenutzt |
| Wasserwaage | `/position_sensor/?request=true` | x_angle/y_angle |
| GPS | `/gps` | Noch ungenutzt |
| System | `/info/`, `/debug`, `/network`, `/conniot` | Noch ungenutzt |
| LTE | `/lte_bridge` | Noch ungenutzt |
| Zusammengefasste Home-Auswahl | `/app_home/` mit Query-Parametern | Noch ungenutzt |
| IoT-Varianten | `/relayiot/`, `/wrelayiot/` | Noch ungenutzt |

Die Feldliste `VANPI-HTTP-FELDER.csv` enthält Namen, Einheiten soweit belegbar,
interne Quellen und den Nutzungsstatus im Display. Geräteobjekte mit dynamischem Schema
sind ausdrücklich markiert; sie benötigen einen Abgleich mit einer realen Antwort.

## Wichtige Ergebnisse für die weitere Entwicklung

- MaxxFan-Live-Daten sind im Export vorhanden: `maxxfan.fan_power`, `fan_direction`,
  `fan_temp`, `fan_auto`, `fan_speed`, `fan_vent` über `/maxxfan/` oder `/fan/`.
- `/temp` kann mehr als vier Sensoren liefern: dimmytemp1–2, ntc1–4 und ruuvitag0–2.
  Ruuvi enthält zusätzlich `hum`, `batt` (mV) und `lowBatt`. Die Heizungsantwort kann
  weitere Ruuvi-Indizes abhängig von `ruuvitag_max_index` enthalten.
- Der Live-Flow definiert temp3 als Temperatursensor. Nur unser Dummy macht daraus
  Feuchte. Die bisherige generische Klima-Auswertung im Display nimmt teilweise
  Sensor 3 als Feuchte an; das ist für echte DS18B20-Daten zu überprüfen.
- Relais-Erweiterungen ermöglichen Relay9–16; zusätzliche Custom-Kanäle können folgen.
  Dimmer umfassen im Handler 7 Basis- oder 15 Pro-Hardware-Kanäle,
  plus Custom/RGBW-Zweige. CamperUI unterstützt bisher nur acht pro Gruppe.
- Starterspannung steht nicht im mitgelieferten `/batt`-Handler. `starter_voltage`
  ist bisher unsere optionale Erweiterung. Sie darf nicht als vorhandene Standard-API gelten.
- BMS und MPPT liefern mehr Werte als CamperUI nutzt. Die Einheit von `mppt_pv_total`
  ist im HTTP-Handler nicht festgelegt; nicht ungeprüft als Wh/kWh beschriften.
- Manche Handler setzen fehlende Daten auf 0 oder Defaults. HTTP 200 und ein Zahlenwert
  beweisen daher weder Sensorverbindung noch Aktualität. Empfang und Gerätegesundheit
  müssen bei der Live-Anbindung getrennt beurteilt werden.

## MaxxFan-Befehle im mitgelieferten Flow

Diese Einträge dokumentieren vorhandene Steuerwege; sie wurden nicht ausgeführt.

| Methode/Weg | Verhalten |
|---|---|
| PUT `/maxxfan/power` | Ein/Aus umschalten (Toggle, kein absolutes Setzen) |
| PUT `/maxxfan/direction` | Richtung umschalten |
| PUT `/maxxfan/vent` | Klappe umschalten; im Auto-Modus HTTP 403 |
| PUT `/maxxfan/auto` | Automatik umschalten |
| PUT `/maxxfan/speed/:value` | Stufe 1–10; nur eingeschaltet und ohne Auto; schrittweise Befehle |
| PUT `/maxxfan/temp/:value` | Sollwert −2 bis 37; nur mit Auto; schrittweise Befehle |

HTTP 200 bestätigt hier teilweise nur die Annahme des Befehls, nicht den erreichten Zustand.
Nach einer Steueraktion erneut Status lesen; Toggle-Befehle nicht blind wiederholen.

## Alle registrierten HTTP-Wege

Die folgende Liste und `VANPI-HTTP-ENDPUNKTE.csv` enthalten jeden HTTP-in-Knoten des Exports,
einschließlich Administration, Upload und Steuerung. `:name` ist ein Parameter, `?` am
Parameter macht ihn optional. Pfade sind mit führendem Slash normalisiert; End-Slashes
werden beibehalten. Die Quell-Node-ID ermöglicht die Zuordnung in Node-RED.

**Auch GET kann Aktionen auslösen:** `/reboot` startet neu, und
`/position_sensor/?request=calibrate` kalibriert. Upload-/Backup-/Netzwerkänderungswege
sind keine Sensorabfragen. Dieser Katalog darf nicht als automatisch abzufragende URL-Liste benutzt werden.

| Methode | Weg | Quell-Node-ID | Flow deaktiviert? |
|---|---|---|---|
| GET | `/batt` | `d88508e9.959f68` | Nein |
| GET | `/level` | `ff26b15d.781d2` | Nein |
| GET | `/names` | `782c51e6.7fe59` | Nein |
| GET | `/heater` | `f43c42b0.3680f` | Nein |
| GET | `/temp` | `9ddd5b7.518bea8` | Nein |
| PUT | `/relay/:input/:value` | `01c1b0774a7ce1c5` | Nein |
| GET | `/relay/:input` | `857bd591a72db827` | Nein |
| PUT | `/toggle/relay/:input` | `86bc478d69c132c1` | Nein |
| GET | `/relay/` | `94700921f269acea` | Nein |
| PUT | `/names/:input` | `10071c4e6ed27059` | Nein |
| GET | `/wrelay/:input` | `a454928c0a2e740d` | Nein |
| PUT | `/toggle/wrelay/:input` | `ac3c4fe5cd58e530` | Nein |
| GET | `/wrelay/` | `5aec9fd67fb417a3` | Nein |
| PUT | `/dimmer/:input/:value` | `04e90dc1676dd13a` | Nein |
| GET | `/dimmer/:input` | `53e6240e6b18b304` | Nein |
| GET | `/dimmer/` | `93928c757eed6cfd` | Nein |
| PUT | `/wrelay/:input/:value` | `2f5ffaba89fd012e` | Nein |
| PUT | `/names/:input/:value` | `4ce25de51b4fb126` | Nein |
| PUT | `/heater/:truefalse` | `aadd4382540212d4` | Nein |
| PUT | `/heater/:truefalse/:temp` | `8cc4bb4336555b86` | Nein |
| PUT | `/heater/:truefalse/:temp/:time` | `ce11f3c7edd3025d` | Nein |
| PUT | `/names/:device/:input/:value` | `4bdada802e6896da` | Nein |
| GET | `/bms` | `dfe6ac35b535e163` | Nein |
| GET | `/conniot` | `fd04752f42acafc2` | Nein |
| GET | `/wrelayiot/` | `890d1f2042565f42` | Nein |
| PUT | `/wrelayiot/:input/:value` | `ab37cf757e8ab171` | Nein |
| GET | `/reboot` | `4b522f61da35eaa9` | Nein |
| GET | `/relayiot/` | `e0d0fb4aa408cd1e` | Nein |
| GET | `/debug` | `a2480738462df03d` | Nein |
| PUT | `/attime` | `d6896f1d6e9fe09d` | Nein |
| PUT | `/switchall/:truefalse` | `542a2c9f0ca623ea` | Nein |
| GET | `/info/` | `af04e125a02e7e81` | Nein |
| PUT | `/reset_wifi_ap/:input` | `42e02e39624a5cb2` | Nein |
| GET | `/network` | `c3e884d6b59ebf48` | Nein |
| PUT | `/update_wifi_ap/:ssid/:wpa` | `049e2d2e9ab3c402` | Nein |
| PUT | `/activate_wifi_ap/:input` | `badd957725934738` | Nein |
| PUT | `/autoterm/:ventilation_heatingpower/:value` | `18248bd261f66d64` | Ja |
| GET | `/gps` | `be4eb9d57f984b56` | Nein |
| GET | `/app_home/` | `85b194725e15adc0` | Nein |
| GET | `/position_sensor/` | `65e561c5aa1b380a` | Nein |
| GET | `/mppt/` | `dd28e3dc8c844555` | Nein |
| PUT | `/switchallSelected/:truefalse` | `925aec786f7ead39` | Nein |
| PUT | `/autoterm/:mode/:value` | `35fa50255dfb5421` | Nein |
| PUT | `/autoterm2/:mode/:value` | `f78b61aa241a03bb` | Nein |
| PUT | `/timers/:device` | `268ab5b68106b017` | Nein |
| GET | `/fan/` | `4374ca7e1e9c706a` | Nein |
| PUT | `/maxxfan/:type/:value?` | `be04e0436d12af78` | Nein |
| PUT | `/bayernluefter/:type/:value?` | `d9c34f2b4faeb4df` | Nein |
| GET | `/maxxfan/` | `608408a294bf58e1` | Nein |
| GET | `/bayernluefter/` | `38e7e5fa3621bb89` | Nein |
| GET | `/smartboil` | `110f907da14b2626` | Nein |
| GET | `/dometic_rc10_ci/` | `278b62cae74991c6` | Nein |
| PUT | `/rgbw/:input` | `36ece5734829534f` | Nein |
| PUT | `/timberline` | `44b4b854feaa0e56` | Nein |
| GET | `/lte_bridge` | `46b380cf93ad07b8` | Nein |
| PUT | `/smartboil/:command/:value?` | `7aec9024dd3b7522` | Nein |
| PUT | `/truma_ci/:device/:value?` | `8c55fdeaf758da4c` | Nein |
| PUT | `/boiler/:auto/:target/:boost?` | `902ef1d4ea5920d6` | Nein |
| GET | `/boiler/` | `93d429b036b5fc65` | Nein |
| POST | `/custom/smartboil/:command/:value?` | `9b75bd5a3e21e04b` | Nein |
| PUT | `/dometic_rc10_ci/:power/:mode/:step?` | `9cf5ef7574f5f108` | Nein |
| GET | `/timberline` | `a29a663dbd52473c` | Nein |
| POST | `/lte_bridge_data` | `c21c219c5dd1e023` | Nein |
| GET | `/uploadvanimg` | `68e58c0264f01092` | Nein |
| POST | `/uploadedvanimg` | `f1cf0386f93ec839` | Nein |
| GET | `/uploadtft` | `41799cb843b00d18` | Nein |
| POST | `/uploadedtft` | `cecce3b3952eb184` | Nein |
| GET | `/userdata_pekaway.zip` | `2e9dbf3e91bf8eef` | Nein |
| GET | `/restoreuserdata` | `9d39f181599b0a09` | Nein |
| POST | `/upload` | `b77abd07bd8c7701` | Nein |
| POST | `/capture-url` | `fb4b91b9.f38cf` | Nein |

## Felder je Abfrage

Einheiten und Nutzungsstatus beziehen sich auf den untersuchten Stand. „Teilweise“ bedeutet, dass nur bestimmte Zweige/Felder ausgewertet werden.

### `/batt`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `VoltB` | Batteriespannung (V) | Ja | MainBattVolt |
| `Ampere` | Batteriestrom (A) | Ja | MainBattAmps |
| `battsoc` | Ladezustand (%) | Ja | MainBattSoc |
| `starter_voltage` | Starterspannung (V) | Optional | CamperUI-Erweiterung/Dummy; NICHT im mitgelieferten /batt-Flow vorhanden. |

### `/mppt/`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `mppt_pv_amps` | PV-Strom (A) | Ja | mppt_pv_amps |
| `mppt_pv_volts` | PV-Spannung (V) | Ja | mppt_pv_volts |
| `mppt_pv_watts` | PV-Leistung (W) | Ja | mppt_pv_watts |
| `mppt_pv_total` | PV-Gesamtwert (Nicht im HTTP-Handler definiert) | Nein | mppt_pv_total |

### `/temp`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `temp1.state` | Temperatursensor 1: Messwert/Zustand (°C) | Ja |  |
| `temp1.name` | Temperatursensor 1: Name (Text) | Ja |  |
| `temp1.type` | Sensortyp (Text) | Nein | Hier ds18b20; der Flow liefert Temperatur, keine fest zugewiesene Feuchte. |
| `temp2.state` | Temperatursensor 2: Messwert/Zustand (°C) | Ja |  |
| `temp2.name` | Temperatursensor 2: Name (Text) | Ja |  |
| `temp2.type` | Sensortyp (Text) | Nein | Hier ds18b20; der Flow liefert Temperatur, keine fest zugewiesene Feuchte. |
| `temp3.state` | Temperatursensor 3: Messwert/Zustand (°C) | Ja |  |
| `temp3.name` | Temperatursensor 3: Name (Text) | Ja |  |
| `temp3.type` | Sensortyp (Text) | Nein | Hier ds18b20; der Flow liefert Temperatur, keine fest zugewiesene Feuchte. |
| `temp4.state` | Temperatursensor 4: Messwert/Zustand (°C) | Ja |  |
| `temp4.name` | Temperatursensor 4: Name (Text) | Ja |  |
| `temp4.type` | Sensortyp (Text) | Nein | Hier ds18b20; der Flow liefert Temperatur, keine fest zugewiesene Feuchte. |
| `dimmytemp1.state` | Temperatur (°C) | Nein | dimmytemp=true |
| `dimmytemp1.name` | Sensorname (Text) | Nein | dimmytemp=true |
| `dimmytemp1.type` | Sensortyp (Text) | Nein | dimmytemp=true |
| `dimmytemp2.state` | Temperatur (°C) | Nein | dimmytemp=true |
| `dimmytemp2.name` | Sensorname (Text) | Nein | dimmytemp=true |
| `dimmytemp2.type` | Sensortyp (Text) | Nein | dimmytemp=true |
| `ntc1.state` | Temperatur (°C) | Nein | ntcOverwrites.ntcN=true |
| `ntc1.name` | Sensorname (Text) | Nein | ntcOverwrites.ntcN=true |
| `ntc1.type` | Sensortyp (Text) | Nein | ntcOverwrites.ntcN=true |
| `ntc2.state` | Temperatur (°C) | Nein | ntcOverwrites.ntcN=true |
| `ntc2.name` | Sensorname (Text) | Nein | ntcOverwrites.ntcN=true |
| `ntc2.type` | Sensortyp (Text) | Nein | ntcOverwrites.ntcN=true |
| `ntc3.state` | Temperatur (°C) | Nein | ntcOverwrites.ntcN=true |
| `ntc3.name` | Sensorname (Text) | Nein | ntcOverwrites.ntcN=true |
| `ntc3.type` | Sensortyp (Text) | Nein | ntcOverwrites.ntcN=true |
| `ntc4.state` | Temperatur (°C) | Nein | ntcOverwrites.ntcN=true |
| `ntc4.name` | Sensorname (Text) | Nein | ntcOverwrites.ntcN=true |
| `ntc4.type` | Sensortyp (Text) | Nein | ntcOverwrites.ntcN=true |
| `ruuvitag0.state` | Temperatur (°C) | Nein | ruuvitags=true |
| `ruuvitag0.name` | Sensorname (Text) | Nein | ruuvitags=true |
| `ruuvitag0.type` | Sensortyp (Text) | Nein | ruuvitags=true |
| `ruuvitag0.hum` | Relative Feuchte (%) | Nein | ruuvitags=true |
| `ruuvitag0.batt` | Sensorbatterie (mV) | Nein | ruuvitags=true |
| `ruuvitag0.lowBatt` | Batteriewarnung (bool) | Nein | ruuvitags=true |
| `ruuvitag1.state` | Temperatur (°C) | Nein | ruuvitags=true |
| `ruuvitag1.name` | Sensorname (Text) | Nein | ruuvitags=true |
| `ruuvitag1.type` | Sensortyp (Text) | Nein | ruuvitags=true |
| `ruuvitag1.hum` | Relative Feuchte (%) | Nein | ruuvitags=true |
| `ruuvitag1.batt` | Sensorbatterie (mV) | Nein | ruuvitags=true |
| `ruuvitag1.lowBatt` | Batteriewarnung (bool) | Nein | ruuvitags=true |
| `ruuvitag2.state` | Temperatur (°C) | Nein | ruuvitags=true |
| `ruuvitag2.name` | Sensorname (Text) | Nein | ruuvitags=true |
| `ruuvitag2.type` | Sensortyp (Text) | Nein | ruuvitags=true |
| `ruuvitag2.hum` | Relative Feuchte (%) | Nein | ruuvitags=true |
| `ruuvitag2.batt` | Sensorbatterie (mV) | Nein | ruuvitags=true |
| `ruuvitag2.lowBatt` | Batteriewarnung (bool) | Nein | ruuvitags=true |

### `/level`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `level1.state` | Tank 1: Messwert/Zustand (%) | Ja |  |
| `level1.name` | Tank 1: Name (Text) | Ja |  |
| `level2.state` | Tank 2: Messwert/Zustand (%) | Ja |  |
| `level2.name` | Tank 2: Name (Text) | Ja |  |
| `level3.state` | Tank 3: Messwert/Zustand (%) | Ja |  |
| `level3.name` | Tank 3: Name (Text) | Ja |  |
| `level4.state` | Tank 4: Messwert/Zustand (%) | Ja |  |
| `level4.name` | Tank 4: Name (Text) | Ja |  |

### `/relay/`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `Relay1.state` | Relais 1: Messwert/Zustand (bool) | Ja |  |
| `Relay1.name` | Relais 1: Name (Text) | Ja |  |
| `Relay2.state` | Relais 2: Messwert/Zustand (bool) | Ja |  |
| `Relay2.name` | Relais 2: Name (Text) | Ja |  |
| `Relay3.state` | Relais 3: Messwert/Zustand (bool) | Ja |  |
| `Relay3.name` | Relais 3: Name (Text) | Ja |  |
| `Relay4.state` | Relais 4: Messwert/Zustand (bool) | Ja |  |
| `Relay4.name` | Relais 4: Name (Text) | Ja |  |
| `Relay5.state` | Relais 5: Messwert/Zustand (bool) | Ja |  |
| `Relay5.name` | Relais 5: Name (Text) | Ja |  |
| `Relay6.state` | Relais 6: Messwert/Zustand (bool) | Ja |  |
| `Relay6.name` | Relais 6: Name (Text) | Ja |  |
| `Relay7.state` | Relais 7: Messwert/Zustand (bool) | Ja |  |
| `Relay7.name` | Relais 7: Name (Text) | Ja |  |
| `Relay8.state` | Relais 8: Messwert/Zustand (bool) | Ja |  |
| `Relay8.name` | Relais 8: Name (Text) | Ja |  |
| `Relay1–N.autooff` | Zusatzinformation zum Kanal (Im Handler prüfen) | Nein | Nur falls im jeweiligen Hardware-/Custom-Zweig erzeugt; kein einheitliches Schema für alle Kanäle. |
| `Relay1–N.offtime` | Zusatzinformation zum Kanal (Im Handler prüfen) | Nein | Nur falls im jeweiligen Hardware-/Custom-Zweig erzeugt; kein einheitliches Schema für alle Kanäle. |
| `Relay1–N.source` | Zusatzinformation zum Kanal (Im Handler prüfen) | Nein | Nur falls im jeweiligen Hardware-/Custom-Zweig erzeugt; kein einheitliches Schema für alle Kanäle. |
| `Relay9.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay9.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay10.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay10.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay11.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay11.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay12.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay12.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay13.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay13.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay14.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay14.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay15.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay15.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay16.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `Relay16.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |

### `/dimmer/`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `dimmer1.state` | Dimmer 1: Messwert/Zustand (0–100 %) | Ja |  |
| `dimmer1.name` | Dimmer 1: Name (Text) | Ja |  |
| `dimmer2.state` | Dimmer 2: Messwert/Zustand (0–100 %) | Ja |  |
| `dimmer2.name` | Dimmer 2: Name (Text) | Ja |  |
| `dimmer3.state` | Dimmer 3: Messwert/Zustand (0–100 %) | Ja |  |
| `dimmer3.name` | Dimmer 3: Name (Text) | Ja |  |
| `dimmer4.state` | Dimmer 4: Messwert/Zustand (0–100 %) | Ja |  |
| `dimmer4.name` | Dimmer 4: Name (Text) | Ja |  |
| `dimmer5.state` | Dimmer 5: Messwert/Zustand (0–100 %) | Ja |  |
| `dimmer5.name` | Dimmer 5: Name (Text) | Ja |  |
| `dimmer6.state` | Dimmer 6: Messwert/Zustand (0–100 %) | Ja |  |
| `dimmer6.name` | Dimmer 6: Name (Text) | Ja |  |
| `dimmer7.state` | Dimmer 7: Messwert/Zustand (0–100 %) | Ja |  |
| `dimmer7.name` | Dimmer 7: Name (Text) | Ja |  |
| `dimmer8.state` | Dimmer 8: Messwert/Zustand (0–100 %) | Ja | Basishardware liefert 7 Dimmer; Kanal 8 gehört zum optionalen Pro-Zweig mit 15 Kanälen. |
| `dimmer8.name` | Dimmer 8: Name (Text) | Ja | Basishardware liefert 7 Dimmer; Kanal 8 gehört zum optionalen Pro-Zweig mit 15 Kanälen. |
| `dimmer1–N.autooff` | Zusatzinformation zum Kanal (Im Handler prüfen) | Nein | Nur falls im jeweiligen Hardware-/Custom-Zweig erzeugt; kein einheitliches Schema für alle Kanäle. |
| `dimmer1–N.offtime` | Zusatzinformation zum Kanal (Im Handler prüfen) | Nein | Nur falls im jeweiligen Hardware-/Custom-Zweig erzeugt; kein einheitliches Schema für alle Kanäle. |
| `dimmer1–N.source` | Zusatzinformation zum Kanal (Im Handler prüfen) | Nein | Nur falls im jeweiligen Hardware-/Custom-Zweig erzeugt; kein einheitliches Schema für alle Kanäle. |
| `dimmer9.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer9.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer10.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer10.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer11.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer11.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer12.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer12.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer13.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer13.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer14.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer14.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer15.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `dimmer15.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `rgbw1.state` | RGBW-Kanal: state (nicht festgelegt) | Nein | Optionaler Dimmy-Pro-Zweig. |
| `rgbw1.name` | RGBW-Kanal: name (nicht festgelegt) | Nein | Optionaler Dimmy-Pro-Zweig. |
| `rgbw1.autooff` | RGBW-Kanal: autooff (nicht festgelegt) | Nein | Optionaler Dimmy-Pro-Zweig. |
| `rgbw1.offtime` | RGBW-Kanal: offtime (nicht festgelegt) | Nein | Optionaler Dimmy-Pro-Zweig. |
| `rgbw2.state` | RGBW-Kanal: state (nicht festgelegt) | Nein | Optionaler Dimmy-Pro-Zweig. |
| `rgbw2.name` | RGBW-Kanal: name (nicht festgelegt) | Nein | Optionaler Dimmy-Pro-Zweig. |
| `rgbw2.autooff` | RGBW-Kanal: autooff (nicht festgelegt) | Nein | Optionaler Dimmy-Pro-Zweig. |
| `rgbw2.offtime` | RGBW-Kanal: offtime (nicht festgelegt) | Nein | Optionaler Dimmy-Pro-Zweig. |

### `/wrelay/`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `WifiRelay1–8.autooff` | Zusatzinformation zum Kanal (Im Handler prüfen) | Nein | Nur falls im jeweiligen Hardware-/Custom-Zweig erzeugt; kein einheitliches Schema für alle Kanäle. |
| `WifiRelay1–8.offtime` | Zusatzinformation zum Kanal (Im Handler prüfen) | Nein | Nur falls im jeweiligen Hardware-/Custom-Zweig erzeugt; kein einheitliches Schema für alle Kanäle. |
| `WifiRelay1–8.firmware` | Zusatzinformation zum Kanal (Im Handler prüfen) | Nein | Nur falls im jeweiligen Hardware-/Custom-Zweig erzeugt; kein einheitliches Schema für alle Kanäle. |
| `WifiRelay1.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay1.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay2.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay2.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay3.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay3.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay4.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay4.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay5.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay5.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay6.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay6.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay7.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay7.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay8.state` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |
| `WifiRelay8.name` | Zusätzlicher Kanal/Zustand oder Name (nicht festgelegt) | Nein | Abhängig von Erweiterung/Hardware; zusätzliche Custom-Kanäle können darüber hinausgehen. |

### `/bms`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `BMSamps` | BMS: BMSamps (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMSamps; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMScap` | BMS: BMScap (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMScap; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMSnominalAh` | BMS: BMSnominalAh (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMSnominalAh; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMScell1` | BMS: BMScell1 (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMScell1; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMScell2` | BMS: BMScell2 (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMScell2; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMScell3` | BMS: BMScell3 (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMScell3; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMScell4` | BMS: BMScell4 (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMScell4; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMSmaxcap` | BMS: BMSmaxcap (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMSmaxcap; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMSmaxvolt` | BMS: BMSmaxvolt (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMSmaxvolt; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMSminvolt` | BMS: BMSminvolt (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMSminvolt; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMSpower` | BMS: BMSpower (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMSpower; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMSsoc` | BMS: BMSsoc (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMSsoc; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMStemp` | BMS: BMStemp (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMStemp; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |
| `BMSvolt` | BMS: BMSvolt (Zellwerte: V (Quelle /1000); sonst Geräte-/Treiberdefinition) | Nein | BMSvolt; Zellwerte werden als String mit zwei Nachkommastellen ausgegeben. Einheiten der übrigen Werte nicht allein aus Namen ableiten. |

### `/heater`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `heatertoggle` | Heizungswert: heatertoggle (Typ/Einheit geräteabhängig) | Teilweise | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `heatstatus` | Heizungswert: heatstatus (Typ/Einheit geräteabhängig) | Teilweise | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `heattemp` | Heizungswert: heattemp (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `heatvolt` | Heizungswert: heatvolt (V) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `heatfan` | Heizungswert: heatfan (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `heatglow` | Heizungswert: heatglow (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `heatwpump` | Heizungswert: heatwpump (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `heaterror` | Heizungswert: heaterror (Typ/Einheit geräteabhängig) | Teilweise | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `targettemp_vanpi` | Heizungswert: targettemp_vanpi (°C) | Teilweise | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `runtime_m` | Heizungswert: runtime_m (min) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `runtime_remaining_s` | Heizungswert: runtime_remaining_s (s) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `timer_state` | Heizungswert: timer_state (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `timer` | Heizungswert: timer (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `tempsensor` | Heizungswert: tempsensor (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `tempsensor_name` | Heizungswert: tempsensor_name (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `heater_name` | Heizungswert: heater_name (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.heatertoggle` | Heizungswert: heatertoggle (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.heatstatus` | Heizungswert: heatstatus (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.heattemp` | Heizungswert: heattemp (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.heatvolt` | Heizungswert: heatvolt (V) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.heatfan` | Heizungswert: heatfan (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.heatglow` | Heizungswert: heatglow (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.heatwpump` | Heizungswert: heatwpump (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.heaterror` | Heizungswert: heaterror (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.targettemp_vanpi` | Heizungswert: targettemp_vanpi (°C) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.runtime_m` | Heizungswert: runtime_m (min) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.runtime_remaining_s` | Heizungswert: runtime_remaining_s (s) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.timer_state` | Heizungswert: timer_state (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.timer` | Heizungswert: timer (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.tempsensor` | Heizungswert: tempsensor (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.tempsensor_name` | Heizungswert: tempsensor_name (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `generic.heater_name` | Heizungswert: heater_name (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.heatertoggle` | Heizungswert: heatertoggle (Typ/Einheit geräteabhängig) | Teilweise | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.heatstatus` | Heizungswert: heatstatus (Typ/Einheit geräteabhängig) | Teilweise | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.heattemp` | Heizungswert: heattemp (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.heatvolt` | Heizungswert: heatvolt (V) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.heatfan` | Heizungswert: heatfan (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.heatglow` | Heizungswert: heatglow (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.heatwpump` | Heizungswert: heatwpump (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.heaterror` | Heizungswert: heaterror (Typ/Einheit geräteabhängig) | Teilweise | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.targettemp_vanpi` | Heizungswert: targettemp_vanpi (°C) | Teilweise | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.runtime_m` | Heizungswert: runtime_m (min) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.runtime_remaining_s` | Heizungswert: runtime_remaining_s (s) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.timer_state` | Heizungswert: timer_state (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.timer` | Heizungswert: timer (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.tempsensor` | Heizungswert: tempsensor (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.tempsensor_name` | Heizungswert: tempsensor_name (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.heater_name` | Heizungswert: heater_name (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.mode` | Heizungswert: mode (Typ/Einheit geräteabhängig) | Teilweise | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.fanspeed` | Heizungswert: fanspeed (Typ/Einheit geräteabhängig) | Teilweise | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm1.powerlevel` | Heizungswert: powerlevel (Typ/Einheit geräteabhängig) | Teilweise | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.heatertoggle` | Heizungswert: heatertoggle (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.heatstatus` | Heizungswert: heatstatus (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.heattemp` | Heizungswert: heattemp (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.heatvolt` | Heizungswert: heatvolt (V) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.heatfan` | Heizungswert: heatfan (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.heatglow` | Heizungswert: heatglow (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.heatwpump` | Heizungswert: heatwpump (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.heaterror` | Heizungswert: heaterror (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.targettemp_vanpi` | Heizungswert: targettemp_vanpi (°C) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.runtime_m` | Heizungswert: runtime_m (min) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.runtime_remaining_s` | Heizungswert: runtime_remaining_s (s) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.timer_state` | Heizungswert: timer_state (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.timer` | Heizungswert: timer (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.tempsensor` | Heizungswert: tempsensor (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.tempsensor_name` | Heizungswert: tempsensor_name (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.heater_name` | Heizungswert: heater_name (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.mode` | Heizungswert: mode (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.fanspeed` | Heizungswert: fanspeed (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `autoterm2.powerlevel` | Heizungswert: powerlevel (Typ/Einheit geräteabhängig) | Nein | generic/Autoterm-Unterobjekte nur bei aktivierter Integration; CamperUI wertet nicht das ganze Objekt aus. |
| `tempsensors.*` | Temperatur-/Feuchtesensoren einschließlich optionaler Quellen (nicht festgelegt) | Nein |  |
| `autoterm2.parallel` | Parallelbetrieb (nicht festgelegt) | Nein |  |
| `truma_CI` | Zusätzliches Geräteobjekt (nicht festgelegt) | Nein | Featureabhängiger Rückgabezweig; teilweise durchgereichte/dynamische Datenstruktur. |
| `timberline` | Zusätzliches Geräteobjekt (nicht festgelegt) | Nein | Featureabhängiger Rückgabezweig; teilweise durchgereichte/dynamische Datenstruktur. |
| `ventilation.maxxfan` | Zusätzliches Geräteobjekt (nicht festgelegt) | Nein | Featureabhängiger Rückgabezweig; teilweise durchgereichte/dynamische Datenstruktur. |
| `ventilation.bayernluefter` | Zusätzliches Geräteobjekt (nicht festgelegt) | Nein | Featureabhängiger Rückgabezweig; teilweise durchgereichte/dynamische Datenstruktur. |
| `boiler` | Zusätzliches Geräteobjekt (nicht festgelegt) | Nein | Featureabhängiger Rückgabezweig; teilweise durchgereichte/dynamische Datenstruktur. |
| `smartboil` | Zusätzliches Geräteobjekt (nicht festgelegt) | Nein | Featureabhängiger Rückgabezweig; teilweise durchgereichte/dynamische Datenstruktur. |
| `dometic_rc10_CI` | Zusätzliches Geräteobjekt (nicht festgelegt) | Nein | Featureabhängiger Rückgabezweig; teilweise durchgereichte/dynamische Datenstruktur. |
| `smartboil.deviceId` | Smartboil: deviceId (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.connected` | Smartboil: connected (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.lastUpdate` | Smartboil: lastUpdate (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.uptime` | Smartboil: uptime (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.temperature` | Smartboil: temperature (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.targetTemp` | Smartboil: targetTemp (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.hysteresis` | Smartboil: hysteresis (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.ledBrightness` | Smartboil: ledBrightness (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.dcEnabled` | Smartboil: dcEnabled (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.acEnabled` | Smartboil: acEnabled (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.autoHeat` | Smartboil: autoHeat (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.boostMode` | Smartboil: boostMode (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.dcSimple` | Smartboil: dcSimple (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.dcActive` | Smartboil: dcActive (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.acActive` | Smartboil: acActive (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.acConnected` | Smartboil: acConnected (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.hvacAction` | Smartboil: hvacAction (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.lastError` | Smartboil: lastError (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |
| `smartboil.firmware` | Smartboil: firmware (Geräte-/Produzentendefinition) | Nein | Nur bei sichtbarer Smartboil-Integration und gültigem Geräteobjekt. |

### `/maxxfan/`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `maxxfan.fan_power` | Lüfter ein/aus (bool) | Nein | maxxfan_power; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |
| `maxxfan.fan_direction` | Richtung (in/out) | Nein | maxxfan_direction; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |
| `maxxfan.fan_temp` | Automatik-Solltemperatur (°C) | Nein | maxxfanAuto_temp; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |
| `maxxfan.fan_auto` | Automatik ein/aus (bool) | Nein | maxxfan_auto; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |
| `maxxfan.fan_speed` | Stufe (1–10) | Nein | maxxfan_speed; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |
| `maxxfan.fan_vent` | Klappe (open/close) | Nein | maxxfan_vent; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |

### `/fan/`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `maxxfan.fan_power` | Lüfter ein/aus (bool) | Nein | maxxfan_power; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |
| `maxxfan.fan_direction` | Richtung (in/out) | Nein | maxxfan_direction; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |
| `maxxfan.fan_temp` | Automatik-Solltemperatur (°C) | Nein | maxxfanAuto_temp; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |
| `maxxfan.fan_auto` | Automatik ein/aus (bool) | Nein | maxxfan_auto; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |
| `maxxfan.fan_speed` | Stufe (1–10) | Nein | maxxfan_speed; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |
| `maxxfan.fan_vent` | Klappe (open/close) | Nein | maxxfan_vent; Flow verwendet Ersatzwerte; vorhandene Felder beweisen keine aktive Hardware. |

### `/bayernluefter/`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `bayernluefter.SystemOn` | Lüfterexport: SystemOn (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.SystemMode` | Lüfterexport: SystemMode (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.Speed_In` | Lüfterexport: Speed_In (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.Speed_Out` | Lüfterexport: Speed_Out (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.Speed_AntiFreeze` | Lüfterexport: Speed_AntiFreeze (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.Temp_In` | Lüfterexport: Temp_In (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.Temp_Out` | Lüfterexport: Temp_Out (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.Temp_Fresh` | Lüfterexport: Temp_Fresh (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.rel_Humidity_In` | Lüfterexport: rel_Humidity_In (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.rel_Humidity_Out` | Lüfterexport: rel_Humidity_Out (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.abs_Humidity_In` | Lüfterexport: abs_Humidity_In (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.abs_Humidity_Out` | Lüfterexport: abs_Humidity_Out (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.Efficiency` | Lüfterexport: Efficiency (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.Humidity_Transport` | Lüfterexport: Humidity_Transport (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.DeviceName` | Lüfterexport: DeviceName (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.Date` | Lüfterexport: Date (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.Time` | Lüfterexport: Time (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.MAC` | Lüfterexport: MAC (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.LocalIP` | Lüfterexport: LocalIP (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.RSSI` | Lüfterexport: RSSI (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.FW_MainController` | Lüfterexport: FW_MainController (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.FW_WiFi` | Lüfterexport: FW_WiFi (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.runtime` | Lüfterexport: runtime (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |
| `bayernluefter.runtime_remaining_s` | Lüfterexport: runtime_remaining_s (Geräteexport) | Nein | bayernluefter_state; Dynamische Exportfelder; diese Namen werden vom Flow verwendet. Laufzeitfelder werden zusätzlich ergänzt. |

### `/boiler/`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `boiler_activated` | Boilerstatus/-Konfiguration: boiler_activated (Im Produzenten definiert) | Nein | boiler_values |
| `boiler_relay.relay` | Boilerstatus/-Konfiguration: boiler_relay.relay (Im Produzenten definiert) | Nein | boiler_values |
| `boiler_relay.state` | Boilerstatus/-Konfiguration: boiler_relay.state (Im Produzenten definiert) | Nein | boiler_values |
| `boiler_relay.name` | Boilerstatus/-Konfiguration: boiler_relay.name (Im Produzenten definiert) | Nein | boiler_values |
| `emergency_limit` | Boilerstatus/-Konfiguration: emergency_limit (Im Produzenten definiert) | Nein | boiler_values |
| `standard_runtime` | Boilerstatus/-Konfiguration: standard_runtime (Im Produzenten definiert) | Nein | boiler_values |
| `waterlevel_min` | Boilerstatus/-Konfiguration: waterlevel_min (Im Produzenten definiert) | Nein | boiler_values |
| `waterlevel_current` | Boilerstatus/-Konfiguration: waterlevel_current (Im Produzenten definiert) | Nein | boiler_values |
| `autotemp_control` | Boilerstatus/-Konfiguration: autotemp_control (Im Produzenten definiert) | Nein | boiler_values |
| `hysteresis` | Boilerstatus/-Konfiguration: hysteresis (Im Produzenten definiert) | Nein | boiler_values |
| `target_temp` | Boilerstatus/-Konfiguration: target_temp (Im Produzenten definiert) | Nein | boiler_values |
| `boiler_temp` | Boilerstatus/-Konfiguration: boiler_temp (Im Produzenten definiert) | Nein | boiler_values |
| `emergency_shutdown` | Boilerstatus/-Konfiguration: emergency_shutdown (Im Produzenten definiert) | Nein | boiler_values |
| `boiler_emergency_reason` | Boilerstatus/-Konfiguration: boiler_emergency_reason (Im Produzenten definiert) | Nein | boiler_values |
| `boiler_connected_heater` | Boilerstatus/-Konfiguration: boiler_connected_heater (Im Produzenten definiert) | Nein | boiler_values |
| `boiler_boost_state` | Boilerstatus/-Konfiguration: boiler_boost_state (Im Produzenten definiert) | Nein | boiler_values |

### `/gps`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `lat` | Breitengrad (Grad) | Nein |  |
| `lon` | Längengrad (Grad) | Nein |  |
| `date` | Aktualisierungszeit (Datum/Zeit) | Nein |  |

### `/info/`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `VanPi_Ctrl` | Systeminformation: VanPi_Ctrl (nicht festgelegt) | Nein |  |
| `install_date` | Systeminformation: install_date (nicht festgelegt) | Nein |  |
| `current_date` | Systeminformation: current_date (nicht festgelegt) | Nein |  |
| `uptime` | Systeminformation: uptime (nicht festgelegt) | Nein |  |
| `van_name` | Systeminformation: van_name (nicht festgelegt) | Nein |  |
| `cpu_temp` | Systeminformation: cpu_temp (nicht festgelegt) | Nein |  |
| `cpu_usage` | Systeminformation: cpu_usage (nicht festgelegt) | Nein |  |
| `board_temp` | Systeminformation: board_temp (nicht festgelegt) | Nein |  |
| `ttgo` | Systeminformation: ttgo (nicht festgelegt) | Nein |  |

### `/debug`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `BatteryDataDelivery` | Diagnose: BatteryDataDelivery (nicht festgelegt) | Nein |  |
| `accesspoint` | Diagnose: accesspoint (nicht festgelegt) | Nein |  |
| `eth0IP` | Diagnose: eth0IP (nicht festgelegt) | Nein |  |
| `pkwshuntactive` | Diagnose: pkwshuntactive (nicht festgelegt) | Nein |  |
| `version` | Diagnose: version (nicht festgelegt) | Nein |  |
| `wifiIP` | Diagnose: wifiIP (nicht festgelegt) | Nein |  |

### `/position_sensor/?request=true`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `x_angle` | Neigung (Grad) | Ja | x_angle |
| `y_angle` | Neigung (Grad) | Ja | y_angle |

### `/names`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `Relay*/WifiRelay*/Dimmer*/Level*/Temp*.name` | Kanal-/Sensornamen (nicht festgelegt) | Nein |  |
| `Relay*/WifiRelay*/Dimmer*/Level*/Temp*.globalVariable` | Interne Variable für den Namen (nicht festgelegt) | Nein | Namensvariable wie Ndimmer3; keine Variable zum Schalten des Kanalzustands. |

### `/conniot`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `connection.state` | Konstante Testantwort (bool) | Nein | Im Export immer true; kein Nachweis einer Internet-/IoT-Verbindung. |

### `/smartboil`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `<Geräteobjekt>` | Dynamische/zusammengestellte Antwort (nicht festgelegt) | Nein | smartboil.devices[deviceId]; Schlüssel dynamisch aus Gerätemeldungen; kein festes Schema im HTTP-Handler. |

### `/dometic_rc10_ci/`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `<Geräteobjekt>` | Dynamische/zusammengestellte Antwort (nicht festgelegt) | Nein | rc10State; CAN-/Gerätedaten durchgereicht; timestamp wird ergänzt, status.power_on im Flow verwendet. |

### `/timberline`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `<Geräteobjekt>` | Dynamische/zusammengestellte Antwort (nicht festgelegt) | Nein | timberline; Gespeichertes Geräteobjekt direkt zurückgegeben; konkrete Schlüssel live abgleichen. |

### `/lte_bridge`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `lteBridge` | Dynamische/zusammengestellte Antwort (nicht festgelegt) | Nein | flow.lteBridge_data; Letzte eingehende Bridge-Nachricht inklusive ergänztem Zeitstempel; dynamische Struktur. |

### `/app_home/`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `<zusammengestellte Auswahl>` | Dynamische/zusammengestellte Antwort (nicht festgelegt) | Nein | Auswahl über Query-Parameter; re/r, w, di/d, t, b, l, ru; Antwort nach folgenden verbundenen Knoten aufgebaut. |

### `/network`

| Feld | Bedeutung/Einheit | Display | Quelle/Hinweis |
|---|---|---|---|
| `<Netzwerkantwort>` | Dynamische/zusammengestellte Antwort (nicht festgelegt) | Nein | Systemabfragen; Zusammengestellte Netzwerkdaten; kein einzelnes festes Sensorobjekt. |

## Quellen und Vollständigkeit

- Flow-Datei: `docs/flows.json`, UTF-16, 7674 Nodes, 71 HTTP-in-Nodes.
- SHA-256 des Flow-Exports: `3a7992dca3798982e1dad62e13551255373c901879808581508ad212e3fb4923`.
- Display-Auswertung: `http_handler.cpp`, `system_state.h`, `ui_home.cpp`.
- Die Wegliste ist vollständig für HTTP-in-Nodes dieses Exports. Die Feldliste dokumentiert
  statisch erkennbare Sensorfelder und bekannte Varianten; dynamische Geräteobjekte, Custom-Daten
  und zusammengesetzte Antworten sind nicht als vollständig aufgelöste Schemas dargestellt.
- Keine Live-Abfragen durchgeführt. Für die tatsächliche Installation sind Geräteausstattung,
  aktiver Flow-Stand und reale JSON-Antworten maßgeblich.
