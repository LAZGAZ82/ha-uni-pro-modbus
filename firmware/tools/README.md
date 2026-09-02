# Test-Werkzeuge für die Inbetriebnahme

Kleine Hilfsprogramme, die bei der Inbetriebnahme der Bridge am UNI3 PRO
entscheidend waren. Sie erlauben, die Kette Home Assistant -> Bridge (TCP) ->
RS485 -> Regler Stück für Stück zu prüfen und den Fehler einzugrenzen.

Voraussetzung für die Python-Skripte: `pip install pymodbus pyserial`.
COM-Port, IP-Adresse und Hostname sind im Kopf der Skripte hart eingetragen
und müssen an die eigene Umgebung angepasst werden.

| Datei | Zweck | Wann einsetzen |
|---|---|---|
| `rtu_slave.py` | **RTU-Slave-Simulator**: Der PC spielt über einen USB-RS485-Adapter (COM3) den Regler (Unit 1, 9600 8N1, Input-Register liefern 123, Holding-Register 456). | Bridge ohne echten Regler testen. Antwortet die Bridge per TCP mit 123, stimmen Firmware, RS485-Richtungssteuerung und Verdrahtung Bridge <-> Adapter. |
| `tcp_test.py` | **TCP-Einzeltest**: Liest ein Input-Register über die Bridge (Modbus TCP, Port 502) und gibt Antwort bzw. Exception-Code aus. | Erster Test nach dem WLAN-Setup der Bridge. Hostname bei Bedarf durch die IP ersetzen (mDNS ist im Feld unzuverlässig). |
| `tcp_loop.py` | **TCP-Dauertest**: Liest jede Sekunde ein Register über die Bridge, bis Strg+C. | Stabilität von WLAN und RS485 über längere Zeit prüfen, Aussetzer erkennen. |
| `rtu_direkt.py` | **RTU-Direkttest**: Der PC fragt den Regler direkt über den USB-RS485-Adapter ab, ohne Bridge. | Klärt, ob der Regler überhaupt antwortet (Modbus am Regler aktiviert? A/B vertauscht? GND verbunden?). |
| `adapter_echo.py` | **Adapter-Echo-Test**: Sendet einen rohen RTU-Frame (Unit 1, FC04, Register 0) über pyserial und zeigt die empfangenen Rohbytes. | Unterste Ebene: Prüft, ob der USB-RS485-Adapter überhaupt etwas empfängt, unabhängig von pymodbus. |
| `rs485_raw_test.ino` | **RS485-Rohtest auf dem ESP32** (Arduino-IDE-Sketch): Kein eModbus, kein WLAN. Sendet alle 3 s eine RTU-Anfrage über GPIO17/18/21 und gibt die Rohantwort auf dem seriellen Monitor aus. | Verdrahtung Bridge <-> Regler prüfen, bevor die eigentliche Firmware ins Spiel kommt. Damit wurde die gekreuzte A/B-Belegung gefunden. |
| `rs485_raw_test.cpp` | Gleicher Rohtest als PlatformIO-Variante (`src/main.cpp` in einem leeren esp32-s3-devkitc-1-Projekt mit USB-CDC-Flags). | Wie oben, für den PlatformIO-Workflow. |

## Typische Reihenfolge bei „es kommt nichts zurück"

1. `adapter_echo.py` bzw. `rtu_direkt.py` direkt am Regler: Antwortet der Regler?
   Wenn nein: Modbus am Regler aktivieren (Menü 13-12-3-1, Code 456, Reset),
   A/B tauschen, GND verbinden.
2. `rtu_slave.py` am PC plus `tcp_test.py` gegen die Bridge: Funktioniert die
   Bridge mit einem bekannten Gegenüber?
3. `rs485_raw_test.ino` auf der Bridge am Regler: Kommt auf dem seriellen
   Monitor eine Rohantwort?
4. `tcp_loop.py`: Läuft die komplette Kette stabil?
