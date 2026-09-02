# UNI PRO Modbus – Einrichtung in Home Assistant

Schritt-für-Schritt-Anleitung, zusammengestellt aus unseren Feldtests mit
UNI2 PRO / UNI3 PRO und der Bridge (Waveshare ESP32-S3-RS485-CAN).

## Sicherheit zuerst

- Verdrahtung nur spannungsfrei. **NIEMALS 230 V an die RS485-Klemmen A/B** –
  zerstört den RS485-Treiber der Bridge und die Modbus-Schnittstelle des
  Reglers sofort.
- Beim UNI liegen die Busklemmen neben 230-V-Klemmen: vor dem Einschalten
  Belegung kontrollieren, im Zweifel Multimeter.

## 1. Verdrahtung Bridge <-> Regler (3 Adern, zwingend!)

- Bridge **A+** -> Regler **B**, Bridge **B-** -> Regler **A** (**gekreuzt!**)
- Bridge **RS485-GND** -> Regler **GND**: **ZWINGEND**. Die RS485-Seite der
  Bridge ist galvanisch getrennt; ohne GND kommt keine Antwort
  (Timeout `0xE0`, obwohl die grüne TX-LED blinkt).
- Terminierungsjumper auf der Bridge-Platine auf **"120R"** stecken.
- Verdrillte, geschirmte Leitung (z. B. J-Y(St)Y).

## 2. Modbus am Regler aktivieren

- Einstellungen **[13]** -> Gemeinsam **[13-12]** -> Kommunikation
  **[13-12-3]** -> MODBUS RTU **[13-12-3-1]**, Zugangscode **456**
- Einschalten und mit mittlerer/rechter Taste **BESTÄTIGEN**.
- Danach **Reset-Taste kurz drücken** (ohne Neustart bleibt die Schnittstelle
  stumm), anschließend im Menü kontrollieren, dass "ein" gespeichert ist.

## 3. Bridge in Betrieb nehmen

- Hotspot **"UNI-PRO-BRIDGE"**, Passwort **"unipro-setup"**, Portal
  `192.168.4.1`
- Heim-WLAN wählen; danach im Router die IP der Bridge **fest reservieren**.
- Diagnose-LEDs am RS485-Block: **grün = senden**, **blau = empfangen**.

## 4. Integration installieren (HACS)

- HACS -> Benutzerdefinierte Repositories ->
  `https://github.com/LAZGAZ82/ha-uni-pro-modbus` (Kategorie: Integration) ->
  herunterladen -> Home Assistant neu starten

## 5. Integration einrichten

- Einstellungen -> Geräte & Dienste -> Integration hinzufügen ->
  **"UNI PRO Modbus"**
- Host: die **IP-ADRESSE** der Bridge eintragen (z. B. `192.168.1.159`),
  **NICHT** `uni-pro-bridge.local` – viele HA-Installationen (Container/VM)
  lösen mDNS nicht auf; häufigste Ursache für "Bridge nicht erreichbar".
- Port **502**, Slave-ID mit dem Schieberegler **exakt auf 1** (linker
  Anschlag ist 0 – damit antwortet der Regler nicht).
- Reglermodell passend zum angeschlossenen Gerät (UNI2 PRO / UNI3 PRO),
  genutzte Heizkreise wählen.

## 6. Fehlerbehebung

| Symptom | Maßnahme |
|---|---|
| Timeout `0xE0`, TX-LED blinkt | GND-Verbindung fehlt (zwingend) oder 120R-Jumper nicht gesetzt |
| Gar keine TX-Aktivität | A/B-Kreuzung prüfen, Verkabelung |
| Regler stumm trotz Verdrahtung | Modbus [13-12-3-1] aktiv? Reset gemacht? Slave-ID 1? Zweites A/B-Klemmenpaar probieren |
| HA-Einrichtung scheitert, PC-Test klappt | IP statt `.local` eintragen, Slave-ID-Regler auf 1 |
| Bridge-Hotspot weg nach Routerwechsel | öffnet sich automatisch neu; BOOT-Taste 5 s = WLAN-Reset |
