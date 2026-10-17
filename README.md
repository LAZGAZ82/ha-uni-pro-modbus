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
- Select which heating circuits (CO1-CO7) are active per installation.
  Circuits CO4-CO7 come from UNIM-PRO expansion modules; the active
  circuits are **auto-detected** at setup (the circuit on/off registers
  40x01 are read and the enabled circuits are preselected — editable by
  hand, falls back to CO1 only if the read fails). On UNI2 PRO, CO3 is not
  offered (the controller has no CO3; module circuits start at CO4).
  Note: Modbus exposes no module *connection* status, so a disconnected
  module is not reported as such — it simply stops answering for its
  circuits.
- Entities:
  - **sensor**: supply/outside/circuit/DHW/circulation temperatures, plus a
    read-only **weekly schedule** sensor per loop (active hours + the full
    per-day on/off in attributes)
  - **binary_sensor**: alarm, pump running, sensor faults, room input,
    circuit enabled/disabled status (read-only per the device's own spec)
  - **number**: set temperatures, weather-curve points, timed-activation
    duration/temperature, heating season start/end date (month/day)
  - **select**: heating season mode, circulation control mode
  - **switch**: DHW disinfection on/off, plus one switch per weekday for
    the DHW disinfection schedule (UNI2 PRO)
- **Weekly schedules** (CO1-CO7, DHW, Circulation) are readable and
  writable — read via the schedule sensors, written with the
  `uni_pro_modbus.set_schedule` service (see "Weekly schedules" below)
- Options flow to change active circuits / DHW / circulation / poll
  interval after setup, without deleting and re-adding the integration

## Not yet implemented (roadmap)

- CO2-CO7 sub-register layout (set point, weather curve, timers, schedule)
  mirrors CO1's; this is **confirmed** by the address-offset table in the
  manual (page 2, fixed +100 per circuit — see
  `custom_components/uni_pro_modbus/const.py` docstring). A spot-check on
  real hardware with more than one active circuit is still recommended
  before relying on it in the field.
- Register addressing was validated against the worked examples in the
  manual (Modicon convention, `raw = register - base`), but the manual
  itself warns addressing "may differ by 1 depending on the software
  used" — re-confirm with a real device on first hardware bring-up.

## Weekly schedules

Each heating circuit (CO1-CO7), the DHW circuit and the Circulation circuit
has a weekly on/off schedule in the controller: a per-hour on/off flag for
every hour of every weekday (168 hours total). The controller's schedule is
**on/off only** — there is no per-slot comfort/setback temperature in the
Modbus schedule registers (confirmed against the manufacturer manual, v0.3).

**Reading.** Each active loop gets a diagnostic sensor, e.g.
`sensor.<prefix>_co1_schedule`. Its state is the number of active hours in
the week (0-168). The full schedule is in its attributes:

- `schedule`: human-readable per-day time ranges, e.g.
  `monday: ["06:00-09:00", "18:00-21:00"]`
- `weekly`: raw per-hour booleans (`{monday: [false, false, ...]}`) for use
  in templates and automations

**Writing.** Use the `uni_pro_modbus.set_schedule` service. It is a
read-modify-write: only the weekdays you pass are changed, the rest keep
their current schedule on the device. Each day's value is a list of the hours
(0-23) that should be ON, or the shortcut `all` / `none`.

```yaml
service: uni_pro_modbus.set_schedule
data:
  # entry_id: <optional, only needed if you have more than one instance>
  schedule: co1_schedule   # co1_schedule .. co7_schedule, dhw_schedule, circulation_schedule
  days:
    monday: [6, 7, 8, 18, 19, 20]   # on 06:00-09:00 and 18:00-21:00
    tuesday: [6, 7, 8, 18, 19, 20]
    sunday: all                      # on all day
    saturday: none                   # off all day
    # wednesday/thursday/friday omitted -> left unchanged
```

Schedules are written with Modbus function 16 (write multiple registers),
which is the only write function the controller offers for the schedule
block.

**Dashboard (no YAML, no extra install).** `docs/dashboard_example.yaml`
ships a second view, "Zeitprogramme", that shows each loop's current schedule
and lets the customer change it by clicking: preset buttons (24/7, off,
weekdays 06-22, ...) and per-weekday whole-day on/off buttons, all wired to
the `set_schedule` service. Free-form per-hour editing is done from
Developer Tools -> Actions (the service form, still no YAML).

> **Note on Home Assistant `schedule` helpers.** The project originally aimed
> to mirror schedules into native HA `schedule` helpers that the customer
> paints in the built-in schedule card. That turns out not to be workable
> with built-in cards alone: Home Assistant neither lets an integration
> create/edit a `schedule` helper at runtime, nor exposes a helper's full
> weekly definition to templates/automations, so a helper cannot drive the
> device automatically. Hence the native approach above (read-back sensors +
> `set_schedule` service + preset/whole-day buttons). Truly free-form
> click-painting of every hour would require an optional custom Lovelace card
> (e.g. a scheduler card from HACS) — deliberately not required here.

## Einrichtung / Installation

Deutsche Schritt-für-Schritt-Anleitung aus den Feldtests (Verdrahtung
inkl. GND und 120R-Terminierung, Modbus am Regler aktivieren, Bridge-WLAN,
HACS, Einrichtung in HA, Fehlerbehebung):
**[docs/einrichtung-home-assistant.md](docs/einrichtung-home-assistant.md)**

Komplettanleitung für Endkunden (Bridge, Verdrahtung, Regler, Home
Assistant, Fehlerbehebung) als PDF:
**[docs/Anleitung_UNI3-PRO_Komplett_v3.1.pdf](docs/Anleitung_UNI3-PRO_Komplett_v3.1.pdf)**

The English quick reference follows below.

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
- Which heating circuits (CO1-CO7) are actually wired up (the setup step
  preselects the circuits the controller reports as active; CO4-CO7 are
  the UNIM-PRO expansion-module circuits, UNI2 PRO has no CO3)

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

## Dashboard

As soon as the integration is added, Home Assistant automatically shows a
device page (Settings → Devices & Services → UNI PRO Modbus) with the
Kaiser Electronics logo, manufacturer/model, and every sensor/control
grouped together — no setup required.

For a curated, single-page overview dashboard (grouped by heating circuit,
with the logo as a header), see
[`docs/dashboard_example.yaml`](docs/dashboard_example.yaml). Home
Assistant does not allow integrations to install dashboards automatically,
so this is a ready-to-paste template: replace the `UNIPRO_PREFIX`
placeholder with your installation's actual entity prefix (instructions are
in the file itself), then import it via Settings → Dashboards → Add
Dashboard → Raw configuration editor.

