# ESP32 als Vektordisplay-Quelle: Lissajous-Figuren, Vektorgrafik und Text am Oszilloskop

Drei kleine Arduino-Sketches, die mit den beiden internen DACs eines ESP32 ein Oszilloskop
im X-Y-Betrieb ansteuern:

| Sketch | Inhalt |
|---|---|
| `Lissajous_DAC` | Zwei Sinussignale mit einstellbarer Frequenz und Phase: Kreis, Achten, Schleifenfiguren |
| `Vektor_DAC` | Zeichnet eine Figur aus einer Punktliste (Beispiel: „Haus vom Nikolaus") |
| `Text_DAC` | Schreibt einen Text in Strichschrift |

Die Sketches bauen aufeinander auf und kommen ohne Zusatzbauteile und ohne Bibliotheken aus.

## Hardware-Voraussetzungen

| Teil | Anforderung |
|---|---|
| ESP32-Board | Klassischer ESP32 mit zwei DAC-Ausgängen, z. B. DevKit mit ESP-WROOM-32 (30 oder 38 Pins) |
| Oszilloskop | Zwei Kanäle und X-Y-Betrieb. Getestet mit einem Fluke 199C ScopeMeter (dort heißt der Modus „A vs B") |
| Tastköpfe | Zwei Stück, 1:1 oder 10:1 |
| USB-Kabel | Zum Flashen |
| USB-Netzteil oder Powerbank | Empfohlen für den Betrieb, siehe [Fehlersuche](#fehlersuche) |

Hinweise zur Board-Wahl:

- **ESP32-S3, ESP32-C3, ESP32-C6** haben keinen DAC und funktionieren mit diesen Sketches nicht.
- **ESP32-S2** hat zwei DACs, aber auf GPIO17 und GPIO18. Die Pin-Konstanten in den Sketches
  müssen dann angepasst werden. Nicht getestet.

## Software-Voraussetzungen

- Arduino IDE
- Board-Paket „esp32" von Espressif (Boardverwalter)
- Board-Auswahl: **ESP32 Dev Module** (alternativ „DOIT ESP32 DEVKIT V1").
  Nicht „ESP32-WROOM-DA Module" wählen, das ist ein anderes Modul mit zwei Antennen.

## Ordnerstruktur

Die Arduino IDE verlangt, dass jeder Sketch in einem Ordner mit gleichem Namen liegt:

```
├── Lissajous_DAC/
│   └── Lissajous_DAC.ino
├── Vektor_DAC/
│   └── Vektor_DAC.ino
├── Text_DAC/
│   └── Text_DAC.ino
└── README.md
```

## Verdrahtung

| ESP32-Pin (Aufdruck) | Funktion | Oszilloskop |
|---|---|---|
| D25 (GPIO25) | DAC 1, vertikale Ablenkung (Y) | Kanal A / CH1, Tastspitze |
| D26 (GPIO26) | DAC 2, horizontale Ablenkung (X) | Kanal B / CH2, Tastspitze |
| GND | Masse | Masseklemme beider Tastköpfe |

Bei Geräten mit voneinander isolierten Eingängen (wie dem Fluke 199C) braucht jeder Tastkopf
seine eigene Masseverbindung zum ESP32.

Die Ausgangsspannung liegt zwischen rund 0,1 V und 3,2 V, also etwa 3 Vss mit 1,65 V Gleichanteil.

## Einstellungen am Oszilloskop

| Einstellung | Wert |
|---|---|
| Betriebsart | X-Y |
| Empfindlichkeit | 500 mV/div oder 1 V/div auf beiden Kanälen |
| Kopplung | AC, oder DC und das Bild mit den Positionsreglern in die Mitte schieben |
| Zeitbasis (nur Digitaloszilloskope) | So wählen, dass eine Erfassung mindestens eine ganze Periode bzw. ein ganzes Bild enthält. Am Fluke 199C: 5 ms/div |
| Persistence | Aus |

**Fluke 199C:** `SCOPE` → `WAVEFORM OPTIONS` → `Waveform: Mathematics...` → `Function: A vs B`.
Eingang A liegt auf der vertikalen, Eingang B auf der horizontalen Achse.

**Andere Oszilloskope:** Meist ist CH1 die X-Achse und CH2 die Y-Achse, also umgekehrt.
Das Bild erscheint dann an der Diagonale gespiegelt. Abhilfe: die beiden Tastköpfe tauschen.

Bei einem Digitaloszilloskop ist die Zeitbasis entscheidend: Ist sie zu schnell, landet pro
Erfassung nur ein kurzes Stück der Figur am Schirm, und man sieht nichts oder nur Bruchstücke.

## Die Sketches

Alle drei Sketches geben mit 10 kHz Abtastrate aus (`SAMPLE_US = 100`). Die einstellbaren
Werte stehen jeweils oben im Sketch unter „Einstellungen".

### 1. `Lissajous_DAC`

Erzeugt zwei Sinussignale per DDS (Sinustabelle mit 256 Werten, 32-Bit-Phasenakkumulator
je Kanal).

| Parameter | Bedeutung | Vorgabe |
|---|---|---|
| `FREQ_A_HZ` | Frequenz an D25 (vertikal) | 100 |
| `FREQ_B_HZ` | Frequenz an D26 (horizontal) | 100 |
| `PHASE_B_DEG` | Phasenverschiebung von B, in Grad der B-Schwingung | 90 |
| `AMPLITUDE` | Aussteuerung um die Mitte, 0 bis 127 | 120 |

Erprobte Werte:

| `FREQ_A_HZ` | `FREQ_B_HZ` | `PHASE_B_DEG` | Figur |
|---|---|---|---|
| 100 | 100 | 90 | Kreis |
| 100 | 100 | 0 | Diagonale Linie |
| 200 | 100 | 90 | Liegende Acht |
| 200 | 100 | 45 | Parabel (die Acht fällt zu einem Bogen zusammen) |
| 100 | 200 | 0 | Stehende Acht |
| 100 | 200 | 90 | Liegende Parabel |
| 300 | 200 | 0 | Drei Schleifen nebeneinander, zwei übereinander |
| 200.2 | 100 | 90 | Acht, die sich in 5 Sekunden einmal scheinbar räumlich dreht |

Faustregeln:

- Die Zahl der Schleifen nebeneinander zu jener übereinander entspricht dem Verhältnis
  `FREQ_A_HZ` zu `FREQ_B_HZ`.
- Die Phase bestimmt, ob die Figur offen oder zu einer Linie zusammengefallen erscheint.
- Weicht das Frequenzverhältnis leicht von einem ganzzahligen Wert ab, dreht sich die Figur.
  Die Abweichung in Hertz ist die Zahl der Umläufe pro Sekunde.
- Je höher die Frequenz, desto weniger Stützpunkte pro Periode (bei 300 Hz noch 33).
  Unter 500 Hz bleiben die Figuren ansehnlich.

### 2. `Vektor_DAC`

Fährt eine Liste von Eckpunkten ab und verbindet sie mit Linien. Das Beispiel ist das
„Haus vom Nikolaus": Quadrat mit beiden Diagonalen und Dach, alle acht Linien in einem Zug.
Dadurch gibt es keine Sprünge zwischen getrennten Teilen.

| Parameter | Bedeutung | Vorgabe |
|---|---|---|
| `PATH[]` | Eckpunkte `{x, y}`, je 0 bis 255, Ursprung links unten | Haus vom Nikolaus |
| `STEP` | Schrittweite je Abtastwert in DAC-Einheiten. Kleiner ergibt dichtere Linien, aber längere Bildzeit | 3.0 |
| `CORNER_DWELL` | Zusätzliche Abtastwerte an jedem Eckpunkt, damit die Ecken scharf bleiben | 2 |

Nach dem letzten Punkt läuft der Strahl zurück zum ersten. Ein Durchlauf dauert mit den
Vorgabewerten 44 ms.

Eigene Figuren: `PATH[]` durch eigene Punkte ersetzen. Am besten eignen sich Figuren, die
sich in einem Zug zeichnen lassen.

### 3. `Text_DAC`

Schreibt einen Text in einer einfachen Strichschrift. Zwischen den Zeichen springt der
Strahl in einem einzigen Schritt, sodass ohne Dunkeltastung (Z-Eingang) keine sichtbaren
Verbindungslinien entstehen.

| Parameter | Bedeutung | Vorgabe |
|---|---|---|
| `TEXT[]` | Anzuzeigender Text | `"Joe"` |
| `STEP` | Schrittweite je Abtastwert in DAC-Einheiten | 2.0 |
| `CORNER_DWELL` | Zusätzliche Abtastwerte an jedem Eckpunkt | 2 |
| `MAX_SCALE` | Größte Zeichengröße in DAC-Einheiten je Rasterpunkt | 16.0 |

- **Zeichensatz:** A–Z, a–z, 0–9, Leerzeichen und `. - ! : / +`. Unbekannte Zeichen werden
  als Leerzeichen ausgegeben.
- **Größe:** Der Text wird automatisch auf den DAC-Bereich skaliert und zentriert.
- **Bildzeit:** Wird einmal nach dem Start am seriellen Monitor ausgegeben (115200 Baud).
  Für „Joe" sind es 35,5 ms. Etwa 8 bis 10 Zeichen passen in 60 ms, bei längeren Texten
  die Zeitbasis am Oszilloskop verlängern oder `STEP` vergrößern.

Aufbau der Schrift: Jedes Zeichen liegt in einem Raster mit x = 0 bis 4 und y = 0 bis 8
(y = 2 Grundlinie, y = 6 Höhe der Kleinbuchstaben, y = 8 Höhe der Großbuchstaben,
y = 0 Unterlänge). Ein Zeichen ist eine Zeichenkette aus Punkten mit je zwei Ziffern `xy`,
das Zeichen `|` bedeutet Absetzen. Beispiele:

| Zeichen | Kodierung | Bedeutung |
|---|---|---|
| L | `"080242"` | (0,8) → (0,2) → (4,2) |
| T | `"0848|2822"` | (0,8) → (4,8), absetzen, (2,8) → (2,2) |

Eigene Zeichen lassen sich nach diesem Muster in `glyphFor()` ergänzen.

## Grenzen

- **Auflösung:** Die internen DACs haben 8 Bit, Stufen sind bei genauem Hinsehen sichtbar.
- **Geschwindigkeit:** 10 kHz Ausgaberate über `dacWrite()`. Das begrenzt die Zahl der Punkte
  pro Bild und damit Textlänge und Detailgrad.
- **Keine Dunkeltastung:** Sprünge sind nur deshalb unsichtbar, weil sie sehr schnell
  erfolgen. Auf einem analogen Röhrenoszilloskop können sie als schwache Linien erscheinen.
- **Digitaloszilloskope** zeigen ein abgetastetes Bild. Es wirkt statischer und gröber als
  auf einer Röhre, und Animationen laufen ruckelig.

## Fehlersuche

| Symptom | Ursache und Abhilfe |
|---|---|
| Nichts oder nur Bruchstücke zu sehen | Zeitbasis zu schnell. Auf 5 ms/div oder langsamer stellen |
| Schräge Linie statt Kreis | Beide Signale sind phasengleich. `PHASE_B_DEG` auf 90 stellen |
| Figur abgeschnitten oder nicht in der Mitte | Gleichanteil. AC-Kopplung verwenden oder die Lage verschieben |
| Figur zehnmal zu groß oder zu klein | Tastkopf-Einstellung (1:1 / 10:1) am Oszilloskop passt nicht zum Tastkopf |
| Alle paar Sekunden Störungen neben der Figur, die LED am Board blinkt dabei | Der ESP32 startet neu, weil ein Programm am PC den COM-Port öffnet (siehe unten) |
| Upload startet nicht | Bei „Connecting…" die BOOT-Taste am Board gedrückt halten |

**Neustarts am PC:** Auf den DevKit-Boards hängt der Reset-Pin über eine Auto-Reset-Schaltung
an den Leitungen DTR und RTS des USB-Seriell-Wandlers. Öffnet irgendein Programm den COM-Port
(Geräte-Software, Slicer, Funk- oder Terminal-Programme mit automatischer Portsuche), startet
der ESP32 neu. Während des Bootens sind die DAC-Pins hochohmig und liefern Störungen.
Abhilfe: den ESP32 an einem USB-Netzteil oder einer Powerbank betreiben, oder den seriellen
Monitor der Arduino IDE geöffnet lassen, der den Port belegt.

## Stand

Alle drei Sketches wurden auf einem ESP32-DevKit mit ESP-WROOM-32 an einem Fluke 199C
ScopeMeter erprobt.
