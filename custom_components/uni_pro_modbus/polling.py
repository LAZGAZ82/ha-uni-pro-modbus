"""Fault-tolerant Modbus polling over ONE persistent TCP connection (v0.3.1).

Deliberately free of Home Assistant imports so it can be unit-tested on its
own (tests/test_polling.py). The coordinator wraps it.

Field test 23.11.2026 (UNI2 PRO + UNIM PRO + ESP32 bridge 1.0.1):

* A register the controller does not answer (bus problem, or everything for
  ~1 min after the controller was switched off and on) comes back from the
  bridge as Modbus exception 0xE0 (RTU timeout). In v0.3.0 that
  failed the whole update and kept the entry in setup_retry. Now only the
  affected block is missing (its entities go unavailable); the update fails
  only if NOTHING could be read.
* The bridge accepts at most 2 TCP clients and stops accepting connections on
  port 502 after errors / parallel clients until power-cycled. Hence: exactly
  one connection per entry, every request serialised through one lock, and on
  any transport error the connection is closed and re-opened on the next
  cycle (never left half-dead, never a second client in parallel).
* The bridge's own RTU timeout is 2 s; the TCP timeout must be clearly longer
  (>= 4 s), otherwise pymodbus gives up first and the late answer shows up as
  "received pdu without a corresponding request". Retries are reduced so
  requests do not queue up inside the bridge.
"""
from __future__ import annotations

import asyncio
import inspect
import logging
from dataclasses import dataclass, field
from typing import Any, Awaitable, Callable

_LOGGER = logging.getLogger(__name__)

TCP_TIMEOUT = 4.0          # seconds; bridge RTU timeout is 2 s
TCP_RETRIES = 0            # no stacking of repeated requests inside the bridge
DEFAULT_REQUEST_PAUSE_MS = 50
MAX_BLOCK_LEN = 16         # registers per block read (≈ 40 ms on the wire at 9600 baud)

INPUT = "input"
HOLDING = "holding"


class PollingError(Exception):
    """Nothing at all could be read in this cycle."""


@dataclass(frozen=True)
class Block:
    """One read request: `count` contiguous registers starting at `address`."""

    kind: str               # INPUT or HOLDING
    address: int
    count: int
    keys: tuple[str, ...]   # cache key per register (single blocks) ...
    whole_key: str | None = None  # ... or one key for the whole list (schedules)


def cache_key(kind: str, address: int) -> str:
    return f"{kind}:{address}"


def plan_blocks(
    registers: list[tuple[str, int]],
    schedules: list[tuple[str, int, int]] | None = None,
    max_len: int = MAX_BLOCK_LEN,
) -> list[Block]:
    """Group single registers into contiguous blocks (per register kind).

    `registers`: (kind, raw address) pairs, duplicates allowed.
    `schedules`: (cache key, raw holding start address, count) - each read as
    its own block (they are 11 contiguous registers and stored as a list).
    Non-contiguous registers become blocks of length 1.
    """
    blocks: list[Block] = []
    for kind in (INPUT, HOLDING):
        addrs = sorted({a for k, a in registers if k == kind})
        run: list[int] = []
        for addr in addrs:
            if run and (addr != run[-1] + 1 or len(run) >= max_len):
                blocks.append(_block(kind, run))
                run = []
            run.append(addr)
        if run:
            blocks.append(_block(kind, run))
    for key, start, count in schedules or []:
        blocks.append(Block(HOLDING, start, count, (), whole_key=key))
    return blocks


def _block(kind: str, run: list[int]) -> Block:
    return Block(kind, run[0], len(run), tuple(cache_key(kind, a) for a in run))


def slave_kwarg(method: Callable) -> str:
    """pymodbus renamed `slave` to `device_id` (3.7+); use what is installed."""
    try:
        params = inspect.signature(method).parameters
    except (TypeError, ValueError):
        params = {}
    return "device_id" if "device_id" in params else "slave"


