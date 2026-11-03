# CLAUDE.md – Projektkontext UNI PRO Modbus

## Produkt
**UNI PRO Modbus** besteht aus zwei Teilen:

| Teil | Pfad | Version | Details |
|---|---|---|---|
| HACS-Integration (Home Assistant) | `custom_components/uni_pro_modbus/` | 0.2.1 | Manufacturer: Kaiser Electronics |
| ESP32-Bridge-Firmware | `firmware/` | 1.0.1 | Waveshare ESP32-S3-RS485-CAN, eModbus v1.7.2stable, USB-CDC-Flags gesetzt |

Zusätzlich im Repo (eigenständiges Nebenprojekt, Branch `claude/flipper-ir-remote-designer-9juneh`):

| Teil | Pfad | Version | Details |
|---|---|---|---|
| Flipper-Zero-App **IR Remote Designer** | `flipper/ir_remote_designer/` | 1.0 | FAP für Firmware 1.5.1 (API 88.2), fertiges Binary in `dist/` |

## Zielgeräte
- Euroster **UNI2 PRO** / **UNI3 PRO** (Heizungsregler)
- Modbus RTU, **9600 8N1**, **Slave-ID 1**
- Aktivierung am Regler: Menü **[13-12-3-1]**, Code **456**, danach **Reset des Reglers nötig**

## Verdrahtung (feldbestätigt)
- A/B **GEKREUZT**: Bridge A+ → Regler B (und Bridge B- → Regler A)
- **GND zwingend** verbinden (RS485-Seite der Bridge ist galvanisch isoliert)
- **120R-Terminierungsjumper** auf der Bridge gesetzt

## Bridge (ESP32)
- Erst-Setup per WLAN-Hotspot: SSID `UNI-PRO-BRIDGE`, Passwort `unipro-setup`
- mDNS: `uni-pro-bridge.local`, Modbus-TCP Port **502**
- In Home Assistant **immer die IP-Adresse** eintragen, nicht den mDNS-Namen (mDNS ist im Feld unzuverlässig)

## Flashen
```
cd firmware
pio run -t upload
pio device monitor -b 115200
```
- Board meldet sich als USB-CDC (VID:PID `303A:1001`)
- Details: `firmware/FLASHEN.md`

## Test-Werkzeuge
- `firmware/tools/` – Python-Skripte (pymodbus) und ESP32-Rohtest aus der Inbetriebnahme: RTU-Slave-Simulator, TCP-Einzel-/Dauertest, RTU-Direkttest, Adapter-Echo, RS485-Rohtest. Einsatzreihenfolge in `firmware/tools/README.md`.

## Dokumentation
- `docs/einrichtung-home-assistant.md` – Schritt-für-Schritt-Einrichtung in HA (Feldtest-Erkenntnisse)
- `docs/Anleitung_UNI3-PRO_Komplett_v3.1.pdf` – Komplettanleitung v3.1
- `docs/bridge-firmware.md` – Firmware-Beschreibung
- `docs/dashboard_example.yaml` – Beispiel-Dashboard

## Arbeitsregeln
- **Nach jeder Änderung committen UND pushen** – GitHub (`LAZGAZ82/ha-uni-pro-modbus`) ist das Backup.
- Versionsnummern bei Änderungen mitziehen: Integration in `custom_components/uni_pro_modbus/manifest.json`, Firmware im Versionsbanner in `firmware/src/`.
- Feldbestätigte Erkenntnisse (Verdrahtung, IP statt mDNS) nicht "korrigieren" – sie widersprechen bewusst der Standarderwartung.

## Flipper Zero: IR Remote Designer (`flipper/ir_remote_designer/`)
- Zweck: eigene IR-Fernbedienungen mit frei belegbaren Tasten (Layout-Designer), Signale aus `.ir`-Bibliothek oder angelernt.
- Doku: `flipper/ir_remote_designer/README.md` (Bedienung, Dateiformat, Architektur, Build).
- Bauen: `ufbt` im App-Ordner (Variante A) oder im Firmware-Quellbaum `FBT_NOENV=1 ./fbt fap_ir_remote_designer` (Variante B, siehe `build.sh`). Symlink nach `applications_user/` reicht.
- **Wichtig**: `infrared_signal_*` und `qsort` sind in der Firmware-API für FAPs gesperrt (Status `-` in `targets/f7/api_symbols.csv`) → eigene Implementierung in `ird_signal.c`, Sortierung in `ir_remote_designer.c`. Nicht "zurückbauen".
- Icons: 9x9-XBM aus `tools/gen_icons.py` → `ird_icons_data.h` (generiert, nicht von Hand editieren).
- Bedienkonzept: OK = senden (halten = wiederholen), **Zurück lang = Menü** für die markierte Taste. Raster 4 Spalten × bis 12 Zeilen, Zeilenhöhe 13 px, Kopfzeile 11 px.
- Layouts auf dem Gerät: `/ext/apps_data/ir_remote_designer/remotes/*.ird` (APP_DATA_PATH), Signale werden erst beim Tastendruck von SD geladen.
- Stand: gegen SDK gebaut und Symbol-Check bestanden; **noch nicht auf echter Hardware getestet** – Feldtest-Erkenntnisse hier eintragen.
- Bei Änderungen Version in `application.fam` (`fap_version`) und `IRD_APP_VERSION` in `ird_app_i.h` mitziehen, `.fap` in `dist/` neu bauen.
