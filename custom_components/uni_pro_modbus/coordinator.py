"""DataUpdateCoordinator for the UNI PRO Modbus TCP bridge."""
from __future__ import annotations

import inspect
import logging
from datetime import timedelta
from typing import Any

from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.helpers.update_coordinator import DataUpdateCoordinator, UpdateFailed

from .const import SCHEDULE_REGISTER_COUNT, DataType, RegisterDef, RegType, ScheduleDef
from .schedule import decode_schedule, encode_schedule

_LOGGER = logging.getLogger(__name__)


class UniProModbusError(Exception):
    """Raised on unrecoverable Modbus communication errors."""


class UniProModbusCoordinator(DataUpdateCoordinator[dict[str, int]]):
    """Polls the configured registers over Modbus TCP and caches raw values.

    Values are stored keyed by (reg_type, address) -> raw register content.
    Platforms/entities decode the specific bit/scale they need from
    RegisterDef via `get_value()`.
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
        self._client: Any | None = None
        self._slave_kwarg: str | None = None

        # De-duplicate registers by (reg_type, address) since several
        # RegisterDef entries (bit fields) share the same physical register.
        seen: dict[tuple[RegType, int], RegisterDef] = {}
        for reg in registers:
            seen[(reg.reg_type, reg.address)] = reg
        self._physical_registers = list(seen.items())

        # Weekly schedule blocks (11 contiguous holding registers each),
        # de-duplicated by their start register.
        sched_seen: dict[int, ScheduleDef] = {}
        for sched in schedules or []:
            sched_seen[sched.start_register] = sched
        self._schedules = list(sched_seen.values())

    async def _async_client(self):
        if self._client is None:
            # Imported lazily so config_flow validation / HA startup doesn't
            # hard-depend on pymodbus being importable before first use.
            from pymodbus.client import AsyncModbusTcpClient

            self._client = AsyncModbusTcpClient(self._host, port=self._port)
        if not self._client.connected:
            await self._client.connect()
        if not self._client.connected:
            raise UniProModbusError(f"Cannot connect to bridge at {self._host}:{self._port}")
        return self._client

    def _slave_kw(self, client) -> str:
        """Return the keyword this pymodbus install expects for the unit/slave id.

        pymodbus renamed this argument from `slave` to `device_id` in newer
        releases (3.7+). Detect which one the actually-installed version
        wants once, from the real method signature, and cache it -- so this
        integration keeps working across whatever pymodbus version each
        customer's Home Assistant happens to have bundled, instead of
        breaking with a TypeError on one side or the other.
        """
        if self._slave_kwarg is None:
            try:
                params = inspect.signature(client.read_holding_registers).parameters
            except (TypeError, ValueError):
                params = {}
            self._slave_kwarg = "device_id" if "device_id" in params else "slave"
        return self._slave_kwarg

    async def _async_update_data(self) -> dict[str, int]:
        client = await self._async_client()
        data: dict[str, int] = {}

        slave_kw = self._slave_kw(client)

        for (reg_type, address), reg in self._physical_registers:
            try:
                if reg_type is RegType.INPUT:
                    result = await client.read_input_registers(
                        address=address, count=1, **{slave_kw: self._unit_id}
                    )
                else:
                    result = await client.read_holding_registers(
                        address=address, count=1, **{slave_kw: self._unit_id}
                    )
            except Exception as err:  # noqa: BLE001 - surfaced via UpdateFailed
                raise UpdateFailed(
                    f"Error reading register {reg.register} ({reg_type}) "
                    f"at {self._host}:{self._port}: {err}"
                ) from err

            if result is None or result.isError():
                raise UpdateFailed(
                    f"Modbus error reading register {reg.register} ({reg_type}) "
                    f"from unit {self._unit_id}: {result}"
                )

            data[self._cache_key(reg_type, address)] = result.registers[0]

        for sched in self._schedules:
            try:
                result = await client.read_holding_registers(
                    address=sched.address,
                    count=SCHEDULE_REGISTER_COUNT,
                    **{slave_kw: self._unit_id},
                )
            except Exception as err:  # noqa: BLE001 - surfaced via UpdateFailed
                raise UpdateFailed(
                    f"Error reading schedule {sched.key} (reg {sched.start_register}) "
                    f"at {self._host}:{self._port}: {err}"
                ) from err

            if result is None or result.isError():
                raise UpdateFailed(
                    f"Modbus error reading schedule {sched.key} (reg "
                    f"{sched.start_register}) from unit {self._unit_id}: {result}"
                )

            data[self._schedule_cache_key(sched)] = list(result.registers)

        return data

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
        client = await self._async_client()
        result = await client.write_registers(
            address=sched.address,
            values=regs,
            **{self._slave_kw(client): self._unit_id},
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

        client = await self._async_client()

        if reg.data_type is DataType.BIT:
            # Registers exposing individual writable bits (disinfection,
            # control mode) are documented as single-purpose bit registers
            # in this integration's scope - write the whole register.
            raw = 1 if value else 0
        elif reg.data_type is DataType.INT16_X10:
            raw = int(round(float(value) * 10))
            if raw < 0:
                raw += 0x10000
        elif reg.data_type is DataType.UINT16:
            raw = int(value)
        elif reg.data_type is DataType.ENUM16:
            reverse = {v: k for k, v in (reg.options or {}).items()}
            raw = reverse.get(value, value)
        else:
            raw = int(value)

        result = await client.write_register(
            address=reg.address, value=raw, **{self._slave_kw(client): self._unit_id}
        )
        if result is None or result.isError():
            raise UniProModbusError(f"Write to register {reg.register} failed: {result}")

        # Update local cache immediately for a snappy UI, real value confirmed
        # on next poll.
        if self.data is not None:
            self.data[self._cache_key(reg.reg_type, reg.address)] = raw
            self.async_set_updated_data(self.data)

    async def async_close(self) -> None:
        if self._client is not None:
            self._client.close()
            self._client = None
