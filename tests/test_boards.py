"""L2: per-board behavior contracts on a virtual CAN bus.

Small, readable models of bus behavior. Prefer blocking recv over Notifier
threads so tests stay deterministic for new engineers.

Run from this directory:
    pip install -r requirements.txt
    pytest
"""

from __future__ import annotations

import pytest

from load_can_ids import CAN
from conftest import (
    CELL_V_MAX,
    CELL_V_MIN,
    THROTTLE_SENT_MAX,
    bit_set,
    decode_cell_voltage_v,
    mppt_string_voltage_id,
    pack_be_u16,
    pack_float,
    pack_u16,
    send,
    steering_digital,
    unpack_float,
    unpack_u16,
)


def drain(rx, count=8, timeout=0.1):
    """Receive up to count messages."""
    messages = []
    for _ in range(count):
        msg = rx.recv(timeout=timeout)
        if msg is None:
            break
        messages.append(msg)
        timeout = 0.02
    return messages


def test_pdc_throttle_and_drive_mode(can_pair):
    tx, rx = can_pair
    send(tx, CAN["SC2_CAN_STEERING_THROTTLE_ID"], pack_u16(THROTTLE_SENT_MAX // 2))
    send(tx, CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"], bytes([CAN["SC2_CAN_DRIVE_MODE_ECO"]]))

    acc_in = None
    eco = None
    for msg in drain(rx, count=4):
        if msg.arbitration_id == CAN["SC2_CAN_STEERING_THROTTLE_ID"]:
            acc_in = min(unpack_u16(bytes(msg.data)) / THROTTLE_SENT_MAX, 1.0)
        elif msg.arbitration_id == CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"]:
            eco = msg.data[0] == CAN["SC2_CAN_DRIVE_MODE_ECO"]

    assert acc_in == pytest.approx(0.5, abs=0.02)
    assert eco is True

    send(tx, CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"], bytes([CAN["SC2_CAN_DRIVE_MODE_PWR"]]))
    msg = rx.recv(timeout=0.2)
    assert msg is not None
    assert msg.data[0] == CAN["SC2_CAN_DRIVE_MODE_PWR"]


def test_powertrain_overvoltage_fault(can_pair):
    tx, rx = can_pair
    payload = (
        pack_be_u16(45000)
        + pack_be_u16(33000)
        + pack_be_u16(CAN["SC2_CAN_BPS_PACK_CURRENT_ZERO"])
    )
    send(tx, CAN["SC2_CAN_BPS_ELECTRICAL_ID"], payload)

    msg = rx.recv(timeout=0.2)
    assert msg is not None
    assert msg.arbitration_id == CAN["SC2_CAN_BPS_ELECTRICAL_ID"]
    data = bytes(msg.data)
    hi = decode_cell_voltage_v(data[0:2])
    lo = decode_cell_voltage_v(data[2:4])
    fault = hi > CELL_V_MAX or hi < CELL_V_MIN or lo > CELL_V_MAX or lo < CELL_V_MIN
    assert fault is True

    send(tx, CAN["SC2_CAN_PT_FAULT_STATUS_ID"], bytes([CAN["SC2_CAN_PT_FAULT_MASK"]]))
    status = rx.recv(timeout=0.2)
    assert status is not None
    assert status.data[0] & CAN["SC2_CAN_PT_FAULT_MASK"]


def test_steering_tx_shape(can_pair):
    tx, rx = can_pair
    send(tx, CAN["SC2_CAN_STEERING_DIGITAL_ID"], bytes([steering_digital(headlight=1, left=1)]))
    send(tx, CAN["SC2_CAN_STEERING_REGEN_ID"], pack_float(0.35))
    send(tx, CAN["SC2_CAN_STEERING_THROTTLE_ID"], pack_u16(2048))
    send(tx, CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"], bytes([CAN["SC2_CAN_DRIVE_MODE_PWR"]]))
    send(tx, CAN["SC2_CAN_STEERING_HAZARD_ID"], bytes([1]))

    got = {msg.arbitration_id for msg in drain(rx, count=5)}
    assert CAN["SC2_CAN_STEERING_DIGITAL_ID"] in got
    assert CAN["SC2_CAN_STEERING_THROTTLE_ID"] in got
    assert CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"] in got


def test_lighting_left_front_and_bps_side(can_pair):
    tx, rx = can_pair
    send(tx, CAN["SC2_CAN_STEERING_DIGITAL_ID"], bytes([steering_digital(headlight=1, left=1)]))
    send(tx, CAN["SC2_CAN_PT_FAULT_STATUS_ID"], bytes([CAN["SC2_CAN_PT_FAULT_MASK"]]))

    left_front = (0, 0)
    left_side_bps = 0
    for msg in drain(rx, count=4):
        data = bytes(msg.data)
        if msg.arbitration_id == CAN["SC2_CAN_STEERING_DIGITAL_ID"] and data:
            left_front = (
                1 if bit_set(data[0], 1) else 0,
                1 if bit_set(data[0], 0) else 0,
            )
        elif msg.arbitration_id == CAN["SC2_CAN_PT_FAULT_STATUS_ID"] and data:
            left_side_bps = 1 if (data[0] & CAN["SC2_CAN_PT_FAULT_MASK"]) else 0

    assert left_front == (1, 1)
    assert left_side_bps == 1


def test_mppt_command_and_string_telem(can_pair):
    tx, rx = can_pair
    send(tx, CAN["SC2_CAN_MPPT_CAP_DISCHARGE_ID"], bytes([1]))
    send(tx, CAN["SC2_CAN_BMS_PACK_ID"], pack_float(12.0))
    send(tx, mppt_string_voltage_id(0), pack_float(48.5))
    send(tx, mppt_string_voltage_id(1), pack_float(47.0))

    discharge = False
    limit = None
    for msg in drain(rx, count=6):
        if msg.arbitration_id == CAN["SC2_CAN_MPPT_CAP_DISCHARGE_ID"]:
            discharge = True
        elif msg.arbitration_id == CAN["SC2_CAN_BMS_PACK_ID"] and len(msg.data) >= 4:
            limit = unpack_float(bytes(msg.data))

    assert discharge is True
    assert limit == pytest.approx(12.0)
    assert mppt_string_voltage_id(0) != mppt_string_voltage_id(1)
