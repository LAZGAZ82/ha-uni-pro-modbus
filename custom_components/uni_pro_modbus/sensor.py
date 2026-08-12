"""Read-only numeric sensors (temperatures etc.)."""
from __future__ import annotations

from homeassistant.components.sensor import SensorDeviceClass, SensorEntity
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import CONF_HOST, UnitOfTemperature
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity_platform import AddEntitiesCallback

from .const import DOMAIN, MODEL_LABELS, DataType, Model
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
        UniProSensor(coordinator, entry.entry_id, device_name, model_label, reg)
        for reg in active.all_defs()
        if reg.data_type in (DataType.INT16_X10, DataType.UINT16) and not reg.writable
    ]
    async_add_entities(entities)


class UniProSensor(UniProModbusEntity, SensorEntity):
    def __init__(self, coordinator, entry_id, device_name, model_label, reg) -> None:
        super().__init__(coordinator, entry_id, device_name, model_label, reg)
        if reg.device_class == "temperature":
            self._attr_device_class = SensorDeviceClass.TEMPERATURE
            self._attr_native_unit_of_measurement = UnitOfTemperature.CELSIUS
            self._attr_suggested_display_precision = 1
        elif reg.unit:
            self._attr_native_unit_of_measurement = reg.unit

    @property
    def native_value(self):
        return self.coordinator.get_value(self._reg)