def make_tcp_client(host: str, port: int) -> Any:
    """AsyncModbusTcpClient with timeout >= 4 s, no retries, no background
    auto-reconnect (we reconnect ourselves, on the next cycle) - passing only
    the keywords the installed pymodbus version knows."""
    from pymodbus.client import AsyncModbusTcpClient

    wanted = {"timeout": TCP_TIMEOUT, "retries": TCP_RETRIES, "reconnect_delay": 0}
    try:
        params = inspect.signature(AsyncModbusTcpClient.__init__).parameters
        accepts_any = any(p.kind is p.VAR_KEYWORD for p in params.values())
        kwargs = {k: v for k, v in wanted.items() if accepts_any or k in params}
    except (TypeError, ValueError):
        kwargs = {"timeout": TCP_TIMEOUT}
    return AsyncModbusTcpClient(host, port=port, **kwargs)


@dataclass
class CycleResult:
    data: dict[str, Any] = field(default_factory=dict)
    failed: list[Block] = field(default_factory=list)
    transport_error: str | None = None


class ModbusSession:
    """Exactly one TCP connection, all requests serialised, pause between them."""

    def __init__(
        self,
        client_factory: Callable[[], Any],
        unit_id: int,
        request_pause_ms: int = DEFAULT_REQUEST_PAUSE_MS,
        sleep: Callable[[float], Awaitable[None]] = asyncio.sleep,
    ) -> None:
        self._factory = client_factory
        self._unit_id = unit_id
        self._pause = max(0, request_pause_ms) / 1000
        self._sleep = sleep
        self._client: Any | None = None
        self._kw: str | None = None
        self._lock = asyncio.Lock()
        self._requests_sent = 0
        self.connects = 0  # for diagnostics / tests

    # -- connection ---------------------------------------------------------
    async def _ensure(self) -> Any:
        if self._client is not None and getattr(self._client, "connected", False):
            return self._client
        self._close_now()
        client = self._factory()
        self._client = client
        try:
            await client.connect()
        except Exception as err:  # noqa: BLE001
            self._close_now()
            raise ConnectionError(f"connect failed: {err}") from err
        if not getattr(client, "connected", False):
            self._close_now()
            raise ConnectionError("bridge did not accept the TCP connection")
        self.connects += 1
        self._requests_sent = 0
        return client

    def _close_now(self) -> None:
        client, self._client = self._client, None
        if client is not None:
            try:
                client.close()
            except Exception:  # noqa: BLE001
                _LOGGER.debug("close() raised", exc_info=True)

    async def close(self) -> None:
        async with self._lock:
            self._close_now()

    @property
    def connected(self) -> bool:
        return self._client is not None and bool(getattr(self._client, "connected", False))

    # -- requests -------------------------------------------------------------
    async def _call(self, client: Any, method_name: str, **kwargs) -> Any:
        method = getattr(client, method_name)
        if self._kw is None:
            self._kw = slave_kwarg(method)
        if self._requests_sent and self._pause:
            await self._sleep(self._pause)
        self._requests_sent += 1
        return await method(**kwargs, **{self._kw: self._unit_id})

    async def read_cycle(self, blocks: list[Block]) -> CycleResult:
        """Read every block once. A Modbus error response (e.g. 0xE0) only
        marks that block as failed; a transport error closes the connection
        and ends the cycle (the next cycle reconnects)."""
        res = CycleResult()
        async with self._lock:
            try:
                client = await self._ensure()
            except ConnectionError as err:
                res.transport_error = str(err)
                res.failed = list(blocks)
                return res
            for i, block in enumerate(blocks):
                method = "read_input_registers" if block.kind == INPUT else "read_holding_registers"
                try:
                    result = await self._call(client, method, address=block.address, count=block.count)
                except Exception as err:  # noqa: BLE001 - transport level
                    self._close_now()
                    res.transport_error = f"{type(err).__name__}: {err}"
                    res.failed.extend(blocks[i:])
                    return res
                regs = getattr(result, "registers", None)
                if result is None or result.isError() or regs is None or len(regs) < block.count:
                    _LOGGER.debug("Block %s@%s x%s failed: %s", block.kind, block.address, block.count, result)
                    res.failed.append(block)
                    continue
                if block.whole_key is not None:
                    res.data[block.whole_key] = list(regs[: block.count])
                else:
                    for key, value in zip(block.keys, regs):
                        res.data[key] = value
        return res

    async def write(self, method_name: str, **kwargs) -> Any:
        """Single write through the same connection; closes it on transport errors."""
        async with self._lock:
            client = await self._ensure()
            try:
                return await self._call(client, method_name, **kwargs)
            except Exception:
                self._close_now()
                raise
