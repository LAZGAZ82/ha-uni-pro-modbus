# Bridge firmware notes (ESP32-S3-RS485-CAN)

This integration expects a **generic Modbus TCP-to-RTU gateway** on the
bridge: it accepts standard Modbus TCP (MBAP-framed) requests, forwards
them as Modbus RTU frames on the RS485 bus, and relays the RTU response
back as a standard Modbus TCP response. It does **not** need to know
anything about UNI PRO registers — that logic lives entirely in the HA
integration, so the same firmware image works for every customer and
every controller model.

## What we confirmed about the Waveshare ESP32-S3-RS485-CAN board

- ESP32-S3 based (dual-core, WiFi).
- Onboard **isolated** RS485 transceiver with automatic direction control
  ("automatic control through main control hardware flow") — no manual
  DE/RE GPIO toggling needed in firmware, which simplifies the UART setup
  considerably.
- Waveshare's own wiki only documents **Arduino IDE** for this board; two
  official demos exist (`01_MAIN_WIFI_AP`, `02_MAIN_WIFI_STA`) covering
  RS485/CAN control over WiFi, but **no Modbus gateway example** and **no
  documented GPIO pin numbers** for the RS485 UART.

### Open item before flashing

The exact UART TX/RX GPIO pins used for the RS485 transceiver were **not**
listed on the wiki page we could access. Check the board's schematic or
the Arduino demo source (`01_MAIN_WIFI_AP`/`02_MAIN_WIFI_STA`, linked from
the wiki) for the actual pin numbers before writing/flashing firmware —
using the wrong pins won't damage anything, but RS485 traffic simply won't
show up.

## Recommended firmware approaches (pick one)

### Option A — Arduino sketch with the `eModbus` library (recommended)

[`eModbus`](https://github.com/eModbus/eModbus) is a maintained Modbus
library for ESP32/Arduino that includes a purpose-built
`ModbusBridge`/`ModbusClientRTU` + `ModbusServerTCPasync` pattern for
exactly this "Modbus TCP-to-RTU gateway" use case (see its
`TCPtoRTUbridge` example). Because Waveshare only officially supports
Arduino for this board, this is the lowest-risk path:

1. Start from Waveshare's own WiFi demo (already handles WiFi + the
   board's RS485 init) as your base.
2. Add `eModbus`'s TCP-to-RTU bridge example on top, pointed at the
   board's RS485 UART pins.
3. Bridge listens on TCP port 502 (configurable), forwards to RS485 at
   9600 8N1 (matching the UNI PRO manual's fixed serial parameters).

This keeps firmware generic and reusable for any RTU device behind it,
matching the "one firmware image, all logic in the HA integration"
architecture this repo assumes.

### Option B — ESPHome + a community Modbus-bridge external component

ESPHome's *built-in* `modbus_controller` and `modbus_server` components do
**not** do this job (they expose the ESP32's own local data as Modbus, or
poll a device to create ESPHome-side entities — not a transparent TCP↔RTU
gateway). There are third-party ESPHome `external_components` that do
implement a real bridge (e.g. `davidrapan/esphome-modbus_bridge`,
`rosenrot00/esphome_modbus_bridge`) — evaluate current maintenance status
before depending on one commercially, and note that ESPHome support for
this specific Waveshare board is not officially documented (Arduino is),
so pin mapping still needs to be confirmed independently.

### Not recommended: putting register logic on the bridge

ESPHome's `modbus_controller`, wired directly to the UNI PRO's registers,
would also work end-to-end (entities show up automatically via the
built-in ESPHome HA integration, no HACS needed at all). We deliberately
did **not** go this route because it ties every register/entity change to
a firmware re-flash per customer device, instead of a single centrally
updated HACS integration. Worth reconsidering only if the product
direction shifts away from "one integration, many bridges."

## Confirmed field issue: RS485 A/B must be crossed

Real-hardware debugging session (UNI3 PRO + Waveshare ESP32-S3-RS485-CAN,
SP3485EN transceiver) found that a straight A→A / B→B wire between bridge
and controller produces **zero response**, even though every other part of
the chain is correct. Full diagnostic trail, in case this needs
re-verifying on a different controller/bridge combo:

1. Software/eModbus ruled out with a standalone sketch that bypasses
   `ModbusBridgeWiFi` entirely and sends raw Modbus RTU frames directly on
   `Serial1` — confirmed the real bridge firmware's `onDataHandler`/
   `onErrorHandler` never fire when `ModbusClientRTU` is driven as a bridge
   backend (that's a diagnostic dead-end, not a bug worth fixing — just
   don't rely on those handlers for debugging bridge traffic).
2. All 4 combinations of TX/RX pin swap × EN polarity tested with the
   standalone sketch — none produced a response, ruling out pin mapping
   and EN/DE polarity assumptions.
3. Confirmed via the official Waveshare schematic PDF that GPIO17→DI,
   GPIO18←RO, GPIO21→DE/RE (SP3485EN, manual direction control, *not*
   hardware auto-direction despite some marketing copy suggesting
   otherwise) — so the firmware's pin constants were correct all along.
4. Multimeter test (static DC levels, no UART toggling) on A/B confirmed
   the bridge's driver output is healthy: EN=HIGH + TX=LOW → −3.3V,
   EN=HIGH + TX=HIGH → +3.3V — a clean, correct differential swing that
   tracks the data bit. This ruled out a defective transceiver.
5. Confirmed the controller's Slave ID (`1`) and Modbus RTU enable via its
   own on-screen settings — both correct.
6. Crossing A/B (bridge A+ → controller B, bridge B- → controller A)
   fixed it immediately; the straight/mirrored connection never worked no
   matter what else was tried.

**Takeaway for future bridge-doesn't-respond cases:** verify the physical
transmit signal is healthy (multimeter static test above) before
suspecting firmware or the transceiver chip — and try crossing A/B early,
it's a 30-second change that resolved a multi-hour debugging session here.

## Testing the bridge before wiring up the UNI PRO

Any standard Modbus TCP tool (e.g. `pymodbus.console`, `mbpoll`,
QModMaster) pointed at the bridge's IP:502 should be able to read register
0 (function 4, input register) once the UNI PRO is wired up and Modbus RTU
is enabled on the controller (`Settings → Common → Communication → MODBUS
RTU`, code `456`) — confirm this works *before* installing the HA
integration, to isolate wiring/gateway issues from integration issues.
