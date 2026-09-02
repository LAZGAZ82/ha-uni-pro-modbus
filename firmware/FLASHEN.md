# Firmware bauen und flashen (interne Anleitung)

Diese Anleitung ist fuer den Betrieb gedacht (nicht fuer Endkunden). Endkunden
erhalten das Geraet fertig geflasht oder fordern die fertige Flash-Datei an.

## Variante A: PlatformIO (empfohlen, reproduzierbare Builds)

1. PlatformIO installieren (VS-Code-Erweiterung oder CLI: `pip install platformio`).
2. In diesem Ordner (`firmware/`) bauen:

   ```bash
   pio run
   ```

3. Board per USB-C anschliessen und flashen:

   ```bash
   pio run -t upload
   ```

   Falls der Port nicht automatisch gefunden wird: `pio run -t upload --upload-port /dev/ttyACM0`
   (Linux) bzw. `COM5` (Windows). Manche ESP32-S3-Boards muessen zum Flashen in den
   Bootloader-Modus: BOOT gedrueckt halten, RESET kurz druecken, BOOT loslassen.

4. Funktionskontrolle ueber den seriellen Monitor:

   ```bash
   pio device monitor
   ```

   Erwartete Ausgabe: Versionsbanner, WLAN-Status bzw. Hotspot-Start, mDNS- und
   Modbus-Meldung.

   Hinweis: Das Board meldet sich ueber das native USB des ESP32-S3 (USB-CDC,
   VID:PID 303A:1001). Die dafuer noetigen Build-Flags (ARDUINO_USB_MODE=1,
   ARDUINO_USB_CDC_ON_BOOT=1) sind in der platformio.ini gesetzt - ohne sie
   laeuft die Ausgabe auf UART0 (GPIO43/44) und der Monitor bleibt leer. Das
   Banner erscheint beim Start: nach dem Oeffnen des Monitors einmal RESET am
   Board druecken, um es erneut zu sehen.

## Fertige Flash-Datei fuer den Versand erzeugen (eine einzige .bin)

PlatformIO erzeugt Bootloader, Partitionstabelle und Anwendung getrennt. Fuer die
Weitergabe an Kunden (oder eigenes Serienflashen) alles zu einer Datei mergen:

```bash
cd .pio/build/unipro_bridge
python -m esptool --chip esp32s3 merge_bin -o unipro-bridge-v1.0.1-full.bin \
  0x0000 bootloader.bin \
  0x8000 partitions.bin \
  0x10000 firmware.bin
```

Flashen der Gesamtdatei (auch beim Kunden-Flash-Service):

```bash
python -m esptool --chip esp32s3 --port /dev/ttyACM0 write_flash 0x0 unipro-bridge-v1.0.1-full.bin
```

`esptool` kommt mit PlatformIO mit, sonst: `pip install esptool`.

## Variante B: Arduino IDE (Waveshare-offizieller Weg)

1. Boardpaket "esp32 by Espressif Systems" installieren, Board "ESP32S3 Dev Module".
2. Bibliotheken ueber den Bibliotheksverwalter installieren: **eModbus** und
   **WiFiManager** (tzapu).
3. `src/main.cpp` als Sketch oeffnen (Inhalt in eine .ino kopieren) und hochladen.

## Hinweise

- Die Firmware enthaelt bewusst keine WLAN-Zugangsdaten - die Einrichtung
  erfolgt beim Kunden ueber den Hotspot `UNI-PRO-BRIDGE` (Passwort
  `unipro-setup`). Es muss also nichts pro Kunde angepasst werden:
  **ein Image fuer alle Geraete.**
- Sollte PlatformIO die gepinnten GitHub-Tags nicht aufloesen koennen
  (Netzwerk-/Proxy-Thema), in `platformio.ini` das `#v...`-Suffix entfernen -
  dann wird der jeweils aktuelle Stand geladen. Fuer Serienfertigung danach
  wieder auf einen getesteten Stand pinnen.
- Bootschleife nach dem Flashen: in `platformio.ini` unter `[env:unipro_bridge]`
  `board_upload.flash_size = 16MB` (bzw. 4MB/8MB je nach Modul) ergaenzen und
  neu bauen/flashen.
- Nach jedem Firmware-Update vor Auslieferung einmal den Endtest fahren:
  Hotspot erscheint -> WLAN einrichten -> `ping uni-pro-bridge.local` ->
  Testlesen mit einem Modbus-TCP-Tool (siehe Kundenanleitung, Kapitel
  Fehlerbehebung).
