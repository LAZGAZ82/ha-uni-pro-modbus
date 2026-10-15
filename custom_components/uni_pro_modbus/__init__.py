"""The UNI PRO Modbus (TCP bridge) integration.

Talks standard Modbus TCP to a generic RTU<->TCP gateway (e.g. an ESP32
running a Modbus TCP-to-RTU bridge) that is wired via RS485 to a UNI2 PRO
or UNI3 PRO heating controller. All device-specific register knowledge
lives here, not on the bridge - the bridge/firmware stays generic and does
not need to be updated when this integration's register support changes.
"""
from __future__ import annotations

import logging

import voluptuous as vol
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import CONF_HOST, CONF_PORT, Platform
from homeassistant.core import HomeAssistant, ServiceCall
from homeassistant.exceptions import HomeAssistantError
from homeassistant.helpers import config_validation as cv

from .const import (
    CONF_CIRCUITS,
    CONF_CIRCULATION,
    CONF_DHW,
    CONF_SCAN_INTERVAL,
    CONF_UNIT_ID,
    DEFAULT_SCAN_INTERVAL,
    DOMAIN,
    SCHEDULE_DAYS,
    Model,
)
from .coordinator import UniProModbusCoordinator
from .registers import ActiveRegisters, build_active_registers

_LOGGER = logging.getLogger(__name__)

PLATFORMS: list[Platform] = [
    Platform.SENSOR,
    Platform.BINARY_SENSOR,
    Platform.NUMBER,
    Platform.SELECT,
    Platform.SWITCH,
]


async def async_setup_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    data = {**entry.data, **entry.options}

    model = Model(data["model"])
    active = build_active_registers(
        model=model,
        circuit_indexes=data.get(CONF_CIRCUITS, [1]),
        dhw=data.get(CONF_DHW, False),
        circulation=data.get(CONF_CIRCULATION, False),
    )

    coordinator = UniProModbusCoordinator(
        hass,
        entry,
        host=data[CONF_HOST],
        port=data[CONF_PORT],
        unit_id=data[CONF_UNIT_ID],
        scan_interval=data.get(CONF_SCAN_INTERVAL, DEFAULT_SCAN_INTERVAL),
        registers=active.all_defs(),
        schedules=active.all_schedules(),
    )

    await coordinator.async_config_entry_first_refresh()

    hass.data.setdefault(DOMAIN, {})
    hass.data[DOMAIN][entry.entry_id] = {
        "coordinator": coordinator,
        "active": active,
    }

    entry.async_on_unload(entry.add_update_listener(_async_update_listener))

    _async_register_services(hass)

    await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)
    return True


SERVICE_SET_SCHEDULE = "set_schedule"
ATTR_ENTRY_ID = "entry_id"
ATTR_SCHEDULE = "schedule"
ATTR_DAYS = "days"

_HOURS_SPEC = vol.Any(
    "all",
    "none",
    [vol.All(vol.Coerce(int), vol.Range(min=0, max=23))],
)
_SET_SCHEDULE_SCHEMA = vol.Schema(
    {
        vol.Optional(ATTR_ENTRY_ID): cv.string,
        vol.Required(ATTR_SCHEDULE): cv.string,
        vol.Required(ATTR_DAYS): vol.All(
            {vol.In(SCHEDULE_DAYS): _HOURS_SPEC},
            vol.Length(min=1),
        ),
    }
)


def _resolve_store(hass: HomeAssistant, entry_id: str | None) -> dict:
    stores = hass.data.get(DOMAIN, {})
    if entry_id is not None:
        store = stores.get(entry_id)
        if store is None:
            raise HomeAssistantError(f"No UNI PRO Modbus config entry {entry_id!r}")
        return store
    if len(stores) == 1:
        return next(iter(stores.values()))
    raise HomeAssistantError(
        "Multiple UNI PRO Modbus entries configured; specify 'entry_id'."
    )


def _async_register_services(hass: HomeAssistant) -> None:
    if hass.services.has_service(DOMAIN, SERVICE_SET_SCHEDULE):
        return

    async def _handle_set_schedule(call: ServiceCall) -> None:
        store = _resolve_store(hass, call.data.get(ATTR_ENTRY_ID))
        coordinator = store["coordinator"]
        active = store["active"]

        key = call.data[ATTR_SCHEDULE]
        sched = next((s for s in active.all_schedules() if s.key == key), None)
        if sched is None:
            available = ", ".join(s.key for s in active.all_schedules()) or "(none active)"
            raise HomeAssistantError(
                f"Unknown or inactive schedule {key!r}. Available: {available}"
            )

        # Read-modify-write: start from the current schedule so that days the
        # caller omits are left untouched. Fall back to all-off if not yet read.
        current = coordinator.get_schedule(sched)
        week = {
            day: list(current.get(day, [False] * 24)) for day in SCHEDULE_DAYS
        }
        for day, spec in call.data[ATTR_DAYS].items():
            if spec == "all":
                week[day] = [True] * 24
            elif spec == "none":
                week[day] = [False] * 24
            else:
                hours = [False] * 24
                for hour in spec:
                    hours[hour] = True
                week[day] = hours

        await coordinator.async_write_schedule(sched, week)

    hass.services.async_register(
        DOMAIN, SERVICE_SET_SCHEDULE, _handle_set_schedule, schema=_SET_SCHEDULE_SCHEMA
    )


async def _async_update_listener(hass: HomeAssistant, entry: ConfigEntry) -> None:
    await hass.config_entries.async_reload(entry.entry_id)


async def async_unload_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    unload_ok = await hass.config_entries.async_unload_platforms(entry, PLATFORMS)
    if unload_ok:
        store = hass.data[DOMAIN].pop(entry.entry_id)
        await store["coordinator"].async_close()
        if not hass.data[DOMAIN]:
            hass.services.async_remove(DOMAIN, SERVICE_SET_SCHEDULE)
    return unload_ok
