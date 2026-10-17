"""Constants and register map for the UNI2 PRO / UNI3 PRO Modbus RTU integration.

Register numbers and behaviour are taken from the manufacturer document
"UNI2 PRO and UNI3 PRO - Modbus RTU communication" (manual version 14/05/2026).

Modicon addressing convention confirmed by the worked examples in that manual:
    documented register 30001 (first input register)  -> raw address 0x0000
    documented register 40001 (first holding register) -> raw address 0x0000
i.e. raw_address = register - 30001   (input registers, function 04)
     raw_address = register - 40001   (holding registers, function 03/06/16)

IMPORTANT / open items to verify against real hardware:
- The manual explicitly warns register addresses "may differ by 1 depending on
  the software used". The examples in the manual match the convention above,
  but re-check with function 04/03 reads against the actual device before
  relying on this in production.
- The per-circuit sub-register layout (set point, weather-curve points,
  timed activation, schedule) is documented in full only for CO1. The
  manual's address-offset table (page 2) documents the complete
  40x01-40x30 / 30x01-30x02 span repeating for every circuit CO1-CO7 with a
  fixed +100 offset (see CIRCUIT_INPUT_BASE / CIRCUIT_HOLDING_BASE /
  CIRCUIT_STEP), so the relative sub-offsets are confirmed to be identical
  per circuit. Still worth a spot-check on hardware with more than one active
  circuit before relying on CO2+ in the field.
- UNI2 PRO does not support CO3 circulation queries (see manual, page 2).
- UNI3 PRO does not support DHW or Circulation queries at all.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum

DOMAIN = "uni_pro_modbus"

DEFAULT_PORT = 502
DEFAULT_UNIT_ID = 1
DEFAULT_SCAN_INTERVAL = 30  # seconds
# Matches the bridge firmware's mDNS hostname (see uni_pro_bridge.ino /
# MDNS_HOSTNAME) so the field is pre-filled with a value that works for
# most customers out of the box, without needing to look up an IP.
DEFAULT_HOST = "uni-pro-bridge.local"
DEFAULT_TIMEOUT = 3  # seconds (bridge <-> integration, TCP). The controller's own
# Modbus RTU timeout must be set to >= 500 ms on the bridge/gateway side.

CONF_MODEL = "model"
CONF_UNIT_ID = "unit_id"
CONF_CIRCUITS = "circuits"
CONF_DHW = "dhw"
CONF_CIRCULATION = "circulation"
CONF_SCAN_INTERVAL = "scan_interval"

# Optionales Fern-Monitoring ("Fernwartung") - standardmaessig AUS. Werte werden
# nur gesendet, wenn der Nutzer dies in den Optionen aktiviert UND einen eigenen
# Endpoint + Key eintraegt. Kein verstecktes "phone home".
CONF_REMOTE_ENABLED = "remote_enabled"
CONF_REMOTE_URL = "remote_url"
CONF_REMOTE_KEY = "remote_key"
CONF_REMOTE_INTERVAL = "remote_interval"
DEFAULT_REMOTE_INTERVAL = 300  # seconds

MANUFACTURER = "Kaiser Electronics"


class Model(str, Enum):
    """Controller model. Determines which optional circuits are legal."""

    UNI2_PRO = "uni2_pro"
    UNI3_PRO = "uni3_pro"


MODEL_LABELS = {
    Model.UNI2_PRO: "UNI2 PRO",
    Model.UNI3_PRO: "UNI3 PRO",
}

# UNI3 PRO does not support DHW / Circulation at all (manual, page 2).
# UNI2 PRO supports both, but not CO3 circulation queries.
MODEL_SUPPORTS_DHW_CIRCULATION = {
    Model.UNI2_PRO: True,
    Model.UNI3_PRO: False,
}

MAX_CIRCUITS = 7  # CO1..CO7

# UNI2 PRO does not have a CO3 circuit (manual, page 2); its heating circuits
# are CO1, CO2 and then the UNIM-PRO expansion-module circuits CO4..CO7.
# UNI3 PRO has CO1..CO3 on board plus CO4..CO7 from modules.
UNI2_SKIPPED_CIRCUITS = {3}


def available_circuits(model: "Model") -> list[int]:
    """Circuit indexes that are selectable for the given model.

    Expansion-module circuits always begin at CO4. UNI2 PRO has no CO3, so it
    is never offered there (manual, page 2)."""
    skip = UNI2_SKIPPED_CIRCUITS if model is Model.UNI2_PRO else set()
    return [i for i in range(1, MAX_CIRCUITS + 1) if i not in skip]


class RegType(str, Enum):
    INPUT = "input"  # function 04, read-only, 3xxxx
    HOLDING = "holding"  # function 03 (+ 06/16 if writable), 4xxxx


class DataType(str, Enum):
    INT16_X10 = "int16_x10"  # signed, divide by 10 (temperatures)
    UINT16 = "uint16"
    BIT = "bit"  # single bit out of a 16-bit bitfield register
    ENUM16 = "enum16"
    UINT8_HIGH = "uint8_high"  # high byte of a 16-bit register (e.g. month in packed month/day)
    UINT8_LOW = "uint8_low"  # low byte of a 16-bit register (e.g. day in packed month/day)


@dataclass(frozen=True)
class RegisterDef:
    """Describes a single logical value mapped to one Modbus register (+ bit)."""

    key: str
    name: str
    register: int  # documented register number, e.g. 30002 or 40102
    reg_type: RegType
    data_type: DataType
    writable: bool = False
    bit: int | None = None  # for DataType.BIT
    options: dict[int, str] | None = None  # for DataType.ENUM16
    unit: str | None = None
    device_class: str | None = None
    icon: str | None = None
    entity_category: str | None = None  # "diagnostic" | "config" | None
    native_min: float | None = None  # optional number min/max/step overrides
    native_max: float | None = None
    native_step: float | None = None

    @property
    def base(self) -> int:
        return 30001 if self.reg_type is RegType.INPUT else 40001

    @property
    def address(self) -> int:
        """Raw 0-based Modbus address to put on the wire."""
        return self.register - self.base


# ---------------------------------------------------------------------------
# General / common registers (always present)
# ---------------------------------------------------------------------------

GENERAL_STATUS_BITS: list[RegisterDef] = [
    RegisterDef("alarm", "Alarm", 30001, RegType.INPUT, DataType.BIT, bit=0,
                device_class="problem"),
    RegisterDef("boiler_relay", "Boiler relay (K1)", 30001, RegType.INPUT, DataType.BIT, bit=1,
                device_class="running", entity_category="diagnostic"),
    RegisterDef("eco_input", "ECO input active", 30001, RegType.INPUT, DataType.BIT, bit=2,
                entity_category="diagnostic"),
    RegisterDef("season_entry", "Season entry active", 30001, RegType.INPUT, DataType.BIT, bit=3,
                entity_category="diagnostic"),
    RegisterDef("power_sensor_1_short", "Power sensor 1 short circuit", 30001, RegType.INPUT,
                DataType.BIT, bit=4, device_class="problem", entity_category="diagnostic"),
    RegisterDef("power_sensor_1_open", "Power sensor 1 open circuit", 30001, RegType.INPUT,
                DataType.BIT, bit=5, device_class="problem", entity_category="diagnostic"),
    RegisterDef("power_sensor_2_short", "Power sensor 2 short circuit", 30001, RegType.INPUT,
                DataType.BIT, bit=6, device_class="problem", entity_category="diagnostic"),
    RegisterDef("power_sensor_2_open", "Power sensor 2 open circuit", 30001, RegType.INPUT,
                DataType.BIT, bit=7, device_class="problem", entity_category="diagnostic"),
    RegisterDef("external_sensor_short", "External sensor short circuit", 30001, RegType.INPUT,
                DataType.BIT, bit=8, device_class="problem", entity_category="diagnostic"),
    RegisterDef("external_sensor_open", "External sensor open circuit", 30001, RegType.INPUT,
                DataType.BIT, bit=9, device_class="problem", entity_category="diagnostic"),
    # Bit 10: buffer pump status - UNI2 only, always 0 on UNI3. Exposed but the
    # config flow only shows it as relevant when model == UNI2_PRO.
    RegisterDef("buffer_pump", "Buffer pump", 30001, RegType.INPUT, DataType.BIT, bit=10,
                device_class="running", entity_category="diagnostic"),
]

GENERAL_SENSORS: list[RegisterDef] = [
    RegisterDef("supply_temp_1", "Supply temperature sensor 1", 30002, RegType.INPUT,
                DataType.INT16_X10, unit="°C", device_class="temperature"),
    RegisterDef("supply_temp_2", "Supply temperature sensor 2", 30003, RegType.INPUT,
                DataType.INT16_X10, unit="°C", device_class="temperature"),
    RegisterDef("outside_temp", "Outside temperature", 30004, RegType.INPUT,
                DataType.INT16_X10, unit="°C", device_class="temperature"),
]

HEATING_SEASON_MODE_OPTIONS = {
    0: "manual",
    1: "automatic_date",
    2: "automatic_temp",
    3: "external_input",
}

GENERAL_CONFIG: list[RegisterDef] = [
    RegisterDef("heating_season_mode", "Heating season mode", 40001, RegType.HOLDING,
                DataType.ENUM16, writable=True, options=HEATING_SEASON_MODE_OPTIONS,
                entity_category="config", icon="mdi:sun-snowflake-variant"),
    RegisterDef("heating_season_start_temp", "Heating season start temperature", 40004,
                RegType.HOLDING, DataType.INT16_X10, writable=True, unit="°C",
                device_class="temperature", entity_category="config"),
    RegisterDef("heating_season_end_temp", "Heating season end temperature", 40005,
                RegType.HOLDING, DataType.INT16_X10, writable=True, unit="°C",
                device_class="temperature", entity_category="config"),
    # Heating season start/end *date*: register 40002 (start) and 40003 (end)
    # each pack the date into one 16-bit register as two bytes. Byte order is
    # CONFIRMED by the manual (v0.3 / 14-05-2026): "Byte HI (older): month
    # (1-12), Byte LO (younger): day (1-31)" -> high byte = month, low byte =
    # day, exactly as mapped below (UINT8_HIGH = month, UINT8_LOW = day).
    # Writes go through a read-modify-write (see coordinator.async_write) so
    # setting one half preserves the other.
    RegisterDef("heating_season_start_month", "Heating season start month", 40002,
                RegType.HOLDING, DataType.UINT8_HIGH, writable=True, native_min=1,
                native_max=12, native_step=1, entity_category="config",
                icon="mdi:calendar-start"),
    RegisterDef("heating_season_start_day", "Heating season start day", 40002,
                RegType.HOLDING, DataType.UINT8_LOW, writable=True, native_min=1,
                native_max=31, native_step=1, entity_category="config",
                icon="mdi:calendar-start"),
    RegisterDef("heating_season_end_month", "Heating season end month", 40003,
                RegType.HOLDING, DataType.UINT8_HIGH, writable=True, native_min=1,
                native_max=12, native_step=1, entity_category="config",
                icon="mdi:calendar-end"),
    RegisterDef("heating_season_end_day", "Heating season end day", 40003,
                RegType.HOLDING, DataType.UINT8_LOW, writable=True, native_min=1,
                native_max=31, native_step=1, entity_category="config",
                icon="mdi:calendar-end"),
]

# ---------------------------------------------------------------------------
# Per-circuit (CO1..CO7) register template
# ---------------------------------------------------------------------------
# Documented in full for CO1 (input 30101/30102, holding 40101-40130).
# CO2..CO7 follow the same +100-per-circuit offset per the address table on
# page 2 of the manual. Sub-offsets below are relative to the circuit's own
# holding/input base and were verified against the CO1 example rows.

CIRCUIT_INPUT_BASE = 30101
CIRCUIT_HOLDING_BASE = 40101
CIRCUIT_STEP = 100


@dataclass(frozen=True)
class CircuitRegisters:
    index: int  # 1..7
    temperature: RegisterDef
    enabled: RegisterDef
    set_temp: RegisterDef
    curve_minus20: RegisterDef
    curve_minus5: RegisterDef
    curve_plus5: RegisterDef
    curve_plus15: RegisterDef
    temp_activation_minutes: RegisterDef
    temp_activation_temp: RegisterDef
    pump_bit: RegisterDef
    room_input_bit: RegisterDef
    sensor_error_bit: RegisterDef
    sensor_error_open_bit: RegisterDef


def build_circuit(index: int) -> CircuitRegisters:
    """Build the register set for heating circuit CO<index> (1-7)."""
    if not 1 <= index <= MAX_CIRCUITS:
        raise ValueError(f"circuit index must be 1-7, got {index}")

    off = CIRCUIT_STEP * (index - 1)
    in_status = CIRCUIT_INPUT_BASE + off
    in_temp = in_status + 1
    h_enabled = CIRCUIT_HOLDING_BASE + off
    h_set_temp = h_enabled + 1
    h_curve_m20 = h_enabled + 2
    h_curve_m5 = h_enabled + 3
    h_curve_p5 = h_enabled + 4
    h_curve_p15 = h_enabled + 5
    h_act_time = h_enabled + 9
    h_act_temp = h_enabled + 10

    prefix = f"co{index}"
    label = f"CO{index}"

    return CircuitRegisters(
        index=index,
        temperature=RegisterDef(f"{prefix}_temperature", f"{label} temperature", in_temp,
                                 RegType.INPUT, DataType.INT16_X10, unit="°C",
                                 device_class="temperature"),
        enabled=RegisterDef(f"{prefix}_enabled", f"{label} circuit enabled", h_enabled,
                             RegType.HOLDING, DataType.BIT, bit=0, writable=False),
        set_temp=RegisterDef(f"{prefix}_set_temp", f"{label} set temperature", h_set_temp,
                              RegType.HOLDING, DataType.INT16_X10, writable=True, unit="°C",
                              device_class="temperature"),
        curve_minus20=RegisterDef(f"{prefix}_curve_m20", f"{label} weather curve -20°C",
                                   h_curve_m20, RegType.HOLDING, DataType.INT16_X10,
                                   writable=True, unit="°C", device_class="temperature",
                                   entity_category="config"),
        curve_minus5=RegisterDef(f"{prefix}_curve_m5", f"{label} weather curve -5°C",
                                  h_curve_m5, RegType.HOLDING, DataType.INT16_X10,
                                  writable=True, unit="°C", device_class="temperature",
                                  entity_category="config"),
        curve_plus5=RegisterDef(f"{prefix}_curve_p5", f"{label} weather curve +5°C",
                                 h_curve_p5, RegType.HOLDING, DataType.INT16_X10,
                                 writable=True, unit="°C", device_class="temperature",
                                 entity_category="config"),
        curve_plus15=RegisterDef(f"{prefix}_curve_p15", f"{label} weather curve +15°C",
                                  h_curve_p15, RegType.HOLDING, DataType.INT16_X10,
                                  writable=True, unit="°C", device_class="temperature",
                                  entity_category="config"),
        temp_activation_minutes=RegisterDef(f"{prefix}_temp_activation_minutes",
                                             f"{label} temporary activation duration",
                                             h_act_time, RegType.HOLDING, DataType.UINT16,
                                             writable=True, unit="min",
                                             entity_category="config"),
        temp_activation_temp=RegisterDef(f"{prefix}_temp_activation_temp",
                                          f"{label} temporary activation temperature",
                                          h_act_temp, RegType.HOLDING, DataType.INT16_X10,
                                          writable=True, unit="°C", device_class="temperature",
                                          entity_category="config"),
        pump_bit=RegisterDef(f"{prefix}_pump", f"{label} pump", in_status, RegType.INPUT,
                              DataType.BIT, bit=0, device_class="running"),
        room_input_bit=RegisterDef(f"{prefix}_room_input", f"{label} room input closed",
                                    in_status, RegType.INPUT, DataType.BIT, bit=1),
        sensor_error_bit=RegisterDef(f"{prefix}_sensor_error", f"{label} sensor error (short circuit)",
                                      in_status, RegType.INPUT, DataType.BIT, bit=2,
                                      device_class="problem", entity_category="diagnostic"),
        sensor_error_open_bit=RegisterDef(f"{prefix}_sensor_error_open",
                                           f"{label} sensor error (open circuit)",
                                           in_status, RegType.INPUT, DataType.BIT, bit=3,
                                           device_class="problem", entity_category="diagnostic"),
    )


# ---------------------------------------------------------------------------
# DHW (UNI2 PRO only)
# ---------------------------------------------------------------------------

DHW_STATUS = RegisterDef("dhw_pump", "DHW pump", 30801, RegType.INPUT, DataType.BIT, bit=0,
                          device_class="running")
DHW_SENSOR_ERROR = RegisterDef("dhw_sensor_error", "DHW sensor error (short circuit)", 30801,
                                RegType.INPUT, DataType.BIT, bit=2, device_class="problem",
                                entity_category="diagnostic")
DHW_SENSOR_ERROR_OPEN = RegisterDef("dhw_sensor_error_open", "DHW sensor error (open circuit)",
                                     30801, RegType.INPUT, DataType.BIT, bit=3,
                                     device_class="problem", entity_category="diagnostic")
DHW_TEMPERATURE = RegisterDef("dhw_temperature", "DHW temperature", 30802, RegType.INPUT,
                               DataType.INT16_X10, unit="°C", device_class="temperature")
DHW_ENABLED = RegisterDef("dhw_enabled", "DHW circuit enabled", 40801, RegType.HOLDING,
                           DataType.BIT, bit=0, writable=False)
DHW_DISINFECTION = RegisterDef("dhw_disinfection", "DHW disinfection", 40802, RegType.HOLDING,
                                DataType.BIT, bit=0, writable=True, icon="mdi:bacteria-outline",
                                entity_category="config")
DHW_SET_TEMP = RegisterDef("dhw_set_temp", "DHW set temperature", 40803, RegType.HOLDING,
                            DataType.INT16_X10, writable=True, unit="°C",
                            device_class="temperature")
DHW_TEMP_ACTIVATION_MINUTES = RegisterDef("dhw_temp_activation_minutes",
                                           "DHW temporary activation duration", 40804,
                                           RegType.HOLDING, DataType.UINT16, writable=True,
                                           unit="min", entity_category="config")
DHW_TEMP_ACTIVATION_TEMP = RegisterDef("dhw_temp_activation_temp",
                                        "DHW temporary activation temperature", 40805,
                                        RegType.HOLDING, DataType.INT16_X10, writable=True,
                                        unit="°C", device_class="temperature",
                                        entity_category="config")

# ---------------------------------------------------------------------------
# Circulation (UNI2 PRO only; UNI2 also does not support CO3-circulation
# queries specifically, see manual page 2 - not applicable to this block,
# which is the single dedicated "Circulation" loop, not CO3).
# ---------------------------------------------------------------------------

CIRC_STATUS = RegisterDef("circulation_pump", "Circulation pump", 30901, RegType.INPUT,
                           DataType.BIT, bit=0, device_class="running")
CIRC_SENSOR_ERROR = RegisterDef("circulation_sensor_error",
                                 "Circulation sensor error (short circuit)", 30901,
                                 RegType.INPUT, DataType.BIT, bit=2, device_class="problem",
                                 entity_category="diagnostic")
CIRC_SENSOR_ERROR_OPEN = RegisterDef("circulation_sensor_error_open",
                                      "Circulation sensor error (open circuit)", 30901,
                                      RegType.INPUT, DataType.BIT, bit=3, device_class="problem",
                                      entity_category="diagnostic")
CIRC_TEMPERATURE = RegisterDef("circulation_temperature", "Circulation temperature", 30902,
                                RegType.INPUT, DataType.INT16_X10, unit="°C",
                                device_class="temperature")
CIRC_ENABLED = RegisterDef("circulation_enabled", "Circulation circuit enabled", 40901,
                            RegType.HOLDING, DataType.BIT, bit=0, writable=False)
CIRC_CONTROL_MODE_OPTIONS = {0: "time_control", 1: "temperature_control"}
CIRC_CONTROL_MODE = RegisterDef("circulation_control_mode", "Circulation control mode", 40902,
                                 RegType.HOLDING, DataType.ENUM16, writable=True,
                                 options=CIRC_CONTROL_MODE_OPTIONS, entity_category="config")
CIRC_SETPOINT_TEMP = RegisterDef("circulation_setpoint_temp", "Circulation set temperature",
                                  40903, RegType.HOLDING, DataType.INT16_X10, writable=True,
                                  unit="°C", device_class="temperature")
CIRC_TEMP_ACTIVATION_MINUTES = RegisterDef("circulation_temp_activation_minutes",
                                            "Circulation temporary activation duration", 40910,
                                            RegType.HOLDING, DataType.UINT16, writable=True,
                                            unit="min", entity_category="config")

# ---------------------------------------------------------------------------
# Weekly schedules (CO1..CO7, DHW, Circulation)
# ---------------------------------------------------------------------------
# Each schedule is a contiguous block of 11 holding registers (manual: CO1 at
# 40120-40130, DHW at 40820-40830, Circulation at 40920-40930; per-circuit
# schedules follow the same +100 offset table as the rest of the circuit, so
# CO<n> starts at 40120 + 100*(n-1)).
#
# Encoding (confirmed by the two worked examples in the manual, v0.3):
#   - 11 registers * 2 bytes = 22 bytes. Byte order within each register is
#     HI (older) then LO (younger).
#   - The first 21 bytes are meaningful: 7 days * 3 eight-hour blocks, in the
#     order Sunday, Monday, ... Saturday; within a day the blocks are
#     00:00-07:00, 08:00-15:00, 16:00-23:00.
#   - Each block byte holds 8 bits = 8 hours; bit N (LSB first) = hour
#     (block*8 + N), value 1 = active/on, 0 = inactive/off.
#   - The 22nd byte (LO of the last register) is unused and always 0x00.
# This is pure per-hour on/off; the manual defines NO per-slot comfort/setback
# temperature for schedules. Writes use function 16 (write multiple holding
# registers) only - function 06 is not offered for the schedule block.
#
# Manual example 1: reading the CO1 schedule "where all hours are active"
#   -> 21x 0xFF + 0x00 (registers 0xFFFF .. 0xFF00).
# Manual example 2: CO2 schedule "inactive every day at 6 o'clock"
#   -> per day 0xBF 0xFF 0xFF (bit 6 of the 00:00-07:00 block cleared).

SCHEDULE_REGISTER_COUNT = 11
# A schedule's first register, relative to the loop's own holding base
# (CIRCUIT_HOLDING_BASE / DHW 40801 / Circulation 40901): 40120-40101 = 19.
SCHEDULE_OFFSET = 19
# Day order as laid out in the register block (manual pages 4-7).
SCHEDULE_DAYS = [
    "sunday",
    "monday",
    "tuesday",
    "wednesday",
    "thursday",
    "friday",
    "saturday",
]


@dataclass(frozen=True)
class ScheduleDef:
    """A weekly on/off schedule occupying SCHEDULE_REGISTER_COUNT holding
    registers starting at `start_register` (documented register number)."""

    key: str
    name: str
    start_register: int  # e.g. 40120 (CO1), 40820 (DHW), 40920 (Circulation)

    @property
    def address(self) -> int:
        """Raw 0-based Modbus address of the first schedule register."""
        return self.start_register - 40001


def build_circuit_schedule(index: int) -> ScheduleDef:
    """Schedule for heating circuit CO<index> (1-7)."""
    if not 1 <= index <= MAX_CIRCUITS:
        raise ValueError(f"circuit index must be 1-7, got {index}")
    start = CIRCUIT_HOLDING_BASE + CIRCUIT_STEP * (index - 1) + SCHEDULE_OFFSET
    return ScheduleDef(f"co{index}_schedule", f"CO{index} schedule", start)


DHW_SCHEDULE = ScheduleDef("dhw_schedule", "DHW schedule", 40820)
CIRC_SCHEDULE = ScheduleDef("circulation_schedule", "Circulation schedule", 40920)

# DHW disinfection weekly schedule: register 40806, "7 bits of the lower byte
# (LO)" = one bit per weekday (manual, r/w via function 03/06). The manual does
# not state which bit is which day, so the mapping below ASSUMES the same
# Sunday-first order used by the hourly schedule block (SCHEDULE_DAYS): bit 0 =
# Sunday ... bit 6 = Saturday. Verify on hardware (enable one weekday in the
# controller UI, read 40806 back) and reorder DISINFECTION_DAY_BITS if needed.
# Each bit is exposed as its own writable switch, matching the DHW disinfection
# on/off switch above. Bit writes are read-modify-write (see coordinator) so
# toggling one day preserves the others.
DISINFECTION_DAY_BITS = list(enumerate(SCHEDULE_DAYS))  # [(0,"sunday"),...,(6,"saturday")]
_DAY_TITLES = {
    "sunday": "Sunday", "monday": "Monday", "tuesday": "Tuesday",
    "wednesday": "Wednesday", "thursday": "Thursday", "friday": "Friday",
    "saturday": "Saturday",
}
DHW_DISINFECTION_DAYS = [
    RegisterDef(
        f"dhw_disinfection_{day}", f"DHW disinfection {_DAY_TITLES[day]}", 40806,
        RegType.HOLDING, DataType.BIT, bit=bit, writable=True,
        icon="mdi:bacteria-outline", entity_category="config",
    )
    for bit, day in DISINFECTION_DAY_BITS
]
