# Register-Abgleich: Handbuch ↔ Integration

Vollständiger Abgleich jedes Registers aus dem Modbus-Handbuch
[`UNI_PRO_Modbus_RTU_v0.3_eng.pdf`](UNI_PRO_Modbus_RTU_v0.3_eng.pdf)
(Stand 14/05/2026) gegen den Stand der Home-Assistant-Integration
`uni_pro_modbus`.

Ziel des Projekts: **alle per Modbus les- und schreibbaren Werte sollen in
Home Assistant ankommen.** Nach diesem Abgleich ist jedes im Handbuch
dokumentierte Register umgesetzt — es fehlt nichts Les- oder Schreibbares.

## Legende

- **Register**: dokumentierte Registernummer aus dem Handbuch.
- **Roh**: tatsächliche Modbus-Adresse auf dem Draht (Modicon-Konvention,
  im Handbuch durch die Beispiele bestätigt):
  `Roh = Register − 30001` (Input, Funktion 04) bzw.
  `Roh = Register − 40001` (Holding, Funktion 03/06/16).
  Das Handbuch warnt, die Adressierung könne „je nach verwendeter Software
  um 1 abweichen" — am ersten echten Gerät gegenprüfen.
- **Zugriff (Handbuch)**: Funktionscodes und ob das Handbuch in der Spalte
  *Possible Data Overwrite* das Schreiben erlaubt (JA/NEIN).
- **Status**: ✅ umgesetzt · ➖ bewusst weggelassen/anders modelliert (mit
  Begründung). Es gibt keinen Eintrag „fehlt" mehr.
- **HA-Entity**: Plattform, unter der der Wert in Home Assistant erscheint.

Modelle: **UNI2 PRO** unterstützt alles. **UNI3 PRO** unterstützt **kein**
DHW (Warmwasser) und **keine** Zirkulation (Handbuch S. 2) — diese
Abschnitte werden bei UNI3 PRO gar nicht angelegt.

---

## 1. Allgemein / gemeinsam

| Register | Roh | Zugriff (Handbuch) | Datenformat | Status | HA-Entity |
|---|---|---|---|---|---|
| 30001 | 0 | 04, lesen | Bitfeld (16 Bit) | ✅ | 11× `binary_sensor` (siehe Bit-Tabelle unten) |
| 30002 | 1 | 04, lesen | int16 ×10 (°C) | ✅ | `sensor` Vorlauftemperatur Fühler 1 |
| 30003 | 2 | 04, lesen | int16 ×10 (°C) | ✅ | `sensor` Vorlauftemperatur Fühler 2 |
| 30004 | 3 | 04, lesen | int16 ×10 (°C) | ✅ | `sensor` Außentemperatur |
| 40001 | 0 | 03/06, **JA** | uint16 (0–3) | ✅ | `select` Heizsaison-Modus (manual / automatic-date / automatic-temp / external-input) |
| 40002 | 1 | 03/06, **JA** | 2× Byte: HI = Monat (1–12), LO = Tag (1–31) | ✅ | 2× `number` (Heizsaison **Start** Monat + Tag) |
| 40003 | 2 | 03/06, **JA** | 2× Byte: HI = Monat, LO = Tag | ✅ | 2× `number` (Heizsaison **Ende** Monat + Tag) |
| 40004 | 3 | 03/06, **JA** | int16 ×10 (°C) | ✅ | `number` Heizsaison-Starttemperatur |
| 40005 | 4 | 03/06, **JA** | int16 ×10 (°C) | ✅ | `number` Heizsaison-Endtemperatur |

### Bitbelegung Register 30001 (Allgemeiner Zustand)

| Bit | Bedeutung | Status | HA-Entity |
|---|---|---|---|
| 0 | OK (0) / Alarm (1) | ✅ | `binary_sensor` Alarm (problem) |
| 1 | Kessel-Relais K1 | ✅ | `binary_sensor` Kessel-Relais (running) |
| 2 | ECO-Eingang | ✅ | `binary_sensor` ECO-Eingang |
| 3 | Saison-Eingang | ✅ | `binary_sensor` Saison-Eingang |
| 4 | Leistungsfühler 1 Kurzschluss | ✅ | `binary_sensor` (problem) |
| 5 | Leistungsfühler 1 Unterbrechung | ✅ | `binary_sensor` (problem) |
| 6 | Leistungsfühler 2 Kurzschluss | ✅ | `binary_sensor` (problem) |
| 7 | Leistungsfühler 2 Unterbrechung | ✅ | `binary_sensor` (problem) |
| 8 | Externer Fühler Kurzschluss | ✅ | `binary_sensor` (problem) |
| 9 | Externer Fühler Unterbrechung | ✅ | `binary_sensor` (problem) |
| 10 | Pufferpumpe (nur UNI2, bei UNI3 immer 0) | ✅ | `binary_sensor` (running) |

---

## 2. Heizkreise CO1 – CO7

