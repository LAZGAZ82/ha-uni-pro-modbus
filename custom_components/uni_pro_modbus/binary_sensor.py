"""Read-only status bits (pump running, sensor errors, enabled flags, ...)."""
from __future__ import annotations

from homeassistant.components.binary_sensor import BinarySensorDeviceClass, BinarySensorEntity
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import CONF_HOST
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity_platform import AddEntitiesCallback

from .const import DOMAIN, MODEL_LABELS, DataType
from .entity import UniProModbusEntity

_DEVICE_CLASS_MAP = {
    "problem": BinarySensorDeviceClass.PROBLEM,
    "running": BinarySensorDeviceClass.RUNNING,
}


async def async_setup_entry(
    hass: HomeAssistant, entry: ConfigEntry, async_add_entities: AddEntitiesCallback
) -> None:
    store = hass.data[DOMAIN][entry.entry_id]
    coordinator = store["coordinator"]
    active = store["active"]
    model_label = MODEL_LABELS[active.model]
    device_name = f"{model_label} ({entry.data[CONF_HOST]})"

    entities = [
        UniProBinarySensor(coordinator, entry.entry_id, device_name, model_label, reg)
        for reg in active.all_defs()
        if reg.data_type is DataType.BIT and not reg.writable
    ]
    async_add_entities(entities)


class UniProBinarySensor(UniProModbusEntity, BinarySensorEntity):
    def __init__(self, coordinator, entry_id, device_name, model_label, reg) -> None:
        super().__init__(coordinator, entry_id, device_name, model_label, reg)
        if reg.device_class in _DEVICE_CLASS_MAP:
            self._attr_device_class = _DEVICE_CLASS_MAP[reg.device_class]

    @property
    def is_on(self) -> bool | None:
        return self.coordinator.get_value(self._reg)
