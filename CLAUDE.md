# CLAUDE.md – Projektkontext UNI PRO Modbus

## Produkt
**UNI PRO Modbus** besteht aus zwei Teilen:

| Teil | Pfad | Version | Details |
|---|---|---|---|
| HACS-Integration (Home Assistant) | `custom_components/uni_pro_modbus/` | 0.1.5 | Manufacturer: Kaiser Electronics |
| ESP32-Bridge-Firmware | `firmware/` | 1.0.1 | Waveshare ESP32-S3-RS485-CAN, eModbus v1.7.2stable, USB-CDC-Flags gesetzt |

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

## Dokumentation
- `docs/einrichtung-home-assistant.md` – Schritt-für-Schritt-Einrichtung in HA (Feldtest-Erkenntnisse)
- `docs/Anleitung_UNI3-PRO_Komplett_v3.1.pdf` – Komplettanleitung v3.1
- `docs/bridge-firmware.md` – Firmware-Beschreibung
- `docs/dashboard_example.yaml` – Beispiel-Dashboard

## Arbeitsregeln
- **Nach jeder Änderung committen UND pushen** – GitHub (`LAZGAZ82/ha-uni-pro-modbus`) ist das Backup.
- Versionsnummern bei Änderungen mitziehen: Integration in `custom_components/uni_pro_modbus/manifest.json`, Firmware im Versionsbanner in `firmware/src/`.
- Feldbestätigte Erkenntnisse (Verdrahtung, IP statt mDNS) nicht "korrigieren" – sie widersprechen bewusst der Standarderwartung.
