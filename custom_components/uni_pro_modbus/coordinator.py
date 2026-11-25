"""DataUpdateCoordinator for the UNI PRO Modbus TCP bridge."""
from __future__ import annotations

import logging
from datetime import timedelta
from typing import Any

from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.helpers.update_coordinator import DataUpdateCoordinator, UpdateFailed

from .const import SCHEDULE_REGISTER_COUNT, DataType, RegisterDef, RegType, ScheduleDef
from .polling import DEFAULT_REQUEST_PAUSE_MS, ModbusSession, make_tcp_client, plan_blocks
from .schedule import decode_schedule, encode_schedule

_LOGGER = logging.getLogger(__name__)


class UniProModbusError(Exception):
    """Raised on unrecoverable Modbus communication errors."""


class UniProModbusCoordinator(DataUpdateCoordinator[dict[str, int]]):
    """Polls the configured registers over ONE persistent Modbus TCP connection.

    Values are stored keyed by "<reg_type>:<address>" -> raw register content
    (schedules: "sched:<start register>" -> list of 11 registers). Since v0.3.1
    the poll is fault tolerant (see polling.py): contiguous registers are read
    block-wise, a block the controller does not answer only makes its own
    entities unavailable, and UpdateFailed is raised only if nothing at all
    could be read.
    """

    def __init__(
        self,
        hass: HomeAssistant,
        entry: ConfigEntry,
        host: str,
        port: int,
        unit_id: int,
        scan_interval: int,
        registers: list[RegisterDef],
        schedules: list[ScheduleDef] | None = None,
        request_pause_ms: int = DEFAULT_REQUEST_PAUSE_MS,
    ) -> None:
        super().__init__(
            hass,
            _LOGGER,
            name=f"uni_pro_modbus ({host}:{port})",
            update_interval=timedelta(seconds=scan_interval),
        )
        self._host = host
        self._port = port
        self._unit_id = unit_id
        self._entry = entry
        self._session = ModbusSession(
            lambda: make_tcp_client(host, port), unit_id, request_pause_ms
        )

        # De-duplicate schedules by their start register (11 contiguous
        # holding registers each).
        sched_seen: dict[int, ScheduleDef] = {}
        for sched in schedules or []:
            sched_seen[sched.start_register] = sched
        self._schedules = list(sched_seen.values())

        # Several RegisterDef entries (bit fields) share one physical register;
        # plan_blocks de-duplicates and groups contiguous addresses.
        self._blocks = plan_blocks(
            [(reg.reg_type.value, reg.address) for reg in registers],
            [
                (self._schedule_cache_key(s), s.address, SCHEDULE_REGISTER_COUNT)
                for s in self._schedules
            ],
        )
        self._last_failed: frozenset[tuple[str, int, int]] = frozenset()

    async def _async_update_data(self) -> dict[str, int]:
        res = await self._session.read_cycle(self._blocks)

        failed = frozenset((b.kind, b.address, b.count) for b in res.failed)
        if failed != self._last_failed and res.data:
            if failed:
                _LOGGER.warning(
                    "UNI PRO Modbus %s:%s: %d of %d blocks not answered by the "
                    "controller (entities unavailable): %s",
                    self._host,
                    self._port,
                    len(failed),
                    len(self._blocks),
                    ", ".join(f"{k}@{a + (30001 if k == 'input' else 40001)}x{c}" for k, a, c in sorted(failed)),
                )
            else:
                _LOGGER.info("UNI PRO Modbus %s:%s: all blocks readable again", self._host, self._port)
        self._last_failed = failed

        if not res.data:
            reason = res.transport_error or "controller did not answer any register (after switching the controller off and on this takes about 1 minute)"
            raise UpdateFailed(f"No data from {self._host}:{self._port}: {reason}")
        if res.transport_error:
            _LOGGER.debug("Transport error after partial read, reconnecting next cycle: %s", res.transport_error)
        return res.data

    @staticmethod
    def _cache_key(reg_type: RegType, address: int) -> str:
        return f"{reg_type.value}:{address}"

    def get_raw(self, reg: RegisterDef) -> int | None:
        """Return the raw 16-bit register value for a RegisterDef, if cached."""
        if self.data is None:
            return None
        return self.data.get(self._cache_key(reg.reg_type, reg.address))

    def get_value(self, reg: RegisterDef):
        """Decode a RegisterDef's value out of the cached raw register."""
        raw = self.get_raw(reg)
        if raw is None:
            return None

        if reg.data_type is DataType.BIT:
            return bool((raw >> (reg.bit or 0)) & 1)
        if reg.data_type is DataType.INT16_X10:
            signed = raw - 0x10000 if raw >= 0x8000 else raw
            return round(signed / 10, 1)
        if reg.data_type is DataType.UINT16:
            return raw
        if reg.data_type is DataType.UINT8_HIGH:
            return (raw >> 8) & 0xFF
        if reg.data_type is DataType.UINT8_LOW:
            return raw & 0xFF
        if reg.data_type is DataType.ENUM16:
            return (reg.options or {}).get(raw, str(raw))
        return raw

    @staticmethod
    def _schedule_cache_key(sched: ScheduleDef) -> str:
        return f"sched:{sched.start_register}"

    def get_schedule(self, sched: ScheduleDef) -> dict[str, list[bool]]:
        """Return the decoded weekly on/off schedule, or {} if not yet read."""
        if self.data is None:
            return {}
        raw = self.data.get(self._schedule_cache_key(sched))
        return decode_schedule(raw) if raw is not None else {}

    async def async_write_schedule(
        self, sched: ScheduleDef, week: dict[str, list[bool]]
    ) -> None:
        """Encode and write a full weekly schedule via function 16."""
        regs = encode_schedule(week)
        result = await self._session.write(
            "write_registers", address=sched.address, values=regs
        )
        if result is None or result.isError():
            raise UniProModbusError(
                f"Write to schedule {sched.key} (reg {sched.start_register}) failed: {result}"
            )
        # Reflect immediately in the local cache; confirmed on next poll.
        if self.data is not None:
            self.data[self._schedule_cache_key(sched)] = regs
            self.async_set_updated_data(self.data)

    async def async_write(self, reg: RegisterDef, value) -> None:
        """Write a decoded value back to the device, encoding per data type."""
        if not reg.writable:
            raise UniProModbusError(f"{reg.key} ({reg.register}) is not writable per spec")

        if reg.data_type is DataType.BIT:
            # Read-modify-write the single bit so that registers packing
            # several writable bits (e.g. the DHW disinfection weekday mask at
            # 40806) keep their other bits intact. For single-purpose bit
            # registers (e.g. disinfection on/off at 40802) this is equivalent
            # to writing the whole register. Falls back to 0 if the register
            # has not been polled yet.
            current = self.get_raw(reg) or 0
            bit = reg.bit or 0
            if value:
                raw = current | (1 << bit)
            else:
                raw = current & ~(1 << bit)
            raw &= 0xFFFF
        elif reg.data_type is DataType.INT16_X10:
            raw = int(round(float(value) * 10))
            if raw < 0:
                raw += 0x10000
        elif reg.data_type is DataType.UINT16:
            raw = int(value)
        elif reg.data_type in (DataType.UINT8_HIGH, DataType.UINT8_LOW):
            # Packed month/day: preserve the other byte of the register by
            # reading the last cached value and overwriting only our half.
            current = self.get_raw(reg) or 0
            byte_val = int(value) & 0xFF
            if reg.data_type is DataType.UINT8_HIGH:
                raw = (byte_val << 8) | (current & 0x00FF)
            else:
                raw = (current & 0xFF00) | byte_val
        elif reg.data_type is DataType.ENUM16:
            reverse = {v: k for k, v in (reg.options or {}).items()}
            raw = reverse.get(value, value)
        else:
            raw = int(value)

        result = await self._session.write(
            "write_register", address=reg.address, value=raw
        )
        if result is None or result.isError():
            raise UniProModbusError(f"Write to register {reg.register} failed: {result}")

        # Update local cache immediately for a snappy UI, real value confirmed
        # on next poll.
        if self.data is not None:
            self.data[self._cache_key(reg.reg_type, reg.address)] = raw
            self.async_set_updated_data(self.data)

    async def async_close(self) -> None:
        await self._session.close()
