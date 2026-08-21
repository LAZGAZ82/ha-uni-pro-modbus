"""Config flow for the UNI PRO Modbus (TCP bridge) integration."""
from __future__ import annotations

import inspect
import logging
from typing import Any

import voluptuous as vol
from homeassistant import config_entries
from homeassistant.const import CONF_HOST, CONF_PORT
from homeassistant.core import callback
from homeassistant.data_entry_flow import FlowResult
from homeassistant.helpers import config_validation as cv

from .const import (
    CONF_CIRCUITS,
    CONF_CIRCULATION,
    CONF_DHW,
    CONF_SCAN_INTERVAL,
    CONF_UNIT_ID,
    DEFAULT_HOST,
    DEFAULT_PORT,
    DEFAULT_SCAN_INTERVAL,
    DEFAULT_UNIT_ID,
    DOMAIN,
    MAX_CIRCUITS,
    MODEL_LABELS,
    MODEL_SUPPORTS_DHW_CIRCULATION,
    Model,
)
from .registers import build_active_registers

_LOGGER = logging.getLogger(__name__)


def _slave_kw(client) -> str:
    """pymodbus renamed the `slave` kwarg to `device_id` in newer releases
    (3.7+). Detect which one the actually-installed version expects from the
    real method signature, so this works across whatever pymodbus version
    each customer's Home Assistant happens to have bundled."""
    try:
        params = inspect.signature(client.read_holding_registers).parameters
    except (TypeError, ValueError):
        params = {}
    return "device_id" if "device_id" in params else "slave"


async def _validate_connection(hass, host: str, port: int, unit_id: int) -> None:
    """Try to read the general status register to confirm the bridge + device work."""
    from pymodbus.client import AsyncModbusTcpClient

    client = AsyncModbusTcpClient(host, port=port)
    try:
        await client.connect()
        if not client.connected:
            raise CannotConnect(f"Could not open TCP connection to {host}:{port}")
        result = await client.read_input_registers(
            address=0, count=1, **{_slave_kw(client): unit_id}
        )
        if result is None or result.isError():
            raise CannotConnect(
                f"Bridge reachable, but Modbus read from unit {unit_id} failed: {result}"
            )
    finally:
        client.close()


class CannotConnect(Exception):
    """Bridge/TCP unreachable, or device did not answer."""


class UniProModbusConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    """Handle the initial setup: bridge connection, then model/circuits."""

    VERSION = 1

    def __init__(self) -> None:
        self._data: dict[str, Any] = {}

    async def async_step_user(self, user_input: dict[str, Any] | None = None) -> FlowResult:
        errors: dict[str, str] = {}

        if user_input is not None:
            try:
                await _validate_connection(
                    self.hass,
                    user_input[CONF_HOST],
                    user_input[CONF_PORT],
                    user_input[CONF_UNIT_ID],
                )
            except CannotConnect as err:
                _LOGGER.warning("Connection test failed: %s", err)
                errors["base"] = "cannot_connect"
            except Exception:  # noqa: BLE001
                _LOGGER.exception("Unexpected error validating UNI PRO Modbus connection")
                errors["base"] = "unknown"
            else:
                self._data.update(user_input)
                return await self.async_step_circuits()

        # Re-show whatever the user just typed (not the hardcoded defaults)
        # when validation failed, so a failed attempt doesn't wipe the form.
        defaults = user_input or {}
        schema = vol.Schema(
            {
                vol.Required(
                    CONF_HOST, default=defaults.get(CONF_HOST, DEFAULT_HOST)
                ): str,
                vol.Required(
                    CONF_PORT, default=defaults.get(CONF_PORT, DEFAULT_PORT)
                ): int,
                vol.Required(
                    CONF_UNIT_ID, default=defaults.get(CONF_UNIT_ID, DEFAULT_UNIT_ID)
                ): vol.All(int, vol.Range(min=1, max=247)),
                vol.Required(
                    "model", default=defaults.get("model", Model.UNI3_PRO.value)
                ): vol.In({m.value: label for m, label in MODEL_LABELS.items()}),
                vol.Required(
                    CONF_SCAN_INTERVAL,
                    default=defaults.get(CONF_SCAN_INTERVAL, DEFAULT_SCAN_INTERVAL),
                ): vol.All(int, vol.Range(min=5, max=600)),
            }
        )
        return self.async_show_form(step_id="user", data_schema=schema, errors=errors)

    async def async_step_circuits(self, user_input: dict[str, Any] | None = None) -> FlowResult:
        model = Model(self._data["model"])
        supports_dhw_circ = MODEL_SUPPORTS_DHW_CIRCULATION[model]

        if user_input is not None:
            circuit_indexes = [int(i) for i in user_input[CONF_CIRCUITS]]
            self._data[CONF_CIRCUITS] = circuit_indexes
            self._data[CONF_DHW] = supports_dhw_circ and user_input.get(CONF_DHW, False)
            self._data[CONF_CIRCULATION] = supports_dhw_circ and user_input.get(
                CONF_CIRCULATION, False
            )

            title = f"{MODEL_LABELS[model]} ({self._data[CONF_HOST]})"
            return self.async_create_entry(title=title, data=self._data)

        circuit_options = {str(i): f"CO{i}" for i in range(1, MAX_CIRCUITS + 1)}
        schema_dict: dict[Any, Any] = {
            vol.Required(CONF_CIRCUITS, default=["1"]): cv.multi_select(circuit_options),
        }
        if supports_dhw_circ:
            schema_dict[vol.Required(CONF_DHW, default=False)] = bool
            schema_dict[vol.Required(CONF_CIRCULATION, default=False)] = bool

        return self.async_show_form(
            step_id="circuits",
            data_schema=vol.Schema(schema_dict),
            description_placeholders={"model": MODEL_LABELS[model]},
        )

    @staticmethod
    @callback
    def async_get_options_flow(config_entry: config_entries.ConfigEntry):
        return UniProModbusOptionsFlow(config_entry)


class UniProModbusOptionsFlow(config_entries.OptionsFlow):
    """Allows changing scan interval and active circuits/DHW/circulation later."""

    def __init__(self, config_entry: config_entries.ConfigEntry) -> None:
        self._entry = config_entry

    async def async_step_init(self, user_input: dict[str, Any] | None = None) -> FlowResult:
        model = Model(self._entry.data["model"])
        supports_dhw_circ = MODEL_SUPPORTS_DHW_CIRCULATION[model]
        current = {**self._entry.data, **self._entry.options}

        if user_input is not None:
            circuit_indexes = [int(i) for i in user_input[CONF_CIRCUITS]]
            options = {
                CONF_CIRCUITS: circuit_indexes,
                CONF_SCAN_INTERVAL: user_input[CONF_SCAN_INTERVAL],
                CONF_DHW: supports_dhw_circ and user_input.get(CONF_DHW, False),
                CONF_CIRCULATION: supports_dhw_circ
                and user_input.get(CONF_CIRCULATION, False),
            }
            return self.async_create_entry(title="", data=options)

        circuit_options = {str(i): f"CO{i}" for i in range(1, MAX_CIRCUITS + 1)}
        schema_dict: dict[Any, Any] = {
            vol.Required(
                CONF_CIRCUITS,
                default=[str(i) for i in current.get(CONF_CIRCUITS, [1])],
            ): cv.multi_select(circuit_options),
            vol.Required(
                CONF_SCAN_INTERVAL, default=current.get(CONF_SCAN_INTERVAL, DEFAULT_SCAN_INTERVAL)
            ): vol.All(int, vol.Range(min=5, max=600)),
        }
        if supports_dhw_circ:
            schema_dict[vol.Required(CONF_DHW, default=current.get(CONF_DHW, False))] = bool
            schema_dict[
                vol.Required(CONF_CIRCULATION, default=current.get(CONF_CIRCULATION, False))
            ] = bool

        return self.async_show_form(step_id="init", data_schema=vol.Schema(schema_dict))