Dokumentiert ist CO1 vollständig. CO2–CO7 wiederholen dieselbe Belegung mit
festem Versatz **+100 je Kreis** (Handbuch-Offset-Tabelle S. 2). Die Tabelle
zeigt CO1; für CO*n* jeweils `+100·(n−1)` addieren (CO2 = 302xx/402xx, …,
CO7 = 307xx/407xx).

| Register (CO1) | Roh | Zugriff (Handbuch) | Datenformat | Status | HA-Entity |
|---|---|---|---|---|---|
| 30101 | 100 | 04, lesen | Bitfeld | ✅ | siehe Bit-Tabelle unten |
| 30102 | 101 | 04, lesen | int16 ×10 (°C) | ✅ | `sensor` CO1 Temperatur |
| 40101 | 100 | 03, **NEIN** | Bit 0 Ein/Aus | ➖ nur lesbar | `binary_sensor` „CO1 Kreis aktiv" — Handbuch erlaubt kein Schreiben (Overwrite NEIN, nur Funktion 03), daher bewusst read-only |
| 40102 | 101 | 03/06, **JA** | int16 ×10 (°C) | ✅ | `number` CO1 Solltemperatur |
| 40103 | 102 | 03/06, **JA** | int16 ×10 (°C) | ✅ | `number` CO1 Heizkurve −20 °C |
| 40104 | 103 | 03/06, **JA** | int16 ×10 (°C) | ✅ | `number` CO1 Heizkurve −5 °C |
| 40105 | 104 | 03/06, **JA** | int16 ×10 (°C) | ✅ | `number` CO1 Heizkurve +5 °C |
| 40106 | 105 | 03/06, **JA** | int16 ×10 (°C) | ✅ | `number` CO1 Heizkurve +15 °C |
| 40110 | 109 | 03/06, **JA** | uint16 (0 / 30–5940 min) | ✅ | `number` CO1 temporäre Aktivierung Dauer |
| 40111 | 110 | 03/06, **JA** | int16 ×10 (°C) | ✅ | `number` CO1 temporäre Aktivierung Temperatur |
| 40120–40130 | 119–129 | 03/16, **JA** | 11 Register, stündliches Wochenprogramm | ✅ | `sensor` CO1 Zeitprogramm (lesen) + Service `set_schedule` (schreiben) |

> Register 40107–40109 und 40112–40119 sind im Handbuch nicht dokumentiert
> (reserviert) und werden nicht abgefragt.

### Bitbelegung Register 301x1 (CO*-Zustand)

| Bit | Bedeutung | Status | HA-Entity |
|---|---|---|---|
| 0 | CO-Pumpe (0 aus / 1 ein) | ✅ | `binary_sensor` CO*n* Pumpe (running) |
| 1 | Raumeingang (1 = geschlossen) | ✅ | `binary_sensor` CO*n* Raumeingang geschlossen |
| 2 | Temperaturfühler Kurzschluss | ✅ | `binary_sensor` CO*n* Fühlerfehler (Kurzschluss) |
| 3 | Temperaturfühler Unterbrechung | ✅ | `binary_sensor` CO*n* Fühlerfehler (Unterbrechung) |

---

## 3. Warmwasser (DHW) — nur UNI2 PRO

| Register | Roh | Zugriff (Handbuch) | Datenformat | Status | HA-Entity |
|---|---|---|---|---|---|
| 30801 | 800 | 04, lesen | Bitfeld (Bit 0 Pumpe, Bit 2 Fühler Kurzschluss, Bit 3 Unterbrechung) | ✅ | 3× `binary_sensor` |
| 30802 | 801 | 04, lesen | int16 ×10 (°C) | ✅ | `sensor` DHW Temperatur |
| 40801 | 800 | 03, **NEIN** | Bit 0 Ein/Aus | ➖ nur lesbar | `binary_sensor` „DHW Kreis aktiv" — Overwrite NEIN, bewusst read-only |
| 40802 | 801 | 03/06, **JA** | Bit 0 Desinfektion aus/ein | ✅ | `switch` DHW Desinfektion |
| 40803 | 802 | 03/06, **JA** | int16 ×10 (°C) | ✅ | `number` DHW Solltemperatur |
| 40804 | 803 | 03/06, **JA** | uint16 (0 / 30–5940 min) | ✅ | `number` DHW temporäre Aktivierung Dauer |
| 40805 | 804 | 03/06, **JA** | int16 ×10 (°C) | ✅ | `number` DHW temporäre Aktivierung Temperatur |
| 40806 | 805 | 03/06, **JA** | 7 Bit im Low-Byte = Wochentage | ✅ | 7× `switch` „DHW Desinfektion <Wochentag>" |
| 40820–40830 | 819–829 | 03/16, **JA** | 11 Register, stündliches Wochenprogramm | ✅ | `sensor` DHW Zeitprogramm (lesen) + Service `set_schedule` (schreiben) |

> **Annahme zu 40806:** Das Handbuch nennt „7 Bit im Low-Byte", aber nicht,
> welches Bit welcher Tag ist. Die Integration nimmt dieselbe Sonntag-zuerst-
> Reihenfolge wie das Stundenprogramm an (Bit 0 = Sonntag … Bit 6 = Samstag).
> Am Gerät prüfen: einen Wochentag im Regler aktivieren, 40806 auslesen, bei
> Bedarf die Reihenfolge in `const.py` (`DISINFECTION_DAY_BITS`) anpassen.

