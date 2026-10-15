"""Assembles the active register set for one config entry based on its
model / circuit / DHW / circulation options."""
from __future__ import annotations

from dataclasses import dataclass

from .const import (
    CIRC_CONTROL_MODE,
    CIRC_ENABLED,
    CIRC_SCHEDULE,
    CIRC_SENSOR_ERROR,
    CIRC_SETPOINT_TEMP,
    CIRC_STATUS,
    CIRC_TEMP_ACTIVATION_MINUTES,
    CIRC_TEMPERATURE,
    DHW_DISINFECTION,
    DHW_ENABLED,
    DHW_SCHEDULE,
    DHW_SENSOR_ERROR,
    DHW_SET_TEMP,
    DHW_STATUS,
    DHW_TEMP_ACTIVATION_MINUTES,
    DHW_TEMP_ACTIVATION_TEMP,
    DHW_TEMPERATURE,
    GENERAL_CONFIG,
    GENERAL_SENSORS,
    GENERAL_STATUS_BITS,
    CircuitRegisters,
    Model,
    RegisterDef,
    ScheduleDef,
    build_circuit,
    build_circuit_schedule,
)


@dataclass
class ActiveRegisters:
    model: Model
    circuits: list[CircuitRegisters]
    dhw: bool
    circulation: bool

    def all_schedules(self) -> list[ScheduleDef]:
        """Weekly on/off schedules for every active loop (CO1-CO7, DHW,
        Circulation)."""
        scheds: list[ScheduleDef] = [
            build_circuit_schedule(circuit.index) for circuit in self.circuits
        ]
        if self.dhw:
            scheds.append(DHW_SCHEDULE)
        if self.circulation:
            scheds.append(CIRC_SCHEDULE)
        return scheds

    def all_defs(self) -> list[RegisterDef]:
        defs: list[RegisterDef] = [*GENERAL_STATUS_BITS, *GENERAL_SENSORS, *GENERAL_CONFIG]

        for circuit in self.circuits:
            defs.extend(
                [
                    circuit.status,
                    circuit.temperature,
                    circuit.enabled,
                    circuit.set_temp,
                    circuit.curve_minus20,
                    circuit.curve_minus5,
                    circuit.curve_plus5,
                    circuit.curve_plus15,
                    circuit.temp_activation_minutes,
                    circuit.temp_activation_temp,
                    circuit.pump_bit,
                    circuit.room_input_bit,
                    circuit.sensor_error_bit,
                ]
            )

        if self.dhw:
            defs.extend(
                [
                    DHW_STATUS,
                    DHW_SENSOR_ERROR,
                    DHW_TEMPERATURE,
                    DHW_ENABLED,
                    DHW_DISINFECTION,
                    DHW_SET_TEMP,
                    DHW_TEMP_ACTIVATION_MINUTES,
                    DHW_TEMP_ACTIVATION_TEMP,
                ]
            )

        if self.circulation:
            defs.extend(
                [
                    CIRC_STATUS,
                    CIRC_SENSOR_ERROR,
                    CIRC_TEMPERATURE,
                    CIRC_ENABLED,
                    CIRC_CONTROL_MODE,
                    CIRC_SETPOINT_TEMP,
                    CIRC_TEMP_ACTIVATION_MINUTES,
                ]
            )

        return defs


def build_active_registers(
    model: Model, circuit_indexes: list[int], dhw: bool, circulation: bool
) -> ActiveRegisters:
    circuits = [build_circuit(i) for i in sorted(circuit_indexes)]
    supports_dhw_circ = model is Model.UNI2_PRO
    return ActiveRegisters(
        model=model,
        circuits=circuits,
        dhw=dhw and supports_dhw_circ,
        circulation=circulation and supports_dhw_circ,
    )
