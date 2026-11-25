"""Optionaler Fern-Monitoring-Push ("Fernwartung") - ausdrücklich opt-in.

Wenn der Nutzer in den Integrations-Optionen "Fernwartung" aktiviert und einen
Endpoint + Zugangs-Key einträgt, werden die aktuellen Registerwerte in einem
festen Intervall per HTTPS an genau diesen Endpoint gesendet (Header X-Api-Key).

Standardmäßig AUS. Ohne Aktivierung durch den Nutzer wird nichts gesendet -
kein verstecktes "phone home". Schlägt ein Push fehl, wird das nur geloggt;
der normale Modbus-Betrieb von Home Assistant ist davon nie betroffen.
"""
from __future__ import annotations

import logging
import time
from datetime import datetime, timezone

from aiohttp import ClientTimeout
from homeassistant.core import HomeAssistant, callback
from homeassistant.helpers.aiohttp_client import async_get_clientsession

from .coordinator import UniProModbusCoordinator
from .registers import ActiveRegisters

_LOGGER = logging.getLogger(__name__)

_PUSH_TIMEOUT = ClientTimeout(total=15)


class RemoteReporter:
    """Sendet die aktuellen Registerwerte gedrosselt an einen HTTPS-Endpoint."""

    def __init__(
        self,
        hass: HomeAssistant,
        coordinator: UniProModbusCoordinator,
        active: ActiveRegisters,
        url: str,
        api_key: str,
        interval: int,
    ) -> None:
        self._hass = hass
        self._coordinator = coordinator
        self._active = active
        self._url = url.strip()
        self._key = api_key.strip()
        # Nie öfter als alle 60 s, egal wie klein das Intervall gesetzt wird.
        self._interval = max(60, int(interval))
        self._last_push = 0.0
        self._pushing = False
        self._unsub = None

    @callback
    def start(self) -> None:
        """An Coordinator-Updates hängen (feuert bei jedem erfolgreichen Poll)."""
        self._unsub = self._coordinator.async_add_listener(self._on_update)

    @callback
    def stop(self) -> None:
        if self._unsub is not None:
            self._unsub()
            self._unsub = None

    @callback
    def _on_update(self) -> None:
        now = time.monotonic()
        if self._pushing or (now - self._last_push) < self._interval:
            return
        self._last_push = now
        self._hass.async_create_task(self._async_push())

    def _build_payload(self) -> dict:
        readings: dict[str, object] = {}
        for reg in self._active.all_defs():
            val = self._coordinator.get_value(reg)
            if val is None:
                continue
            if isinstance(val, bool):
                val = 1 if val else 0
            readings[reg.key] = val
        return {
            "device_ts": datetime.now(timezone.utc).isoformat(),
            "readings": readings,
        }

    async def _async_push(self) -> None:
        self._pushing = True
        try:
            payload = self._build_payload()
            if not payload["readings"]:
                return
            session = async_get_clientsession(self._hass)
            async with session.post(
                self._url,
                json=payload,
                headers={"X-Api-Key": self._key},
                timeout=_PUSH_TIMEOUT,
            ) as resp:
                if resp.status == 200:
                    _LOGGER.debug(
                        "Fernwartung: %d Werte an %s gesendet",
                        len(payload["readings"]),
                        self._url,
                    )
                else:
                    text = (await resp.text())[:200]
                    _LOGGER.warning(
                        "Fernwartung: Push-Antwort HTTP %s von %s: %s",
                        resp.status,
                        self._url,
                        text,
                    )
        except Exception as err:  # noqa: BLE001 - Push darf HA niemals stören
            _LOGGER.debug("Fernwartung: Push fehlgeschlagen: %s", err)
        finally:
            self._pushing = False
