"""Read-only numeric sensors (temperatures etc.)."""
from __future__ import annotations

from homeassistant.components.sensor import SensorDeviceClass, SensorEntity
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import CONF_HOST, EntityCategory, UnitOfTemperature
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity import DeviceInfo
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from .const import DOMAIN, MANUFACTURER, MODEL_LABELS, DataType, Model
from .entity import UniProModbusEntity
from .schedule import active_hour_count, hours_to_ranges


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
    entities += [
        UniProScheduleSensor(coordinator, entry.entry_id, device_name, model_label, sched)
        for sched in active.all_schedules()
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


class UniProScheduleSensor(CoordinatorEntity, SensorEntity):
    """Read-only view of a weekly on/off schedule.

    State is the number of active hours in the week (0-168). The full
    schedule is exposed in attributes, both as human-readable per-day time
    ranges (`schedule`) and as raw per-hour booleans (`weekly`) for use in
    automations/templates. Writing is done via the `uni_pro_modbus.set_schedule`
    service.
    """

    _attr_has_entity_name = True
    _attr_entity_category = EntityCategory.DIAGNOSTIC
    _attr_icon = "mdi:calendar-clock"
    _attr_native_unit_of_measurement = "h"

    def __init__(self, coordinator, entry_id, device_name, model_label, sched) -> None:
        super().__init__(coordinator)
        self._sched = sched
        self._attr_unique_id = f"{entry_id}_{sched.key}"
        self._attr_name = sched.name
        self._attr_device_info = DeviceInfo(
            identifiers={(DOMAIN, entry_id)},
            name=device_name,
            manufacturer=MANUFACTURER,
            model=model_label,
        )

    @property
    def available(self) -> bool:
        return super().available and bool(self.coordinator.get_schedule(self._sched))

    @property
    def native_value(self):
        week = self.coordinator.get_schedule(self._sched)
        if not week:
            return None
        return active_hour_count(week)

    @property
    def extra_state_attributes(self):
        week = self.coordinator.get_schedule(self._sched)
        if not week:
            return None
        return {
            "schedule": {day: hours_to_ranges(hours) for day, hours in week.items()},
            "weekly": week,
        }
