<p align="center">
  <img src="brand_assets/logo_portrait.png" alt="Kaiser Electronics" width="180">
</p>

# UNI PRO Modbus (Home Assistant custom integration)

Built and maintained by [Kaiser Electronics](https://github.com/LAZGAZ82).

Home Assistant integration for **UNI2 PRO** and **UNI3 PRO** heating
controllers, talked to over **Modbus TCP** via a generic RTU↔TCP bridge
(e.g. an ESP32 running a transparent Modbus gateway) rather than a direct
serial connection to the Home Assistant host.

## Why a TCP bridge instead of a direct USB/RS485 connection?

This integration is built for a product/reseller scenario: the RS485↔TCP
bridge firmware is intentionally kept **generic** (it knows nothing about
UNI PRO registers, it just forwards Modbus TCP requests onto the RS485 bus
and returns the RTU response). All device-specific logic — register
addresses, scaling, which entities to expose, model differences — lives in
this integration instead, so it can be:

- installed/updated centrally via HACS, without re-flashing every
  customer's bridge hardware, and
- reused with *any* Modbus TCP-to-RTU gateway, not just one specific board.

```
UNI2/UNI3 PRO  --RS485 (Modbus RTU)-->  Bridge (ESP32, generic firmware)  --WiFi/Ethernet (Modbus TCP)-->  Home Assistant + this integration
```

See `docs/bridge-firmware.md` for bridge firmware notes and open items.

## Features

- Config flow (UI setup, no YAML)
- Supports both UNI2 PRO and UNI3 PRO; DHW and Circulation are only
  offered for UNI2 PRO, since UNI3 PRO does not support those Modbus
  queries (per the manufacturer's Modbus manual)
- Select which heating circuits (CO1-CO7) are active per installation
- Entities:
  - **sensor**: supply/outside/circuit/DHW/circulation temperatures
  - **binary_sensor**: alarm, pump running, sensor faults, room input,
    circuit enabled/disabled status (read-only per the device's own spec)
  - **number**: set temperatures, weather-curve points, timed-activation
    duration/temperature
  - **select**: heating season mode, circulation control mode
  - **switch**: DHW disinfection
- Options flow to change active circuits / DHW / circulation / poll
  interval after setup, without deleting and re-adding the integration

## Not yet implemented (roadmap)

- Weekly schedule registers (CO1-CO7, DHW, Circulation) — these are
  bit-packed, 11 registers per schedule; worth a dedicated
  card/service rather than 11 generic number entities per circuit.
- Heating season start/end **date** (month/day packed into one register).
- CO2-CO7 sub-register layout (set point, weather curve, timers) is
  **assumed** to mirror CO1's, based on the address-offset table in the
  manual (see `custom_components/uni_pro_modbus/const.py` docstring).
  Verify against real hardware once a customer has more than one active
  circuit before relying on it in the field.
- Register addressing was validated against the worked examples in the
  manual (Modicon convention, `raw = register - base`), but the manual
  itself warns addressing "may differ by 1 depending on the software
  used" — re-confirm with a real device on first hardware bring-up.

## Installation

### Via HACS (recommended once this repo is on GitHub)

1. HACS → the 3-dot menu → **Custom repositories**
2. Add this repository's URL, category **Integration**
3. Install "UNI PRO Modbus", restart Home Assistant
4. Settings → Devices & Services → **Add integration** → "UNI PRO Modbus"

### Manual

Copy `custom_components/uni_pro_modbus` into your Home Assistant
`config/custom_components/` folder (a Docker volume mount, typically),
restart Home Assistant, then add the integration via the UI as above.

## Setup

You'll need:

- The bridge's IP address and TCP port (default 502)
- The controller's Modbus **Slave ID** (default `1`, set in the
  controller under `Settings → Common → Communication → MODBUS RTU`,
  access code `456`)
- The controller model (UNI2 PRO / UNI3 PRO)
- Which heating circuits (CO1-CO7) are actually wired up

The integration polls once every 30 seconds by default (adjustable in
Settings → Devices & Services → UNI PRO Modbus → Configure).

### RS485 wiring — A/B must be crossed

On real hardware (confirmed on a UNI3 PRO unit, RS485 bridge = Waveshare
ESP32-S3-RS485-CAN), a straight A→A / B→B connection between the bridge
and the controller did **not** work — the bridge sent valid, correctly
formed Modbus RTU requests (confirmed with a multimeter and a
bridge-independent raw-frame test), but the controller never answered.
Swapping to a **crossed** connection (bridge A+ → controller B, bridge B-
→ controller A) fixed it immediately and communication has been reliable
since. If a customer reports "bridge unreachable / no response from
controller" after otherwise-correct setup (common ground present, correct
Slave ID, Modbus RTU enabled on the controller), try crossing A/B **before**
suspecting hardware — this is now the first thing to check. Some UNI3 PRO
units also expose two A/B terminal pairs; if the first pair doesn't work,
try the other one as well as crossing polarity.

## Development / packaging notes for maintainers

- `custom_components/uni_pro_modbus/const.py` holds the full register map
  extracted from the manufacturer's "UNI2 PRO and UNI3 PRO — Modbus RTU
  communication" manual (v. 14/05/2026), with docstring notes on every
  assumption made where the manual was ambiguous or incomplete.
- Update `manifest.json` (`codeowners`, `documentation`, `issue_tracker`,
  `version`) with your actual GitHub org/repo before publishing.
- Tag a GitHub release matching `manifest.json`'s `version` before adding
  this as a HACS custom repository — HACS resolves versions from releases.
- Run `python3 -m json.tool` on `hacs.json`/`manifest.json` and Home
  Assistant's `hassfest` / HACS validation actions (see
  `.github/workflows/`) before publishing.
