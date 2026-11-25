# UNI PRO Modbus – Einrichtung in Home Assistant

Schritt-für-Schritt-Anleitung, zusammengestellt aus unseren Feldtests mit
UNI2 PRO / UNI3 PRO und der Bridge (Waveshare ESP32-S3-RS485-CAN).
Stand: Integration 0.3.1, Kunden-Anleitung v1.5
(`docs/Anleitung_HomeAssistant_UNI2-UNI3-PRO_v1.5.pdf`).

## Sicherheit zuerst

- Verdrahtung nur spannungsfrei. **NIEMALS 230 V an die RS485-Klemmen A/B** –
  zerstört den RS485-Treiber der Bridge und die Modbus-Schnittstelle des
  Reglers sofort.
- Beim UNI liegen die Busklemmen neben 230-V-Klemmen: vor dem Einschalten
  Belegung kontrollieren, im Zweifel Multimeter.

## 1. Verdrahtung Bridge <-> Regler

- Bridge **A+** -> Regler **B**, Bridge **B-** -> Regler **A** (**gekreuzt!**)
- Regler **GND** -> Bridge **GND (−V) der Versorgungsklemme**: **empfohlen,
  nicht zwingend**. In einer früheren Installation (UNI3 PRO) war die
  Verbindung nötig, im Test 11/2026 nicht (Gegentest 24.11.2026: Bridge-GND
  abgeklemmt, 3 Minuten fehlerfrei). RS485-Seite laut Schaltplan galvanisch
  getrennt; die Klemme hat keinen eigenen GND. Eine Verbindung Regler-GND ↔ −V
  kann bei Masseproblemen helfen.
- Terminierungsjumper auf der Bridge-Platine auf **"120R"** stecken.
- Verdrillte, geschirmte Leitung (z. B. J-Y(St)Y).

### Mit UNIM-PRO-Erweiterungsmodul (CO4–CO7)

- DIP-Schalter am Modul ab Werk alle OFF; Schalter 1 → CO4, 2 → CO5,
  3 → CO6, 4 → CO7.
- **UNIM PRO gerade** (A→A, B→B), **Bridge gekreuzt** – beides am
  RS485-Klemmenblock des Reglers (Grafik: `docs/bilder/verdrahtung-unim-bridge.png`,
  Quelle `.svg` daneben).
- **GND Regler ↔ UNIM PRO: zwingend** (Herstellervorgabe; im Feldtest 11/2026
  Ursache des Totalausfalls – die GND fehlte zwischen Regler und Modul).
- GND Regler ↔ Bridge (−V): empfohlen, nicht zwingend (siehe oben).
- Am Regler unter [13-12-3] das Modul aktivieren, Modbus an lassen, Regler aus- und wieder einschalten.
  Danach **ca. 1 Minute** bis zur ersten Antwort (vorher nur `0xE0`).

## 2. Modbus am Regler aktivieren

- Einstellungen **[13]** -> Gemeinsam **[13-12]** -> Kommunikation
  **[13-12-3]** -> MODBUS RTU **[13-12-3-1]**, Zugangscode **456**
- Einschalten und mit mittlerer/rechter Taste **BESTÄTIGEN**.
- Danach den **Regler aus- und wieder einschalten** (ohne Neustart bleibt die
  Schnittstelle stumm; eine Reset-Taste hat der Regler nicht), anschließend im
  Menü kontrollieren, dass "ein" gespeichert ist.

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
  genutzte Heizkreise wählen – **nur Kreise, die am Regler aktiv sind**; die
  Auto-Erkennung hakt sie vor (inaktive Kreise antworten normal mit „aus“,
  Gerätetest 24.11.2026). Antwortet ein Register gar nicht (`0xE0`, z. B. bei
  einem Busproblem), macht das seit 0.3.1 nur die betroffenen Entitäten
  „nicht verfügbar“, der Rest läuft weiter. Abgewählte Kreise verschwinden
  beim Neuladen aus der Entitätenliste.
- Optionen: Pause zwischen Anfragen (Standard 50 ms), nur bei sporadisch
  fehlenden Werten erhöhen.

## 6. Fehlerbehebung

| Symptom | Maßnahme |
|---|---|
| Timeout `0xE0`, TX-LED blinkt | Mit UNIM PRO: GND Regler ↔ Modul prüfen (zwingend). GND Regler ↔ Bridge (−V) klemmen (hilft bei Masseproblemen). 120R-Jumper gesetzt? |
| Gar keine TX-Aktivität | A/B-Kreuzung prüfen, Verkabelung |
| Regler stumm trotz Verdrahtung | Modbus [13-12-3-1] aktiv? Regler danach aus- und wieder eingeschaltet? Slave-ID 1? Zweites A/B-Klemmenpaar probieren |
| HA-Einrichtung scheitert, PC-Test klappt | IP statt `.local` eintragen, Slave-ID-Regler auf 1 |
| Bridge-Hotspot weg nach Routerwechsel | öffnet sich automatisch neu; BOOT-Taste 5 s = WLAN-Reset |
| Port 502 nimmt keine Verbindung an | Bridge erlaubt nur 2 Verbindungen (Firmware 1.0.1, ab 1.1.0: 4): kein zweites Programm parallel zu HA; Bridge kurz stromlos machen (1.1.0 startet sich dann selbst neu) |
| Nach Aus-/Einschalten des Reglers keine Werte | normal, ca. 1 Minute warten (Modulsuche, nur `0xE0`) |


## 7. Nach der Einrichtung: Bedienen

- Die **Geräteseite** (Einstellungen -> Geräte & Dienste -> UNI PRO Modbus ->
  Gerät) zeigt automatisch alle Temperaturen, Soll-/Kurvenwerte, Pumpen- und
  Fühlerzustände sowie die Zeitprogramme – ohne weitere Einrichtung.
- **Wochenzeitprogramme** (CO1-CO7, Warmwasser, Zirkulation) lassen sich je
  Kreis ablesen (Sensor "... Zeitprogramm", Stunde für Stunde in den
  Attributen) und per Dienst **`uni_pro_modbus.set_schedule`** setzen
  (Entwicklerwerkzeuge -> Aktionen, Formular, kein YAML nötig). Beispiel:
  `schedule: co1_schedule`, `days: {monday: [6,7,8,18,19,20], sunday: all}`.
- **Warmwasser-Desinfektion**: Ein/Aus plus je ein Schalter pro Wochentag
  (nur UNI2 PRO).
- **Fertiges Dashboard** zum Kopieren inkl. einer Klick-Ansicht für die
  Zeitprogramme: siehe [`dashboard_example.yaml`](dashboard_example.yaml).
- Eine vollständige Liste aller ausgelesenen/beschreibbaren Register steht in
  [`register-abgleich.md`](register-abgleich.md).
