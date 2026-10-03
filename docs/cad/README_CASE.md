# CamperUI Gehäuse & Wandmontage 🚐📐

Gehäuselösungen für das **Waveshare ESP32-S3-Touch-LCD-4** (4.0 Zoll, 480×480 Pixel).  
Entwickelt für eine extrem flache, professionelle Wandintegration im Campervan (z. B. Möbelbau, Schrankseitenwand, Trennwand).

---

## 🔍 Maße & Technische Referenz des Displays

* **Glasabmessungen (Außen):** `84.20 mm × 84.20 mm` (Dicke ca. 1.2 mm)
* **Aktive Touch-/Displayfläche:** `71.86 mm × 70.18 mm`
* **Platinenabmessungen (PCB):** `74.50 mm × 76.50 mm`
* **Lochabstände der 4 Befestigungsbohrungen:**
  * **Horizontal:** `66.50 mm` (Mitte-zu-Mitte)
  * **Vertikal:** `68.50 mm` (Mitte-zu-Mitte)
  * **Schrauben:** M2.5 (auf der Platine sind ab Werk 4 mm hohe M2.5-Gewindebuchsen vorhanden)
* **Wichtig:** Die Glasfront überragt die Platine seitlich um ca. `4.85 mm` und oben/unten um `3.85 mm`!
* **Gesamttiefe der Baugruppe:** ca. `14.5 – 15.0 mm` (vom Glas bis zur Unterkante der grünen 10-Pin Schraubklemme).

```text
                  84.20 mm (Glas)
      +-------------------------------------+
      |                                     |
      |       +---------------------+       |
      |       |                     |       |
      |  (o)  |    Aktives LCD      |  (o)  |
      |   |   |    71.9 x 70.2 mm   |   |   |
      |   |   |                     |   |   |
      |   |   +---------------------+   |   |
68.50 |   |                             |   |
 mm   |   |                             |   |
      |  (o)  [===== Klemmen =====]    (o)  |
      |       +---------------------+       |
      +-------------------------------------+
                  66.50 mm (Lochabstand)
```

---

## 🛠️ Zwei Montagekonzepte

### Variante 1: "Flush-Mount" (Wandeinbau – Empfohlen!) ⭐
> **Das Display verschwindet in der Wand. Der Wandüberstand beträgt nur ca. 3 mm!**

* **Funktionsweise:** Die Elektronik (PCB, Klemmenblock, ESP32) sitzt im Wandausschnitt. Vorne schließt nur das Glas mit einem zarten Schutzrahmen ab.
* **Wandausschnitt:**
  * **Option A (Rechteck):** `78 mm × 80 mm` (z. B. mit Multitool / Stichsäge)
  * **Option B (Rundloch):** `Ø 80 mm` (Standard-Lochsäge)
* **Wandstärke:** Geeignet für Holzwände, Multiplex und Leichtbauplatten ab 8 mm Stärke.
* **Befestigung:**
  1. Der 3D-Druck-Einbaurahmen (`flush_mount_frame.stl`) wird in den Ausschnitt eingesetzt und mit 4 kleinen Senkkopfschrauben (z. B. 2.5 × 12 mm Holzschrauben) im Holz verschraubt.
  2. Die 12V-Zuleitung wird an die grüne Klemme angeschlossen (`VIN` und `GND`).
  3. Das Waveshare-Display wird von vorne in den Rahmen eingesetzt und mit 4 Schrauben M2.5 × 6 mm fixiert.
  4. Der magnetische oder geclipste Front-Bezel (`front_bezel.stl`) wird aufgedrückt und deckt die Schraubenköpfe sowie den Wandspalt unsichtbar ab.

---

### Variante 2: "Ultra-Slim Surface-Mount" (Flaches Aufputzgehäuse)
> **Falls in der Wand nur eine Kabeldurchführung (Loch) gebohrt werden soll.**

* **Wandüberstand:** `16.0 mm`
* **Wandbohrung:** Ein einfaches Loch von `Ø 25 – 35 mm` reicht aus, um das 12V-Kabel (und ggf. den Klemmenblock) durch die Wand zu führen.
* **Befestigung:**
  1. Das Gehäuseunterteil (`surface_mount_case.stl`) wird mit 2 Senkkopfschrauben von innen an die Wand geschraubt.
  2. Kabel durch die Wandöffnung führen und anklemmen.
  3. Display mit 4 Schrauben M2.5 im Gehäuse verschrauben.
  4. Frontrahmen aufclipsen.

---

## ⚡ Stromversorgung im Campervan (12V Direktanschluss!)

> [!TIP]
> Das Waveshare-Board benötigt **kein** separates 5V-USB-Netzteil!
> Auf dem grünen 10-Pin Klemmenblock befindet sich ein integrierter DC/DC-Schaltregler für **DC 7V bis 36V**:
> * **Pin 1 (`VIN`):** 12V / 24V Bordnetz (über 1A Sicherung absichern)
> * **Pin 2 (`GND`):** Masse
>
> Dadurch genügt ein einfaches 2-adriges dünnes Kabel (z. B. 2 × 0.75 mm² oder 2 × 0.5 mm²) direkt hinter der Wand.

---

## 🖨️ 3D-Druck Empfehlungen

| Parameter | Empfohlener Wert |
| :--- | :--- |
| **Material** | **PETG** oder **ABS/ASA** (wärme- und UV-beständig im Fahrzeug, kein reines PLA wegen Sommerhitze!) |
| **Schichthöhe** | `0.16 mm` oder `0.20 mm` |
| **Infill** | `25% - 40%` (Gyroid oder Wabe) |
| **Wände (Perimeter)**| `4 Wände` für stabile Gewinde |
| **Support** | Meist **ohne Support** druckbar (Flachseite auf das Druckbett legen) |

---

## 📁 Verfügbare CAD- & Druckdateien in diesem Ordner

1. `case_flush_mount.scad`: Parametrischer OpenSCAD-Code für den Wandeinbau.
2. `case_surface_mount.scad`: Parametrischer OpenSCAD-Code für das Aufputzgehäuse.
3. `flush_mount_frame.stl`: Druckfertiger Einbaurahmen für die Wand.
4. `surface_mount_case.stl`: Druckfertiges flaches Aufputzgehäuse.
5. `front_bezel.stl`: Eleganter Zier- und Abdeckrahmen.
