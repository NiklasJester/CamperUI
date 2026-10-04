# Navigation v8 auf dem Display pruefen

Dieser Stand hat neun Hauptseiten: Home, Licht, Energie, Wasser, Klima,
MaxxFan, Schalter, Wasserwaage und Einstellungen. Die untere Leiste wird
nur mit dem Finger horizontal verschoben. Antippen aendert die Seite,
ohne die Leiste automatisch zu verschieben. Die Hauptseiten selbst haben
keine horizontale Wischnavigation.

## Vorbereitung

- Den vollstaendigen aktuellen Sketch kompilieren und auf das Display laden.
- Seriellen Monitor mit 115200 Baud oeffnen. Die Startausgabe muss
  `[NAV v8] init` enthalten; so ist die geflashte Version erkennbar.
- Erwartete Werte: `faults=0`, `nav=(0,420) 480x60`,
  `scroll=(...,0)`, `content=(0,40) 480x380`.
- Fuer den unbeaufsichtigten Test den Display-Timeout ausreichend hoch setzen.

## Zuerst ohne Touch

1. Home mindestens drei Minuten geoeffnet lassen, ohne das Display zu beruehren.
2. Die Leiste muss durchgehend unten bleiben und darf den Inhalt nicht ueberlagern.
3. Falls sie springt, im seriellen Monitor `n` senden. Das gibt die gemessene
   Geometrie, freien LVGL-Speicher und einen LVGL-Pool-Integritaetstest aus.
4. Automatische `[NAV v8] before-data`/`after-data`-Meldungen erscheinen nur
   bei einer erkannten Abweichung oder der Rueckkehr zum Normalzustand.
   Die Kontrolle liest Koordinaten und setzt die Leiste nicht zurueck.

## Anschliessend Bedienung

1. Leiste in beide Richtungen verschieben; nach dem Loslassen muss sie stoppen.
2. Jede der neun Seiten mehrfach auswaehlen, auch teilweise sichtbare Randbuttons.
   Kein selbststaendiges Nachscrollen, keine vertikale Bewegung, genau eine
   aktive Markierung. Ein Ziehen darf keine andere Seite auswaehlen.
3. Home und MaxxFan vertikal scrollen. Die untere Leiste bleibt bei y=420.
4. System-Einstellungen oeffnen und zurueckkehren.
5. Farbmodus mehrmals wechseln. Aktive Hauptseite und horizontale
   Leistenposition bleiben erhalten; Symbole behalten ihre Groesse.
6. MaxxFan AUF/ZU sowie EIN/AUS pruefen. Home-Favoriten und MaxxFan bleiben Demos.

## Wenn die Leiste trotzdem falsch aussieht

- `faults` ungleich 0 oder y ungleich 420: tatsaechliche Geometrieabweichung.
  Ausgabe vor/nach dem Springen sichern.
- Leiste optisch falsch, aber `faults=0` und `nav=(0,420) 480x60`:
  Der LVGL-Container liegt richtig. Dann Anzeige-/Renderpfad untersuchen;
  allein aus diesem Ergebnis folgt noch kein bestimmter Treiberfehler.
- `LVGL pool integrity: FAILED`: Speicherverwaltung des LVGL-Pools ist beschaedigt.
  `OK` schliesst andere Speicherfehler oder Nebenlaeufigkeitsfehler nicht aus.

Erst nach bestandenem Ruhe- und Bedienungstest ist die Hardwarestabilitaet
bestaetigt. Ein erfolgreicher Build allein bestaetigt sie nicht.
