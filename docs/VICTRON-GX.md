# Victron GX – Architektur und Einrichtung (RV3D.90)

## Architektur

Basis ist CamperUI V1.8.0 mit Websettings, Truma-Erweiterungen, Energie-Dashboard, Screensaver und den RV3D.89-Verläufen. VanPi-HTTP und dessen Datenmodell bleiben eigenständig. Ein eigener, niedrig priorisierter MQTT-Worker auf Core 0 bedient genau eine GX-Verbindung. LVGL wird ausschließlich auf dem UI-Thread verwendet. Beide Victron-Ansichten und GX-Favoriten lesen denselben Cache; Befehle laufen über eine begrenzte Queue.

Die Anbindung startet deaktiviert, ohne persönlichen Host oder Relaisnamen. MQTT TCP (1883, optional andere TCP-Ports) mit optionaler Anmeldung. TLS/VRM-Cloud gehören nicht zu dieser ersten lokalen Integration. Ein leerer Host ist eine unkonfigurierte Verbindung, keine Broadcast-Adresse. Benutzername/Passwort sind optional; Passwörter werden nicht geloggt.

## Verifizierte Schnittstelle

Notifications: N/<Portal-ID>/<Dienst>/<Instanz>/<Pfad>, JSON {"value": ...}. Writes: W mit identischem Ziel und Payload, nicht retained. Null oder leere Nachrichten bedeuten nicht verfügbar. Steuerbefehle werden erst durch passende neue N-Nachrichten bestätigt; retained Werte bestätigen keine neuen Befehle. Keine Wiederholung unbestätigter Writes.

VE.Bus /Mode: 3 EIN, 4 AUS, 1 LADEN, 2 Nur Wechselrichter (nur Anzeige). /ModeIsAdjustable muss Fernsteuerung erlauben; /Connected wird mit dem aktuellen Modus ausgewertet. /State zeigt den tatsächlichen Zustand unabhängig von /Mode. AC-Detailwerte sind L1; Gesamtleistungen werden bevorzugt, wenn vorhanden. DC-Leistung wird aus DC-Spannung und -Strom berechnet und als DC-Leistung bezeichnet, nicht als garantierte Brutto-Ladeleistung.

GX-Relais: system/<Instanz>/Relay/0/State und Relay/1/State. Manuelle Funktion 2 aus settings/<Instanz>/Settings/Relay/Function bzw. Settings/Relay/1/Function muss bekannt sein. CamperUI verändert diese Funktionen nicht automatisch.

