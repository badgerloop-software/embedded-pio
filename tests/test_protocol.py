"""L1: shared CAN ID / packing contracts from can_ids.h."""

import pytest

from load_can_ids import CAN
from conftest import (
    decode_cell_voltage_v,
    decode_pack_current_a,
    mppt_string_voltage_id,
    pack_be_u16,
    pack_float,
    steering_digital,
    unpack_float,
)


def test_core_ids():
    assert CAN["CAN_STEERING_DIGITAL"] == 0x300
    assert CAN["CAN_STEERING_DRIVE_MODE"] == 0x303
    assert CAN["CAN_PT_FAULT_STATUS"] == 0x505
    assert CAN["CAN_FAULT_CLEAR"] == 0x7EB


def test_float_roundtrip():
    assert unpack_float(pack_float(12.5)) == pytest.approx(12.5)


def test_cell_voltage_decode():
    assert decode_cell_voltage_v(pack_be_u16(34409)) == pytest.approx(3.4409)


def test_pack_current_midscale_is_zero():
    assert decode_pack_current_a(
        pack_be_u16(CAN["CAN_BPS_PACK_CURRENT_ZERO"])
    ) == pytest.approx(0.0)


def test_steering_digital_bits():
    value = steering_digital(headlight=1, left=1, right=0)
    assert value & 0x01
    assert value & 0x02
    assert not (value & 0x04)


def test_mppt_string_offsets_do_not_collide():
    ids = [mppt_string_voltage_id(i) for i in range(3)]
    assert len(ids) == len(set(ids))
    stride = CAN["CAN_MPPT_STRING_STRIDE"]
    assert ids[1] - ids[0] == stride
    assert CAN["CAN_MPPT_FIELD_TARGET"] == 4