## Remote monitoring (optional, opt-in)

The integration can optionally send the current register values to an external
HTTPS endpoint of **your** choosing — for a service provider / installer who
monitors the heating system remotely and alerts you if something looks wrong.

**This is off by default and never sends anything on its own.** It only
activates when you, in *Settings → Devices & Services → UNI PRO Modbus →
Configure*, tick **"Enable remote monitoring"** and enter:

- **Remote endpoint URL (HTTPS)** — the address your provider gave you, e.g.
  `https://uni-api.example.com/ingest.php`. Must be `https://` (data is sent
  encrypted).
- **Remote access key** — the per-site key your provider gave you (sent as the
  `X-Api-Key` header).
- **Push interval** — how often the values are sent (default 300 s, min 60 s).

What is sent: the current decoded values of the registers this integration is
already polling (temperatures, states, setpoints), as JSON
`{"device_ts": "...", "readings": {"<entity_key>": <value>, ...}}`. Nothing
else — no credentials, no Home Assistant data beyond these register values. A
failed push is logged at debug level and never disrupts normal operation.
Turning the option off stops it immediately. Remote *writing* back to the
controller is **not** part of this and is not enabled.

## Development / packaging notes for maintainers

- `custom_components/uni_pro_modbus/const.py` holds the full register map
  extracted from the manufacturer's "UNI2 PRO and UNI3 PRO — Modbus RTU
  communication" manual (v. 14/05/2026, = document revision v0.3), with
  docstring notes on every assumption made where the manual was ambiguous
  or incomplete. The manual itself is committed at
  [`docs/UNI_PRO_Modbus_RTU_v0.3_eng.pdf`](docs/UNI_PRO_Modbus_RTU_v0.3_eng.pdf)
  for reference. A complete register-by-register mapping of the manual
  against the code (every register: address, read/write per the manual, and
  its status in the integration) is in
  [`docs/register-abgleich.md`](docs/register-abgleich.md).
- Update `manifest.json` (`codeowners`, `documentation`, `issue_tracker`,
  `version`) with your actual GitHub org/repo before publishing.
- Tag a GitHub release matching `manifest.json`'s `version` before adding
  this as a HACS custom repository — HACS resolves versions from releases.
- Run `python3 -m json.tool` on `hacs.json`/`manifest.json` and Home
  Assistant's `hassfest` / HACS validation actions (see
  `.github/workflows/`) before publishing.
