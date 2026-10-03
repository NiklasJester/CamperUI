# Home-Feinschliff testen

Branch: jan-van-ui. Arduino IDE vor dem Ersetzen der Dateien schliessen.
Den ZIP-Inhalt in die Projekt-Arbeitskopie kopieren, vorhandene Dateien ersetzen.
Der Ordner tools bleibt tools; die vorhandene TTF-Datei wird weiterverwendet.
Den Generator muss man zum Flashen nicht ausfuehren: die erzeugten Fonts sind enthalten.

- Home hat ein Haus-Symbol; Lampen/Dimmer sind wieder eine eigene Seite.
- Acht Seiten: Home, Lampen, Energie, Wasser, Klima, Schalter, Wasserwaage, Einstellungen.
- Die untere Icon-Leiste laesst sich horizontal verschieben.
- Gewaehlte Seite wird hervorgehoben, auch beim Wischen zwischen den Seiten.
- Home verwendet feste Beispielwerte, erkennbar am kleinen Demo-Hinweis.
- Favoritentasten auf Home senden keine Befehle. Die anderen Seiten behalten ihre bestehenden Funktionen.
- Temperaturrahmen sind neutral hellgrau; Batterie ist etwas breiter als Solar.

Auf dem Display pruefen: Start ohne Neustartschleife, Haus-Symbol sichtbar,
Lampen-Seite vorhanden, seitliches Scrollen bis zu Einstellungen und zurueck,
aktive Markierung nach Tabwechsel, Wasser/Power-Animation auf richtiger Seite,
Home ohne abgeschnittene Texte.

Die vollstaendige ESP32-Kompilierung und der Hardwaretest erfolgen in Arduino IDE.
Nach erfolgreichem Test Commit und Push auf jan-van-ui, nicht fix-lvgl-config.
