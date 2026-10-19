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
    CONF_REMOTE_ENABLED,
    CONF_REMOTE_INTERVAL,
    CONF_REMOTE_KEY,
    CONF_REMOTE_URL,
    CONF_SCAN_INTERVAL,
    CONF_UNIT_ID,
    DEFAULT_HOST,
    DEFAULT_PORT,
    DEFAULT_REMOTE_INTERVAL,
    DEFAULT_SCAN_INTERVAL,
    DEFAULT_TIMEOUT,
    DEFAULT_UNIT_ID,
    DOMAIN,
    MODEL_LABELS,
    MODEL_SUPPORTS_DHW_CIRCULATION,
    Model,
    available_circuits,
    build_circuit,
)

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


async def _read_active_circuits(
    hass, host: str, port: int, unit_id: int, model: Model
) -> list[int] | None:
    """Read each circuit's on/off register (40x01, bit 0) and return the list
    of circuit indexes the controller reports as active.

    Only circuits valid for the model are probed (UNI2 PRO skips CO3). Returns
    None on any read error so the caller can fall back to the previous
    behaviour (preselect CO1 only). An empty list (controller answered but no
    circuit is enabled) is returned as-is; callers treat that like the
    fallback since a controller always drives at least one circuit."""
    from pymodbus.client import AsyncModbusTcpClient

    client = AsyncModbusTcpClient(host, port=port, timeout=DEFAULT_TIMEOUT)
    try:
        await client.connect()
        if not client.connected:
            return None
        active: list[int] = []
        for index in available_circuits(model):
            reg = build_circuit(index).enabled
            result = await client.read_holding_registers(
                address=reg.address, count=1, **{_slave_kw(client): unit_id}
            )
            if result is None or result.isError():
                _LOGGER.debug(
                    "Auto-detect: read of CO%s enable register failed: %s",
                    index,
                    result,
                )
                return None
            if result.registers[0] & (1 << (reg.bit or 0)):
                active.append(index)
        _LOGGER.debug("Auto-detected active circuits: %s", active)
        return active
    except Exception:  # noqa: BLE001
        _LOGGER.exception("Could not auto-detect active circuits")
        return None
    finally:
        client.close()


class CannotConnect(Exception):
    """Bridge/TCP unreachable, or device did not answer."""


class UniProModbusConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    """Handle the initial setup: bridge connection, then model/circuits."""

    VERSION = 1

    def __init__(self) -> None:
        self._data: dict[str, Any] = {}
        # Circuits the controller reported as active at setup time (None = the
        # probe failed; fall back to preselecting CO1 only).
        self._detected_circuits: list[int] | None = None

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
                # Probe which circuits are enabled so the next step can
                # preselect them. Best-effort; failure just falls back to CO1.
                self._detected_circuits = await _read_active_circuits(
                    self.hass,
                    user_input[CONF_HOST],
                    user_input[CONF_PORT],
                    user_input[CONF_UNIT_ID],
                    Model(user_input["model"]),
                )
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
        available = available_circuits(model)

        if user_input is not None:
            circuit_indexes = [int(i) for i in user_input[CONF_CIRCUITS]]
            self._data[CONF_CIRCUITS] = circuit_indexes
            self._data[CONF_DHW] = supports_dhw_circ and user_input.get(CONF_DHW, False)
            self._data[CONF_CIRCULATION] = supports_dhw_circ and user_input.get(
                CONF_CIRCULATION, False
            )

            title = f"{MODEL_LABELS[model]} ({self._data[CONF_HOST]})"
            return self.async_create_entry(title=title, data=self._data)

        circuit_options = {str(i): f"CO{i}" for i in available}
        # Preselect the circuits the controller reported as active; if the
        # probe failed or reported none, fall back to CO1 only (as before).
        detected = [i for i in (self._detected_circuits or []) if i in available]
        default_circuits = [str(i) for i in detected] or ["1"]
        schema_dict: dict[Any, Any] = {
            vol.Required(
                CONF_CIRCUITS, default=default_circuits
            ): cv.multi_select(circuit_options),
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
        available = available_circuits(model)
        errors: dict[str, str] = {}

        if user_input is not None:
            remote_enabled = user_input.get(CONF_REMOTE_ENABLED, False)
            remote_url = (user_input.get(CONF_REMOTE_URL) or "").strip()
            remote_key = (user_input.get(CONF_REMOTE_KEY) or "").strip()
            # Fernwartung nur zulassen, wenn Endpoint + Key gesetzt sind und der
            # Endpoint HTTPS ist (Messdaten dürfen nicht im Klartext gehen).
            if remote_enabled and (not remote_url or not remote_key):
                errors["base"] = "remote_incomplete"
            elif remote_enabled and not remote_url.lower().startswith("https://"):
                errors["base"] = "remote_not_https"

            if not errors:
                circuit_indexes = [int(i) for i in user_input[CONF_CIRCUITS]]
                options = {
                    CONF_CIRCUITS: circuit_indexes,
                    CONF_SCAN_INTERVAL: user_input[CONF_SCAN_INTERVAL],
                    CONF_DHW: supports_dhw_circ and user_input.get(CONF_DHW, False),
                    CONF_CIRCULATION: supports_dhw_circ
                    and user_input.get(CONF_CIRCULATION, False),
                    CONF_REMOTE_ENABLED: remote_enabled,
                    CONF_REMOTE_URL: remote_url,
                    CONF_REMOTE_KEY: remote_key,
                    CONF_REMOTE_INTERVAL: user_input.get(
                        CONF_REMOTE_INTERVAL, DEFAULT_REMOTE_INTERVAL
                    ),
                }
                return self.async_create_entry(title="", data=options)

        # Re-probe the live controller so a module added after the first setup
        # shows up preselected. If the probe fails, keep the currently stored
        # selection (better than resetting an existing install to CO1).
        detected = await _read_active_circuits(
            self.hass,
            self._entry.data[CONF_HOST],
            self._entry.data[CONF_PORT],
            self._entry.data[CONF_UNIT_ID],
            model,
        )
        if detected:
            preselect = [i for i in detected if i in available]
        else:
            preselect = [i for i in current.get(CONF_CIRCUITS, [1]) if i in available]
        default_circuits = [str(i) for i in preselect] or ["1"]

        circuit_options = {str(i): f"CO{i}" for i in available}
        schema_dict: dict[Any, Any] = {
            vol.Required(
                CONF_CIRCUITS, default=default_circuits
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

        # Fernwartung (opt-in). Nach einem Fehler die gerade eingegebenen Werte
        # wieder anzeigen statt die gespeicherten.
        rsrc = user_input if user_input is not None else current
        schema_dict[
            vol.Required(
                CONF_REMOTE_ENABLED, default=rsrc.get(CONF_REMOTE_ENABLED, False)
            )
        ] = bool
        schema_dict[
            vol.Optional(CONF_REMOTE_URL, default=rsrc.get(CONF_REMOTE_URL, ""))
        ] = str
        schema_dict[
            vol.Optional(CONF_REMOTE_KEY, default=rsrc.get(CONF_REMOTE_KEY, ""))
        ] = str
        schema_dict[
            vol.Required(
                CONF_REMOTE_INTERVAL,
                default=rsrc.get(CONF_REMOTE_INTERVAL, DEFAULT_REMOTE_INTERVAL),
            )
        ] = vol.All(int, vol.Range(min=60, max=3600))

        return self.async_show_form(
            step_id="init", data_schema=vol.Schema(schema_dict), errors=errors
        )
