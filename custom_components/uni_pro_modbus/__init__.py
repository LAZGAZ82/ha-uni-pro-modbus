"""The UNI PRO Modbus (TCP bridge) integration.

Talks standard Modbus TCP to a generic RTU<->TCP gateway (e.g. an ESP32
running a Modbus TCP-to-RTU bridge) that is wired via RS485 to a UNI2 PRO
or UNI3 PRO heating controller. All device-specific register knowledge
lives here, not on the bridge - the bridge/firmware stays generic and does
not need to be updated when this integration's register support changes.
"""
from __future__ import annotations

import logging

from homeassistant.config_entries import ConfigEntry
from homeassistant.const import CONF_HOST, CONF_PORT, Platform
from homeassistant.core import HomeAssistant

from .const import (
    CONF_CIRCUITS,
    CONF_CIRCULATION,
    CONF_DHW,
    CONF_SCAN_INTERVAL,
    CONF_UNIT_ID,
    DEFAULT_SCAN_INTERVAL,
    DOMAIN,
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
    )

    await coordinator.async_config_entry_first_refresh()

    hass.data.setdefault(DOMAIN, {})
    hass.data[DOMAIN][entry.entry_id] = {
        "coordinator": coordinator,
        "active": active,
    }

    entry.async_on_unload(entry.add_update_listener(_async_update_listener))

    await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)
    return True


async def _async_update_listener(hass: HomeAssistant, entry: ConfigEntry) -> None:
    await hass.config_entries.async_reload(entry.entry_id)


async def async_unload_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    unload_ok = await hass.config_entries.async_unload_platforms(entry, PLATFORMS)
    if unload_ok:
        store = hass.data[DOMAIN].pop(entry.entry_id)
        await store["coordinator"].async_close()
    return unload_ok
