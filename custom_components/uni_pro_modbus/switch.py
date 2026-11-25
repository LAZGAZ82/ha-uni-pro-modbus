"""Writable single-purpose bit registers (e.g. DHW disinfection).

Note: circuit/DHW/circulation on-off registers (40101, 40801, 40901) are
documented as read-only (function 03 only, no 06/16 support) and are
therefore exposed as binary_sensor, not switch. Only registers explicitly
marked "Possible Data Overwrite: YES" with DataType.BIT land here.
"""
from __future__ import annotations

from homeassistant.components.switch import SwitchEntity
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import CONF_HOST
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity_platform import AddEntitiesCallback

from .const import DOMAIN, MODEL_LABELS, DataType
from .entity import UniProModbusEntity


async def async_setup_entry(
    hass: HomeAssistant, entry: ConfigEntry, async_add_entities: AddEntitiesCallback
) -> None:
    store = hass.data[DOMAIN][entry.entry_id]
    coordinator = store["coordinator"]
    active = store["active"]
    model_label = MODEL_LABELS[active.model]
    device_name = f"{model_label} ({entry.data[CONF_HOST]})"

    entities = [
        UniProSwitch(coordinator, entry.entry_id, device_name, model_label, reg)
        for reg in active.all_defs()
        if reg.data_type is DataType.BIT and reg.writable
    ]
    async_add_entities(entities)


class UniProSwitch(UniProModbusEntity, SwitchEntity):
    @property
    def is_on(self) -> bool | None:
        return self.coordinator.get_value(self._reg)

    async def async_turn_on(self, **kwargs) -> None:
        await self.coordinator.async_write(self._reg, True)

    async def async_turn_off(self, **kwargs) -> None:
        await self.coordinator.async_write(self._reg, False)
