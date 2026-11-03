# IR Remote Designer – Flipper Zero App

Eigene Infrarot-Fernbedienungen für den Flipper Zero mit frei belegbaren,
verschiebbaren und skalierbaren Tasten. Statt durch hunderte Signale einer
`.ir`-Datei zu scrollen, baust du dir pro Gerät eine eigene Fernbedienung und
weist jeder Taste genau das Signal zu, das sie senden soll.

![App-Icon](icon.png)

| | |
|---|---|
| Firmware | Offizielle Flipper-Firmware **1.5.1** (API 88.2); für andere Versionen mit `ufbt` neu bauen |
| Fertige App | `dist/ir_remote_designer.fap` |
| Speicherort auf dem Flipper | `SD/apps/Infrared/ir_remote_designer.fap` |
| Layouts | `SD/apps_data/ir_remote_designer/remotes/*.ird` |
| Signal-Bibliothek | `SD/infrared/**/*.ir` (eigene Remotes + Universal-Bibliothek `infrared/assets`) |

## Funktionen

- **11 Gerätekategorien** mit passender Tastenvorlage: Fernseher, Beamer,
  Klimaanlage, DVD/Blu-ray, Musikanlage, Soundbar, AV-Receiver,
  LED-Beleuchtung, Ventilator, Präsentation, Sonstiges.
- **Signal pro Taste zuweisen** aus jeder vorhandenen `.ir`-Datei
  (Dateibrowser → Signalliste → fertig). Das Signal wird in die Layout-Datei
  kopiert, die Fernbedienung funktioniert danach unabhängig von der Quelle.
- **Signal anlernen** direkt von der Original-Fernbedienung (RX über die
  IR-Hardware, dekodiert oder RAW).
- **Tasten bearbeiten**: hinzufügen, löschen, umbenennen, verschieben,
  Größe ändern (1–4 Spalten × 1–2 Zeilen), 45 eingebaute Symbole.
- **Senden mit Halten**: OK kurz = einmal, OK halten = Wiederholung
  (wie die offizielle Infrarot-App). LED blinkt beim Senden.
- **Mehrere Fernbedienungen**, alle dauerhaft auf der SD-Karte gespeichert.
- Geringer RAM-Bedarf: Signale werden erst beim Tastendruck von der SD-Karte
  geladen (wie in der offiziellen App), im RAM liegt nur das Layout
  (< 3 KB).

## Bedienung

| Taste | Fernbedienungsansicht |
|---|---|
| Pfeile | Taste auswählen (räumliche Navigation im Raster) |
| OK | Signal senden, halten = wiederholen |
| Zurück kurz | Fernbedienung verlassen |
| **Zurück lang** | **Menü** für die markierte Taste / die Fernbedienung |

Kopfzeile: links Name der Fernbedienung, rechts Beschriftung der markierten
Taste. Ein `?` in der Taste bedeutet: noch kein Signal zugewiesen.

### Menü (Zurück lang)

- Signal aus Bibliothek – `.ir`-Datei wählen, dann Signal wählen
- Signal anlernen – Original-Fernbedienung auf den Flipper richten
- Signal entfernen
- Umbenennen / Symbol ändern / Größe ändern
- Verschieben – Pfeile schieben die Taste im Raster, OK = fertig
- Taste löschen
- Neue Taste, Fernbedienung umbenennen, Fernbedienung löschen

### Neue Fernbedienung

Hauptmenü → `+ Neue Fernbedienung` → Kategorie → Name. Die Vorlage wird
angelegt; danach jeder Taste per Menü ein Signal zuweisen.

## Installation

1. `dist/ir_remote_designer.fap` nach `apps/Infrared/` auf die SD-Karte
   kopieren (qFlipper → Dateimanager, oder SD-Karte direkt).
2. Auf dem Flipper: `Apps` → `Infrared` → `IR Remote Designer`.

Alternativ mit angeschlossenem Flipper: `ufbt launch` im App-Ordner.

## Selbst bauen

```bash
pip install ufbt
cd flipper/ir_remote_designer
ufbt update      # SDK passend zur aktuellen Release-Firmware laden
ufbt             # baut dist/ir_remote_designer.fap
ufbt launch      # optional: auf den Flipper kopieren und starten
```