---

## 4. Zirkulation — nur UNI2 PRO

| Register | Roh | Zugriff (Handbuch) | Datenformat | Status | HA-Entity |
|---|---|---|---|---|---|
| 30901 | 900 | 04, lesen | Bitfeld (Bit 0 Pumpe, Bit 2 Fühler Kurzschluss, Bit 3 Unterbrechung) | ✅ | 3× `binary_sensor` |
| 30902 | 901 | 04, lesen | int16 ×10 (°C) | ✅ | `sensor` Zirkulation Temperatur |
| 40901 | 900 | 03, **NEIN** | Bit 0 Ein/Aus | ➖ nur lesbar | `binary_sensor` „Zirkulation Kreis aktiv" — Overwrite NEIN, bewusst read-only |
| 40902 | 901 | 03/06, **JA** | Bit 0: 0 Zeitsteuerung / 1 Temperatursteuerung | ➖ als `select` | `select` Zirkulation Steuermodus — das 0/1-Register wird als benanntes Auswahlfeld statt als nackter Schalter modelliert (gleichwertig, bessere Bedienung) |
| 40903 | 902 | 03/06, **JA** | int16 ×10 (°C) | ✅ | `number` Zirkulation Solltemperatur |
| 40910 | 909 | 03/06, **JA** | uint16 (0 / 30–5940 min) | ✅ | `number` Zirkulation temporäre Aktivierung Dauer |
| 40920–40930 | 919–929 | 03/16, **JA** | 11 Register, stündliches Wochenprogramm | ✅ | `sensor` Zirkulation Zeitprogramm (lesen) + Service `set_schedule` (schreiben) |

> Zirkulation hat laut Handbuch **keine** „temporäre Aktivierung Temperatur"
> (anders als CO*/DHW) — korrekt nicht vorhanden.

---

## 5. Wochenzeitprogramme (Format)

Jedes Zeitprogramm (CO1–CO7, DHW, Zirkulation) belegt **11 aufeinanderfolgende
Holding-Register**. Lesen mit Funktion 03, Schreiben mit Funktion **16**
(Funktion 06 bietet der Regler dafür nicht an).

- 11 Register × 2 Byte = 22 Byte, Byte-Reihenfolge je Register **HI dann LO**.
- Die ersten 21 Byte sind belegt: 7 Tage × 3 Acht-Stunden-Blöcke, Reihenfolge
  **Sonntag … Samstag**; Blöcke je Tag 00:00–07:00, 08:00–15:00, 16:00–23:00.
- Jedes Byte = 8 Bit = 8 Stunden; Bit *N* (LSB zuerst) = Stunde `Block·8 + N`,
  1 = ein, 0 = aus.
- Das 22. Byte (LO des letzten Registers) ist ungenutzt und immer 0x00.
- **Reines Stunden-Ein/Aus — keine** Komfort-/Absenktemperatur pro Zeitfenster.

Der Codec ist gegen beide Rechenbeispiele des Handbuchs geprüft
(„alle Stunden aktiv" → 21× 0xFF + 0x00; „täglich um 6 Uhr aus" → je Tag
0xBF 0xFF 0xFF).

---

## 6. Zusammenfassung

| Gruppe | Register im Handbuch | umgesetzt | read-only (Overwrite NEIN) | anders modelliert |
|---|---|---|---|---|
| Allgemein | 30001 (+11 Bit), 30002–30004, 40001–40005 | alle | – | – |
| CO1–CO7 | je 30x01 (+4 Bit), 30x02, 40x01–40x06, 40x10, 40x11, Programm 40x20–40x30 | alle | 40x01 Ein/Aus | – |
| DHW | 30801 (+3 Bit), 30802, 40801–40806, Programm 40820–40830 | alle | 40801 Ein/Aus | – |
| Zirkulation | 30901 (+3 Bit), 30902, 40901–40903, 40910, Programm 40920–40930 | alle | 40901 Ein/Aus | 40902 als `select` |

**Es fehlt nichts Les- oder Schreibbares.** Bewusste Abweichungen:

1. **40101 / 40801 / 40901 (Kreis Ein/Aus)** — als `binary_sensor` nur lesbar,
   weil das Handbuch für diese Register *Possible Data Overwrite = NEIN* angibt
   und nur Funktion 03 (lesen) nennt. Schreiben würde der Regler laut
   Dokumentation nicht zulassen.
2. **40902 (Zirkulation Steuermodus)** — als `select` mit benannten Optionen
   statt als nackter Ein/Aus-Schalter umgesetzt. Funktional identisch
   (0 = Zeitsteuerung, 1 = Temperatursteuerung).

Alle Byte-/Bit-/Offset-Annahmen (Datums-Byteordnung, +100-Kreisversatz,
Desinfektions-Tagesbits, Modicon-Adressierung ±1) sind im Code als solche
dokumentiert und sollten bei der ersten echten Inbetriebnahme gegengeprüft
werden.
