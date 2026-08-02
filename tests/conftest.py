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


def steering_digital(*, headlight=0, left=0, right=0, direction=0, horn=0) -> int:
    return (
        (1 if headlight else 0) << 0
        | (1 if left else 0) << 1
        | (1 if right else 0) << 2
        | (1 if direction else 0) << 3
        | (1 if horn else 0) << 4
    )


def bit_set(value: int, bit: int) -> bool:
    return bool(value & (1 << bit))


def mppt_string_voltage_id(index: int) -> int:
    return CAN["SC2_CAN_MPPT_STRING0_V_ID"] + 5 * index


def decode_cell_voltage_v(raw_be: bytes) -> float:
    return int.from_bytes(raw_be[:2], "big") * 0.0001


def decode_pack_current_a(raw_be: bytes) -> float:
    raw = int.from_bytes(raw_be[:2], "big")
    zero = CAN["SC2_CAN_BPS_PACK_CURRENT_ZERO"]
    return abs(raw - zero) * 0.1
