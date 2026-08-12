"""Writable numeric values (setpoints, weather-curve points, timers)."""
from __future__ import annotations

from homeassistant.components.number import NumberDeviceClass, NumberEntity, NumberMode
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import CONF_HOST, UnitOfTemperature, UnitOfTime
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
        UniProNumber(coordinator, entry.entry_id, device_name, model_label, reg)
        for reg in active.all_defs()
        if reg.data_type in (DataType.INT16_X10, DataType.UINT16) and reg.writable
    ]
    async_add_entities(entities)


class UniProNumber(UniProModbusEntity, NumberEntity):
    _attr_mode = NumberMode.BOX

    def __init__(self, coordinator, entry_id, device_name, model_label, reg) -> None:
        super().__init__(coordinator, entry_id, device_name, model_label, reg)

        if reg.device_class == "temperature":
            self._attr_device_class = NumberDeviceClass.TEMPERATURE
            self._attr_native_unit_of_measurement = UnitOfTemperature.CELSIUS
            self._attr_native_min_value = -30
            self._attr_native_max_value = 90
            self._attr_native_step = 0.1
        elif reg.unit == "min":
            self._attr_native_unit_of_measurement = UnitOfTime.MINUTES
            # Manual: adjustable 30-5940 min, 0 disables timed activation.
            self._attr_native_min_value = 0
            self._attr_native_max_value = 5940
            self._attr_native_step = 1
        else:
            self._attr_native_min_value = 0
            self._attr_native_max_value = 65535
            self._attr_native_step = 1

    @property
    def native_value(self):
        return self.coordinator.get_value(self._reg)

    async def async_set_native_value(self, value: float) -> None:
        await self.coordinator.async_write(self._reg, value)