Quellen: [Victron MQTT-Protokoll](https://github.com/victronenergy/dbus-flashmq), [D-Bus-Pfade](https://github.com/victronenergy/venus/wiki/dbus), [Relais-Konfiguration](https://github.com/victronenergy/gui-v2/blob/main/pages/settings/PageSettingsRelay.qml), [Funktionswerte](https://github.com/victronenergy/gui-v2/blob/main/src/enums.h).

## Erkennung und Keepalive

Portal-ID wird aus wenigen empfangenen N-Topics ermittelt. Die bevorzugte VE.Bus-Instanz stammt aus system/VebusInstance; eine eindeutige VE.Bus-Erkennung ist der Fallback. Bei mehreren Geräten oder Portal-IDs ist eine explizite Auswahl erforderlich. Manuelle Portal-ID und Geräte-ID sind deshalb einstellbar. Bei Venus-Versionen ohne retained Daten kann die Erkennung auf den nächsten externen Refresh warten. Ohne bestehende Veröffentlichungen kann eine manuelle Portal-ID nötig sein, bevor eigener Keepalive startet.

Eigener Victron-Keepalive ist standardmäßig AUS und unabhängig vom normalen MQTT-Verbindungs-Keepalive. Einschalten sendet einmal einen Refresh, danach sparsame suppress-republish-Keepalives. Gezielte initiale Reads holen selten veränderte Steuer-/Konfigurationswerte; Daten werden nicht ständig gepollt. Vor jedem Write werden Zustand und Steuerfreigabe bzw. Relaisfunktion neu gelesen. Erst vollständige aktuelle Rückmeldungen erlauben das Schreiben. Ein Read nach einem Write fordert eine Rückmeldung an. Stille oder abgerissene Verbindungen werden sichtbar; alte Daten aktivieren keine Tasten.

## Oberflächen und Ressourcen

Home: Wasser oder MultiPlus, gegenseitig exklusiv. Drei Tasten gleicher Höhe/Radien wie Favoriten; Titel und kompakte Statuszeile. Engere Abstände erhalten die verfügbare Höhe. Der Victron-Tab ist optional und bietet Details sowie beide Relais; vertikales Scrollen ist dort zulässig. Zwei zusätzliche Favoriten-IDs 17/18 erhalten die bisherigen IDs 0..16.

Konfiguration auf Display UND /settings, mit gemeinsamen NVS-Schlüsseln und Status-API. Das Zahnrad oben rechts im Victron-Tab öffnet direkt den Victron-Bereich der Display-Einstellungen. Neuer Tab wird mit stabiler ID angehängt; bestehende Reihenfolge und Sichtbarkeit migrieren. Default: MQTT aus, eigener Keepalive aus, Victron-Tab aus, Home-Wasser wie bisher. PubSubClient v2.8 liegt samt MIT-Lizenz im Projekt: keine weitere Arduino-Bibliotheksinstallation nötig. Lokale kleine Anpassung stellt das retained-Flag für Rückmeldungsprüfung bereit.

MQTT-Puffer 768 Bytes, Worker-Stack 6 KiB, begrenzter Cache und Queue. Weitere TCP-/RTOS-Allocations entstehen beim Aktivieren. Es gibt keine zusätzlichen Charts, keine GUI-Arbeit im Worker und keine zweite MQTT-Verbindung pro Ansicht. Boot- und Laufzeit-Heaps sind am Gerät zu prüfen.

Die echte Cerbo-IP und aktuelle Geräte-IDs wurden noch nicht angegeben. Daher keine Live-Schaltversuche; die konkrete Anlage muss nach Konfiguration gesondert getestet werden.


## Einrichtung nach dem Arduino-Upload

1. Unter System-Einstellungen → Victron bzw. im Browser unter `/settings` → Victron GX die reale Cerbo-IP oder einen auflösbaren Hostnamen eintragen. Empfohlen: DHCP-Reservierung im Router, damit die IP gleich bleibt. Keine Adresse mit `.255` als Vorgabe verwenden: sie ist bei üblichen Heimnetzen Broadcast.
2. MQTT am GX im lokalen Netzwerk aktivieren. Port 1883 verwenden, falls nicht anders eingerichtet; Zugangsdaten nur falls erforderlich. Die Anbindung in CamperUI einschalten und speichern.
3. Zum ersten UI-Test kann CamperUI im globalen Dummy-Modus bleiben. Für echte GX-Daten auf Live wechseln. Dummy sendet keine GX-Schaltbefehle.
4. Portal-ID und MultiPlus-Geräte-ID zunächst automatisch ermitteln lassen. Wenn keine Werte ankommen: die Portal-ID am GX nachsehen und eintragen. Eigener Keepalive bleibt zunächst aus; nur einschalten, wenn kein anderer Teilnehmer den GX-Datenstrom aufrechterhält. Eine manuelle Geräte-ID verwenden, wenn mehrere VE.Bus-Geräte erkannt werden.
5. Den Victron-Tab aktivieren; auf Home bei Bedarf MultiPlus statt Wasser auswählen. Beide GX-Relais stehen auch als Favoriten zur Auswahl. Anzeigenamen frei vergeben; Funktionen am Cerbo selbst auf **Manuell** setzen, sofern sie aus CamperUI geschaltet werden sollen.

## Prüfung und offene Gerätetests

Die Protokollprüfungen verwenden die tatsächlichen C++-Hilfsfunktionen: Topic-Parsing, Auswahl bei mehreren Geräten, Migration der bisherigen Tab-Konfiguration, frische Rückmeldungen, Retained-Ausschluss, Timeout/Überlauf und neue Freigabeabfragen vor einem Write. Die bestehenden Prüfungen für Relais-Touch, Energie-Historie und MaxxFan wurden ebenfalls erfolgreich ausgeführt.

Die Web-Oberfläche wurde mit lokalen Test-APIs geprüft: MQTT/Keepalive standardmäßig aus, editierbare Adresse und Namen, GX-Favoriten 17/18, Wasser/MultiPlus exklusiv, elf Tabs und Fehlermeldung bei ungültigem Port. Das prüft die Browser-Bedienung, nicht die NVS-Speicherung auf Hardware.

Am echten Display/Cerbo noch prüfen:

- Erst lesend: Portal/Geräte-ID, Mode/State, AC IN/OUT, Frequenz, DC-Leistung, SoC und PV, soweit die Anlage sie liefert. Nicht vorhandene Werte müssen `--` bleiben.
- Danach EIN/AUS/LADEN und beide manuell konfigurierten Relais testen. Erst Rückmeldung darf den Zustand bestätigen; ohne Rückmeldung muss nach 15 Sekunden ein Fehler erscheinen. Wiederholtes/langes Drücken darf keine Befehlsserie auslösen.
- Ein automatisch verwendetes Relais darf nicht schaltbar sein. Fehlende Mode-Freigabe oder getrenntes VE.Bus-Gerät müssen die MultiPlus-Tasten sperren.
- WLAN/GX-Verbindung trennen, neu verbinden und die Zieladresse ändern: keine alten Befehle nachholen und keine alten Rückmeldungen übernehmen.
- Display- und Websettings speichern, neu starten, Tab-Reihenfolge und Home-Auswahl prüfen. Bestehende VanPi-Seiten/Favoriten müssen weiter funktionieren.
- Freien Heap nach Aktivierung und im Langzeitbetrieb beobachten; beide Ansichten wiederholt wechseln und OTA durchführen. Die zusätzliche Laufzeitbelegung lässt sich erst am Gerät zuverlässig messen.

## Starterbatterie-Spannung

Die Quelle ist in den Victron-Einstellungen auf Display und Web auswählbar. Standard bleibt VanPi. Cerbo GX liest ausschließlich `N/<Portal>/battery/<Instanz>/Dc/1/Voltage` vom Starter-/Aux-Eingang eines SmartShunt/BMV und den Verbindungsstatus desselben Monitors. Der Monitor muss für Starterspannungsmessung eingerichtet und angeschlossen sein. Die Hauptbatteriespannung `/Dc/0/Voltage` wird niemals als Starterspannung verwendet.

Die Batteriemonitor-ID ist unabhängig von der MultiPlus-/VE.Bus-ID. Automatik akzeptiert einen eindeutig erkannten Monitor mit gültiger Starterspannung; bei mehreren bleibt die Anzeige leer, bis eine ID gewählt wird. Keine automatische Rückkehr zur VanPi-Quelle bei MQTT-Ausfall. Die bestehende VanPi-Messung bleibt getrennt erhalten. Dummy verwendet den bereits vorhandenen simulierten Starterwert; in Live sind Gerätetests erforderlich.

## MultiPlus-Verbindungsdiagnose

Die MultiPlus-ID ist die numerische VRM-/VE.Bus-Geräteinstanz (beispielsweise 276), nicht die hexadezimale Product-ID (beispielsweise 0x2605). Fehlender `Connected`-Status und ausdrücklich empfangenes `Connected=0` werden getrennt angezeigt. `/api/victron` liefert unter `vebus_diagnosis` für Mode, Connected und ModeIsAdjustable die Rohwerte, Gültigkeit, Retained-/Live-Kennung, Empfangsalter und verwendeten Topics. Das erleichtert die Prüfung, ohne Steuerfreigaben zu lockern oder Schaltbefehle zu senden.

## Einschalten aus AUS bei Connected=0

CamperUI erlaubt EIN/LADEN auch aus dem explizit zurückgemeldeten Modus AUS (`Mode=4`), wenn `Connected=0` und `ModeIsAdjustable=1` aktuell vom GX vorliegen. Die MQTT-Verbindung, eindeutige Geräteinstanz und alle drei gültigen, nicht-retained Werte bleiben erforderlich. Andere Modi bei `Connected=0`, fehlende Freigabe oder fehlende/veraltete Daten sperren weiter. Messwerte werden bei `Connected=0` nicht als aktuell ausgegeben; Modus und Steuerfreigabe bleiben separat sichtbar.

Vor jedem echten Write werden alle drei Werte erneut gelesen und dieselbe Freigabelogik angewandt. Der angeforderte Modus wird erst nach einer neuen passenden Rückmeldung bestätigt. Keine automatische Wiederholung unbestätigter Befehle. Gerätetest: MultiPlus über GX ausschalten, Display neu starten, direkt EIN bzw. LADEN wählen, Rückmeldung prüfen und erneut AUS/EIN testen. Das direkte Einschalten nach dem Display-Neustart bei ausgeschaltetem MultiPlus wurde vom Benutzer live erfolgreich getestet.

## Dynamische Home-Leistungskachel

Wenn Home-MultiPlus aktiviert ist, erscheint bei bestätigtem EIN/LADEN (auch Nur-Wechselrichter) neben Batterie und Solar eine zusätzliche MP-Kachel mit AC-IN-/AC-OUT-Leistung und vorzeichenbehafteter DC-Leistung. Positive DC-Werte stehen für Zufuhr, negative für Entnahme. Bei bestätigtem AUS wird sie ausgeblendet. Kurze Datenlücken ändern die zuletzt bestätigte Sichtbarkeit nicht, Werte werden als `--` dargestellt. Ein Wechsel Dummy/Live verwirft die vorherige Sichtbarkeit.

Bei drei Kacheln bleiben Batterie/Solar gleich breit (172 px), MP erhält 104 px. Die bisherigen Schriftgrößen bleiben unverändert; die schmale MP-Kachel verwendet 20 px für neue Werte. Die Home-Steuertasten bleiben 56 px hoch, ihre alte AC-Wertezeile entfällt. Die Gesamthöhe wird kleiner; Antippen der neuen MP-Kachel öffnet den VE-Tab. Ausgeblendete Batterie-/Solarkacheln geben ihre Breite weiterhin an die übrigen Kacheln ab. Sichtbarkeit und Werte verwenden denselben GX-Cache und erzeugen keine zusätzliche MQTT-Verbindung.

Am Gerät: EIN/LADEN/AUS, kurze WLAN-Unterbrechung, Dummy/Live-Wechsel und unterschiedliche Home-Feldauswahlen prüfen. Die tatsächliche Darstellung ist noch live zu beurteilen.

## Markierung der AUS-Taste

Auf Home und im VE-Tab wird AUS nur kurz blau markiert: maximal zehn Sekunden nach einem angenommenen Tastendruck und nur, wenn der tatsächlich empfangene Modus AUS ist. Die Markierung löst keine weiteren Befehle aus und wird bei Fehler, Timeout oder Verbindungswechsel verworfen. Danach erscheint AUS neutral; die Statuszeile zeigt den Modus weiterhin an. EIN und LADEN bleiben entsprechend dem aktuellen Modus markiert.

## Integration auf V1.6.1

Der Entwicklungsstand heißt `V1.6.1-RV3D.90-dev`. V1.6.1 (`ef54f4141c24da032d1291f7a8a592fd9f391f49`) ist vollständig als Basis enthalten, einschließlich Energiefluss-/Erzeugerauswahl, Truma Gas/Diesel/Elektro, Improv-Konfiguration und ST7701S-Reset. Der VE-Tab wird an den Upstream-Truma-Tab angehängt: Truma=10, Victron=11. Die früheren RV3D.90-Einstellungen mit VE=10 werden unter Beibehaltung von Position/Sichtbarkeit migriert. Die alten zehn Tabs und ein bereits gespeicherter Stock-V1.6.1-Truma-Tab bleiben ebenso erhalten. Ein Schema-Marker verhindert eine wiederholte Umdeutung nach dem Speichern.

Die frühere RV3D-Auswahl Keine/Deaktiviert (`heater_type=2`) wird zur neuen V1.6.1-Kennung 3 migriert. Victron- und VanPi-Energiequellen bleiben getrennt. Der neue Upstream-Energiefluss nutzt weiterhin VanPi-Daten; die eigene MP-Home-Kachel nutzt den GX-Cache. Nach dem Update sind Navigation, Truma-Auswahl, Erzeuger-Einstellungen, Display-Neustarts und GX-Live-Bedienung am Gerät erneut zu prüfen.

## Integration auf V1.7.0

Der aktuelle Entwicklungsstand heißt `V1.7.0-RV3D.90-dev`, basierend auf Release-Tag V1.7.0 (`362a4fea3d0a9b96743ebe246b3ebb9ecae5a082`). Die VE-Seite, Einstellungen am Display und im Browser, GX-Favoriten, die optionale Starterspannung und die dynamische MultiPlus-Home-Kachel bleiben erhalten. Die AUS-Rückmeldung bleibt auf zehn Sekunden begrenzt. Die neuen VanPi-Relaissperren werden auch bei Home-Favoriten berücksichtigt; GX-Relais behalten ihre separate Freigabeprüfung.

V1.7.0 liefert Energie-Arcs, Nacht-Screensaver und VanPi-Uhrzeitsynchronisation. Die bisherige manuelle/NTP-Uhrzeiteinstellung entfällt entsprechend Upstream. Die vorhandene Migration für Truma/VE-Tab-IDs bleibt bestehen. Die Landstrom-/Lichtmaschinen-Datenquellen und Schätzungen bleiben vorerst unverändert; sie sind nicht automatisch auf Cerbo umgestellt. Nach dem Update sind Screensaver/Aufwecken, gesperrte Home-Relais und VE-Steuerung am Gerät zu prüfen.

## Integration auf V1.7.1

Der aktuelle lokale Entwicklungsstand heißt `V1.7.1-RV3D.90-dev`. Das offizielle Release enthält die RV3D.90-VE-Erweiterungen und die Home-Anpassungen bereits: GX-Favoriten mit Befehlsrückmeldung, Home-Relaissperren, MultiPlus-Steuerung, dynamische AC/DC-Kachel und zehnsekündige AUS-Rückmeldung. Die Home-Abweichungen zur lokalen V1.7.0-Integration waren ausschließlich Formatierung; die Release-Fassung übernimmt diese Funktionen vollständig.

V1.7.1 entfernt die internen Leistungs-Rückrechnungen für Landstrom und Lichtmaschine, ergänzt Erzeuger-Auswahl und Maximalwerte in den Websettings sowie optionale Heizungs-Plus/Minus-Tasten. Dies ersetzt noch keine konfigurierbare Zuordnung zwischen Cerbo- und VanPi-Datenquellen. Hardware- und Langzeittests dieses zusammengeführten Stands stehen aus.

## Integration auf V1.8.0

Der lokale Entwicklungsstand heißt `V1.8.0-RV3D.91-dev`. Release V1.8.0 enthält die Home- und VE-Anpassungen unverändert gegenüber V1.7.1. Momentum-Scrolling, adaptiver Standby, Panel-Sleep, WLAN-Stromsparmodus und die neuen VanPi-Erzeuger-Endpunkte werden entsprechend Upstream übernommen. Nach dem Update sind Aufwecken, Scrollen und GX-Steuerung im Betrieb am Gerät erneut zu prüfen; die Release-Angaben zur Leistungsaufnahme wurden lokal nicht nachgemessen.
