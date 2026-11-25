"""Writable enum registers (heating season mode, circulation control mode)."""
from __future__ import annotations

from homeassistant.components.select import SelectEntity
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
        UniProSelect(coordinator, entry.entry_id, device_name, model_label, reg)
        for reg in active.all_defs()
        if reg.data_type is DataType.ENUM16 and reg.writable
    ]
    async_add_entities(entities)


class UniProSelect(UniProModbusEntity, SelectEntity):
    def __init__(self, coordinator, entry_id, device_name, model_label, reg) -> None:
        super().__init__(coordinator, entry_id, device_name, model_label, reg)
        self._attr_options = list((reg.options or {}).values())

    @property
    def current_option(self) -> str | None:
        return self.coordinator.get_value(self._reg)

    async def async_select_option(self, option: str) -> None:
        await self.coordinator.async_write(self._reg, option)
