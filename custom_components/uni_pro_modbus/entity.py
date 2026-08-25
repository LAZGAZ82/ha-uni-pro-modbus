"""Shared base entity: wires each RegisterDef up to one HA device."""
from __future__ import annotations

from homeassistant.helpers.entity import DeviceInfo, EntityCategory
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from .const import DOMAIN, MANUFACTURER, RegisterDef
from .coordinator import UniProModbusCoordinator

_ENTITY_CATEGORY_MAP = {
    "diagnostic": EntityCategory.DIAGNOSTIC,
    "config": EntityCategory.CONFIG,
}


class UniProModbusEntity(CoordinatorEntity[UniProModbusCoordinator]):
    """Base class handling naming, unique_id and device grouping."""

    _attr_has_entity_name = True

    def __init__(
        self,
        coordinator: UniProModbusCoordinator,
        entry_id: str,
        device_name: str,
        model_label: str,
        reg: RegisterDef,
    ) -> None:
        super().__init__(coordinator)
        self._reg = reg
        self._attr_unique_id = f"{entry_id}_{reg.key}"
        self._attr_name = reg.name
        self._attr_icon = reg.icon
        if reg.entity_category:
            self._attr_entity_category = _ENTITY_CATEGORY_MAP.get(reg.entity_category)

        self._attr_device_info = DeviceInfo(
            identifiers={(DOMAIN, entry_id)},
            name=device_name,
            manufacturer=MANUFACTURER,
            model=model_label,
        )

    @property
    def available(self) -> bool:
        return super().available and self.coordinator.get_raw(self._reg) is not None
