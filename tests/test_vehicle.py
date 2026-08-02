"""L3: multi-board scenario on one virtual bus."""

from __future__ import annotations

import pytest

from load_can_ids import CAN
from conftest import (
    bit_set,
    decode_cell_voltage_v,
    pack_be_u16,
    pack_float,
    pack_u16,
    send,
    steering_digital,
    unpack_float,
)
from test_boards import drain


THROTTLE_SENT_MAX = 4095
CELL_V_MAX = 4.18


def test_drive_blink_fault_vehicle(can_pair):
    tx, rx = can_pair

    send(tx, CAN["SC2_CAN_STEERING_THROTTLE_ID"], pack_u16(THROTTLE_SENT_MAX // 2))
    send(tx, CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"], bytes([CAN["SC2_CAN_DRIVE_MODE_ECO"]]))
    send(
        tx,
        CAN["SC2_CAN_STEERING_DIGITAL_ID"],
        bytes([steering_digital(headlight=1, left=1)]),
    )
    send(tx, CAN["SC2_CAN_PDC_MPH_ID"], pack_float(42.0))
    send(
        tx,
        CAN["SC2_CAN_BPS_ELECTRICAL_ID"],
        pack_be_u16(45000)
        + pack_be_u16(33000)
        + pack_be_u16(CAN["SC2_CAN_BPS_PACK_CURRENT_ZERO"]),
    )

    world = {
        "acc_in": 0.0,
        "eco": None,
        "left_blink": False,
        "headlight": False,
        "mph": None,
        "fault": False,
        "bps_light": False,
    }

    for msg in drain(rx, count=8):
        data = bytes(msg.data)
        arb = msg.arbitration_id
        if arb == CAN["SC2_CAN_STEERING_THROTTLE_ID"] and len(data) >= 2:
            world["acc_in"] = min(int.from_bytes(data[:2], "little") / THROTTLE_SENT_MAX, 1.0)
        elif arb == CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"] and data:
            world["eco"] = data[0] == CAN["SC2_CAN_DRIVE_MODE_ECO"]
        elif arb == CAN["SC2_CAN_STEERING_DIGITAL_ID"] and data:
            world["headlight"] = bit_set(data[0], 0)
            world["left_blink"] = bit_set(data[0], 1)
        elif arb == CAN["SC2_CAN_PDC_MPH_ID"] and len(data) >= 4:
            world["mph"] = unpack_float(data)
        elif arb == CAN["SC2_CAN_BPS_ELECTRICAL_ID"] and len(data) >= 6:
            if decode_cell_voltage_v(data[0:2]) > CELL_V_MAX:
                world["fault"] = True

    if world["fault"]:
        send(tx, CAN["SC2_CAN_PT_FAULT_STATUS_ID"], bytes([CAN["SC2_CAN_PT_FAULT_MASK"]]))
        status = rx.recv(timeout=0.2)
        assert status is not None
        world["bps_light"] = bool(status.data[0] & CAN["SC2_CAN_PT_FAULT_MASK"])

    assert world["acc_in"] == pytest.approx(0.5, abs=0.02)
    assert world["eco"] is True
    assert world["headlight"] and world["left_blink"]
    assert world["mph"] == pytest.approx(42.0)
    assert world["fault"] is True
    assert world["bps_light"] is True
