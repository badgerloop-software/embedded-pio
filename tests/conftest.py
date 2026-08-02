"""Shared helpers for SC2 python-can contract tests."""

from __future__ import annotations

import struct
import uuid
from typing import Iterator

import can
import pytest

from load_can_ids import CAN


@pytest.fixture
def can_pair() -> Iterator[tuple[can.BusABC, can.BusABC]]:
    """Paired virtual buses on one channel (TX + RX). Works on Windows/Linux."""
    channel = f"sc2-{uuid.uuid4().hex}"
    tx = can.Bus(interface="virtual", channel=channel, receive_own_messages=False)
    rx = can.Bus(interface="virtual", channel=channel, receive_own_messages=False)
    try:
        yield tx, rx
    finally:
        tx.shutdown()
        rx.shutdown()


def send(bus: can.BusABC, arb_id: int, data: bytes) -> None:
    bus.send(can.Message(arbitration_id=arb_id, data=data, is_extended_id=False))


def pack_float(value: float) -> bytes:
    return struct.pack("<f", value)


def unpack_float(data: bytes) -> float:
    return struct.unpack("<f", data[:4])[0]


def pack_u16(value: int) -> bytes:
    return struct.pack("<H", value & 0xFFFF)


def unpack_u16(data: bytes) -> int:
    return struct.unpack("<H", data[:2])[0]


def pack_be_u16(value: int) -> bytes:
    return struct.pack(">H", value & 0xFFFF)


# Bit layouts must match steering can_steering.cpp / PDC Digital_Data / lighting BIT_OFF*.
STEERING_BIT_HEADLIGHT = 0
STEERING_BIT_LEFT = 1
STEERING_BIT_RIGHT = 2
STEERING_BIT_DIRECTION = 3
STEERING_BIT_HORN = 4

PDC_BIT_DIRECTION = 0  # reverse lamp on rear lighting
PDC_BIT_MC_SPEED = 1
PDC_BIT_ECO = 2
PDC_BIT_MCU_MC_ON = 3
PDC_BIT_PARK_BRAKE = 4
PDC_BIT_BRAKE_LED = 5  # brake lamp on lighting boards

# Must match FAULT_CLEAR_PAYLOAD in sc2-powertrain can_powertrain.cpp
FAULT_CLEAR_PAYLOAD = bytes([0x03, 0x7F, 0x20, 0x22, 0x00, 0x00, 0x00, 0x00])

CELL_V_MIN = 2.52
CELL_V_MAX = 4.18
THROTTLE_SENT_MAX = 4095


def steering_digital(*, headlight=0, left=0, right=0, direction=0, horn=0) -> int:
    return (
        (1 if headlight else 0) << STEERING_BIT_HEADLIGHT
        | (1 if left else 0) << STEERING_BIT_LEFT
        | (1 if right else 0) << STEERING_BIT_RIGHT
        | (1 if direction else 0) << STEERING_BIT_DIRECTION
        | (1 if horn else 0) << STEERING_BIT_HORN
    )


def pdc_digital(
    *,
    direction=0,
    mc_speed=0,
    eco=0,
    mcu_mc_on=0,
    park_brake=0,
    brake_led=0,
) -> int:
    return (
        (1 if direction else 0) << PDC_BIT_DIRECTION
        | (1 if mc_speed else 0) << PDC_BIT_MC_SPEED
        | (1 if eco else 0) << PDC_BIT_ECO
        | (1 if mcu_mc_on else 0) << PDC_BIT_MCU_MC_ON
        | (1 if park_brake else 0) << PDC_BIT_PARK_BRAKE
        | (1 if brake_led else 0) << PDC_BIT_BRAKE_LED
    )


def bit_set(value: int, bit: int) -> bool:
    return bool(value & (1 << bit))


def cell_voltages_fault(hi_v: float, lo_v: float) -> bool:
    return hi_v > CELL_V_MAX or hi_v < CELL_V_MIN or lo_v > CELL_V_MAX or lo_v < CELL_V_MIN


def bps_electrical_payload(*, hi_raw: int, lo_raw: int, current_raw: int | None = None) -> bytes:
    if current_raw is None:
        current_raw = CAN["SC2_CAN_BPS_PACK_CURRENT_ZERO"]
    return pack_be_u16(hi_raw) + pack_be_u16(lo_raw) + pack_be_u16(current_raw)


def mppt_string_voltage_id(index: int) -> int:
    stride = CAN.get("SC2_CAN_MPPT_STRING_STRIDE", 5)
    return CAN["SC2_CAN_MPPT_STRING0_V_ID"] + stride * index


def decode_cell_voltage_v(raw_be: bytes) -> float:
    return int.from_bytes(raw_be[:2], "big") * 0.0001


def decode_pack_current_a(raw_be: bytes) -> float:
    raw = int.from_bytes(raw_be[:2], "big")
    zero = CAN["SC2_CAN_BPS_PACK_CURRENT_ZERO"]
    return abs(raw - zero) * 0.1
