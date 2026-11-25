"""Encoding/decoding of the UNI PRO weekly on/off schedule register block.

A schedule is SCHEDULE_REGISTER_COUNT (11) consecutive holding registers.
See const.py for the full wire-format description; the codec here is verified
against both worked examples in the manufacturer manual (v0.3):

  - "all hours active"            -> 21x 0xFF + 0x00
  - "inactive every day at 6:00"  -> per day 0xBF 0xFF 0xFF

Both the device<->HA read path (coordinator) and the read-only schedule
sensors use `decode_schedule`; the `set_schedule` service uses
`encode_schedule`.
"""
from __future__ import annotations

from .const import SCHEDULE_DAYS, SCHEDULE_REGISTER_COUNT

HOURS_PER_DAY = 24
_BLOCKS_PER_DAY = 3  # 00:00-07:00, 08:00-15:00, 16:00-23:00
_HOURS_PER_BLOCK = 8


def decode_schedule(registers: list[int]) -> dict[str, list[bool]]:
    """Decode 11 raw uint16 registers into {day: [24 booleans]} (hour 0..23).

    Returns an empty dict if the register count is unexpected (e.g. a partial
    or missing read) so callers can treat the schedule as "unknown".
    """
    if registers is None or len(registers) != SCHEDULE_REGISTER_COUNT:
        return {}

    byts: list[int] = []
    for reg in registers:
        byts.append((int(reg) >> 8) & 0xFF)  # HI (older) first
        byts.append(int(reg) & 0xFF)  # LO (younger)

    week: dict[str, list[bool]] = {}
    for day_index, day in enumerate(SCHEDULE_DAYS):
        hours = [False] * HOURS_PER_DAY
        for block in range(_BLOCKS_PER_DAY):
            byte = byts[day_index * _BLOCKS_PER_DAY + block]
            for bit in range(_HOURS_PER_BLOCK):
                hours[block * _HOURS_PER_BLOCK + bit] = bool((byte >> bit) & 1)
        week[day] = hours
    return week


def encode_schedule(week: dict[str, list[bool]]) -> list[int]:
    """Encode {day: [24 booleans]} back into 11 raw uint16 registers.

    Every day in SCHEDULE_DAYS must be present with exactly 24 booleans.
    """
    byts = [0] * 22
    for day_index, day in enumerate(SCHEDULE_DAYS):
        hours = week[day]
        if len(hours) != HOURS_PER_DAY:
            raise ValueError(f"day {day} must have {HOURS_PER_DAY} hours, got {len(hours)}")
        for block in range(_BLOCKS_PER_DAY):
            byte = 0
            for bit in range(_HOURS_PER_BLOCK):
                if hours[block * _HOURS_PER_BLOCK + bit]:
                    byte |= 1 << bit
            byts[day_index * _BLOCKS_PER_DAY + block] = byte
    byts[21] = 0  # unused tail byte, always 0x00

    return [(byts[i] << 8) | byts[i + 1] for i in range(0, 22, 2)]


def hours_to_ranges(hours: list[bool]) -> list[str]:
    """Render one day's 24 booleans as human-readable "HH:00-HH:00" ranges.

    e.g. [on 0..5, off 6, on 7..23] -> ["00:00-06:00", "07:00-24:00"].
    Returns [] for an all-off day.
    """
    ranges: list[str] = []
    start: int | None = None
    for hour in range(HOURS_PER_DAY):
        if hours[hour] and start is None:
            start = hour
        elif not hours[hour] and start is not None:
            ranges.append(f"{start:02d}:00-{hour:02d}:00")
            start = None
    if start is not None:
        ranges.append(f"{start:02d}:00-24:00")
    return ranges


def active_hour_count(week: dict[str, list[bool]]) -> int:
    """Total number of active (on) hours across the whole week (0..168)."""
    return sum(1 for hours in week.values() for on in hours if on)