Ohne Zugriff auf `update.flipperzero.one` (z. B. hinter Proxy) kann die App
auch direkt im Firmware-Quellbaum gebaut werden, siehe `build.sh`
(Variante B). Genau so wurde das beiliegende `.fap` erzeugt:
Firmware-Tag `1.5.1`, `FBT_NOENV=1 ./fbt fap_ir_remote_designer`,
System-Toolchain `arm-none-eabi-gcc 13.2`.

## Dateiformat der Layouts (`.ird`)

Flipper-Format-Textdatei, Signalblöcke sind 1:1 das Format der offiziellen
`.ir`-Dateien:

```
Filetype: IR Remote Designer Layout
Version: 1
Remote: Wohnzimmer_TV
Category: 0

Button: Power
Icon: 1
Pos: 0 0 1 1
Source: Samsung_TV/Power
Signal: 1
#
name: Power
type: parsed
protocol: NEC
address: 07 00 00 00
command: 02 00 00 00
```

`Pos` = Spalte, Zeile, Breite, Höhe im 4-Spalten-Raster. `Signal: 0` = Taste
ohne Signal (dann folgt kein Signalblock).

## Technik / Aufbau

```
application.fam          Manifest (appid ir_remote_designer, Kategorie Infrared)
ir_remote_designer.c     Einstieg, ViewDispatcher/SceneManager, App-Struktur
ird_app_i.h              App-Kontext, View-IDs, Custom-Events
ird_layout.[ch]          Datenmodell + Laden/Speichern der .ird-Dateien
ird_signal.[ch]          IR-Signal lesen/schreiben (.ir-kompatibel), Beschreibung
ird_ir.c                 Senden (InfraredWorker TX) und Anlernen (RX)
ird_templates.[ch]       Kategorien und Tastenvorlagen
ird_icons.[ch]           9x9-XBM-Symbole (generiert aus tools/gen_icons.py)
views/ird_remote_view.*  Eigene Raster-Ansicht (Zeichnen, Navigation, Verschieben)
scenes/                  Start, Kategorie, Name, Remote, Tastenmenü, Dateiwahl,
                         Signalwahl, Anlernen, Ergebnis, Symbol, Größe,
                         Löschen, Meldung, Hilfe
images/                  Icon für den Dateibrowser (fap_icon_assets)
```

Wichtige Entscheidungen:

- Die Firmware-Funktionen `infrared_signal_*` sind für externe Apps
  **nicht freigegeben** (API-Status `-`), ebenso `qsort`. Deshalb eigenes
  `ird_signal.c` auf Basis von `flipper_format_*` und `infrared_*`
  (Encoder/Decoder, Worker), die freigegeben sind.
- Senden über `infrared_worker_tx_*` mit „steady“-Callback: solange OK
  gehalten wird, wiederholt der Worker das Signal.
- Beim Speichern wird die Layout-Datei komplett neu geschrieben
  (`.tmp` → umbenennen); vorhandene Signale werden sequentiell aus der alten
  Datei übernommen, damit auch lange RAW-Signale nie im RAM gehalten
  werden müssen.
- Raster: 4 Spalten × 31 px, Zeilen 13 px, 4 Zeilen sichtbar, bis zu 12
  Zeilen (scrollt), max. 40 Tasten pro Fernbedienung.

## Hinweise

- Nur Geräte steuern, zu deren Bedienung du berechtigt bist.
- Universal-Dateien (`infrared/assets/*.ir`) enthalten pro Tastenname sehr
  viele Varianten; die Liste zeigt die ersten 250 Einträge, gleiche Namen
  werden nummeriert. Eigene Remote-Dateien sind der sinnvollere Weg.
- Die App wurde hier gegen das SDK gebaut und statisch geprüft
  (Symbol-Check gegen API 88.2); ein Test auf echter Hardware stand in dieser
  Umgebung nicht zur Verfügung.
- `tools/hosttest/run.sh` rendert die Raster-Ansicht mit allen Vorlagen auf
  dem PC (Shim statt Firmware) als PNG nach `tools/hosttest/out/` und prüft
  Navigation/Verschieben – praktisch für Layout-Änderungen ohne Gerät.
