"""Car smoke contracts: board-to-board talk that must not silently break.

These do not run firmware. They freeze IDs, DLCs, bit layouts, and the
steering → PDC / lighting / powertrain ↔ BPS shapes used on the car.
If this file fails, do not flash boards for dyno or track testing.
"""

from __future__ import annotations

import pytest

from load_can_ids import CAN
from conftest import (
    CELL_V_MAX,
    CELL_V_MIN,
    FAULT_CLEAR_PAYLOAD,
    PDC_BIT_BRAKE_LED,
    PDC_BIT_DIRECTION,
    STEERING_BIT_HEADLIGHT,
    STEERING_BIT_LEFT,
    STEERING_BIT_RIGHT,
    THROTTLE_SENT_MAX,
    bit_set,
    bps_electrical_payload,
    cell_voltages_fault,
    decode_cell_voltage_v,
    mppt_string_voltage_id,
    pack_float,
    pack_u16,
    pdc_digital,
    send,
    steering_digital,
    unpack_float,
    unpack_u16,
)


def _drain(rx, count=12, timeout=0.1):
    messages = []
    for _ in range(count):
        msg = rx.recv(timeout=timeout)
        if msg is None:
            break
        messages.append(msg)
        timeout = 0.02
    return messages


# ------------- ID / DLC TABLE -------------


def test_critical_ids_and_dlcs_frozen():
    """IDs and DLCs every board depends on for drive / lights / BPS."""
    assert CAN["SC2_CAN_STEERING_DIGITAL_ID"] == 0x300
    assert CAN["SC2_CAN_STEERING_REGEN_ID"] == 0x301
    assert CAN["SC2_CAN_STEERING_THROTTLE_ID"] == 0x302
    assert CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"] == 0x303
    assert CAN["SC2_CAN_STEERING_HAZARD_ID"] == 0x304

    assert CAN["SC2_CAN_PDC_DIGITAL_ID"] == 0x207
    assert CAN["SC2_CAN_PDC_MPH_ID"] == 0x208
    assert CAN["SC2_CAN_PDC_ACC_OUT_ID"] == 0x200

    assert CAN["SC2_CAN_BPS_TEMPERATURE_ID"] == 0x108
    assert CAN["SC2_CAN_BPS_ELECTRICAL_ID"] == 0x109
    assert CAN["SC2_CAN_PT_FAULT_STATUS_ID"] == 0x505
    assert CAN["SC2_CAN_FAULT_CLEAR_ID"] == 0x7EB
    assert CAN["SC2_CAN_LIGHTING_HEARTBEAT_ID"] == 0x700

    assert CAN["SC2_CAN_BPS_TEMPERATURE_DLC"] == 4
    assert CAN["SC2_CAN_BPS_ELECTRICAL_DLC"] == 6
    assert CAN["SC2_CAN_PT_FAULT_STATUS_DLC"] == 1
    assert CAN["SC2_CAN_FAULT_CLEAR_DLC"] == 8

    assert CAN["SC2_CAN_DRIVE_MODE_ECO"] == 0
    assert CAN["SC2_CAN_DRIVE_MODE_PWR"] == 1
    assert CAN["SC2_CAN_PT_FAULT_MASK"] == 0x01


def test_command_families_do_not_overlap():
    """Steering / PDC / MPPT / powertrain / lighting ID bands stay distinct."""
    steering = {0x300, 0x301, 0x302, 0x303, 0x304}
    pdc = {0x200, 0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207, 0x208}
    mppt_cmd = {0x050, 0x051, 0x101, 0x103}
    pt = {0x500, 0x501, 0x502, 0x503, 0x504, 0x505}
    lighting = {0x700}
    bps = {0x108, 0x109}
    clear = {0x7EB}

    bands = [steering, pdc, mppt_cmd, pt, lighting, bps, clear]
    for i, a in enumerate(bands):
        for b in bands[i + 1 :]:
            assert a.isdisjoint(b)


# ------------- STEERING → PDC (motor commands) -------------


def test_steering_to_pdc_throttle_and_drive_mode(can_pair):
    tx, rx = can_pair
    send(tx, CAN["SC2_CAN_STEERING_THROTTLE_ID"], pack_u16(THROTTLE_SENT_MAX // 2))
    send(tx, CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"], bytes([CAN["SC2_CAN_DRIVE_MODE_ECO"]]))
    send(tx, CAN["SC2_CAN_STEERING_REGEN_ID"], pack_float(0.25))

    acc = None
    eco = None
    regen = None
    for msg in _drain(rx, count=6):
        data = bytes(msg.data)
        if msg.arbitration_id == CAN["SC2_CAN_STEERING_THROTTLE_ID"] and len(data) >= 2:
            acc = min(unpack_u16(data) / THROTTLE_SENT_MAX, 1.0)
        elif msg.arbitration_id == CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"] and data:
            eco = data[0] == CAN["SC2_CAN_DRIVE_MODE_ECO"]
        elif msg.arbitration_id == CAN["SC2_CAN_STEERING_REGEN_ID"] and len(data) >= 4:
            regen = unpack_float(data)

    assert acc == pytest.approx(0.5, abs=0.02)
    assert eco is True
    assert regen == pytest.approx(0.25)

    send(tx, CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"], bytes([CAN["SC2_CAN_DRIVE_MODE_PWR"]]))
    msg = rx.recv(timeout=0.2)
    assert msg is not None
    assert msg.data[0] == CAN["SC2_CAN_DRIVE_MODE_PWR"]


# ------------- STEERING / PDC → LIGHTING -------------


def test_steering_digital_bits_for_lights(can_pair):
    tx, rx = can_pair
    send(
        tx,
        CAN["SC2_CAN_STEERING_DIGITAL_ID"],
        bytes([steering_digital(headlight=1, left=1, right=0)]),
    )
    msg = rx.recv(timeout=0.2)
    assert msg is not None
    data = msg.data[0]
    assert bit_set(data, STEERING_BIT_HEADLIGHT)
    assert bit_set(data, STEERING_BIT_LEFT)
    assert not bit_set(data, STEERING_BIT_RIGHT)


def test_hazards_frame_and_both_blink_bits(can_pair):
    """Hazards: dedicated 0x304 plus both blink bits phased on 0x300."""
    tx, rx = can_pair
    send(tx, CAN["SC2_CAN_STEERING_HAZARD_ID"], bytes([1]))
    send(
        tx,
        CAN["SC2_CAN_STEERING_DIGITAL_ID"],
        bytes([steering_digital(left=1, right=1)]),
    )

    hazard = None
    both_blink = False
    for msg in _drain(rx, count=4):
        data = bytes(msg.data)
        if msg.arbitration_id == CAN["SC2_CAN_STEERING_HAZARD_ID"] and data:
            hazard = bool(data[0])
        elif msg.arbitration_id == CAN["SC2_CAN_STEERING_DIGITAL_ID"] and data:
            both_blink = bit_set(data[0], STEERING_BIT_LEFT) and bit_set(
                data[0], STEERING_BIT_RIGHT
            )

    assert hazard is True
    assert both_blink is True


def test_pdc_brake_and_reverse_bits_for_lights(can_pair):
    """Lighting REVERSE uses PDC bit 0; BRAKELIGHT uses PDC bit 5."""
    tx, rx = can_pair
    send(
        tx,
        CAN["SC2_CAN_PDC_DIGITAL_ID"],
        bytes([pdc_digital(direction=1, brake_led=1)]),
    )
    msg = rx.recv(timeout=0.2)
    assert msg is not None
    data = msg.data[0]
    assert bit_set(data, PDC_BIT_DIRECTION)
    assert bit_set(data, PDC_BIT_BRAKE_LED)


# ------------- BPS ↔ POWERTRAIN ↔ LIGHTING -------------


def test_bps_healthy_cells_are_not_a_fault():
    hi = decode_cell_voltage_v((34409).to_bytes(2, "big"))  # ~3.44 V
    lo = decode_cell_voltage_v((33000).to_bytes(2, "big"))  # ~3.30 V
    assert CELL_V_MIN <= lo <= hi <= CELL_V_MAX
    assert cell_voltages_fault(hi, lo) is False


def test_bps_overvolt_is_a_fault_and_status_shape(can_pair):
    tx, rx = can_pair
    payload = bps_electrical_payload(hi_raw=45000, lo_raw=33000)  # 4.5 V / 3.3 V
    assert len(payload) == CAN["SC2_CAN_BPS_ELECTRICAL_DLC"]
    send(tx, CAN["SC2_CAN_BPS_ELECTRICAL_ID"], payload)

    msg = rx.recv(timeout=0.2)
    assert msg is not None
    data = bytes(msg.data)
    hi = decode_cell_voltage_v(data[0:2])
    lo = decode_cell_voltage_v(data[2:4])
    assert cell_voltages_fault(hi, lo) is True

    send(tx, CAN["SC2_CAN_PT_FAULT_STATUS_ID"], bytes([CAN["SC2_CAN_PT_FAULT_MASK"]]))
    status = rx.recv(timeout=0.2)
    assert status is not None
    assert status.dlc == CAN["SC2_CAN_PT_FAULT_STATUS_DLC"]
    assert status.data[0] & CAN["SC2_CAN_PT_FAULT_MASK"]


def test_fault_clear_command_shape(can_pair):
    tx, rx = can_pair
    assert len(FAULT_CLEAR_PAYLOAD) == CAN["SC2_CAN_FAULT_CLEAR_DLC"]
    send(tx, CAN["SC2_CAN_FAULT_CLEAR_ID"], FAULT_CLEAR_PAYLOAD)

    msg = rx.recv(timeout=0.2)
    assert msg is not None
    assert msg.arbitration_id == CAN["SC2_CAN_FAULT_CLEAR_ID"]
    assert bytes(msg.data) == FAULT_CLEAR_PAYLOAD

    # Wrong length or bytes must not look like a valid clear.
    send(tx, CAN["SC2_CAN_FAULT_CLEAR_ID"], bytes([0xFF]) * 8)
    bad = rx.recv(timeout=0.2)
    assert bad is not None
    assert bytes(bad.data) != FAULT_CLEAR_PAYLOAD


# ------------- MPPT / TELEMETRY BANDS -------------


def test_mppt_string_block_stays_below_powertrain():
    """3-string board telem must not collide with powertrain 0x500+."""
    num_arrays = 3
    last_id = (
        mppt_string_voltage_id(num_arrays - 1) + CAN["SC2_CAN_MPPT_FIELD_TARGET"]
    )
    assert last_id < CAN["SC2_CAN_PT_I_12V_ID"]
    ids = {
        mppt_string_voltage_id(i) + field
        for i in range(num_arrays)
        for field in range(CAN["SC2_CAN_MPPT_STRING_STRIDE"])
    }
    assert len(ids) == num_arrays * CAN["SC2_CAN_MPPT_STRING_STRIDE"]


def test_pdc_speed_telem_roundtrip(can_pair):
    tx, rx = can_pair
    send(tx, CAN["SC2_CAN_PDC_MPH_ID"], pack_float(42.0))
    msg = rx.recv(timeout=0.2)
    assert msg is not None
    assert unpack_float(bytes(msg.data)) == pytest.approx(42.0)


# ------------- WHOLE-CAR SMOKE -------------


def test_drive_lights_bps_smoke(can_pair):
    """One pass: throttle + eco + blink/headlight + brake/reverse + BPS fault light."""
    tx, rx = can_pair

    send(tx, CAN["SC2_CAN_STEERING_THROTTLE_ID"], pack_u16(THROTTLE_SENT_MAX // 2))
    send(tx, CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"], bytes([CAN["SC2_CAN_DRIVE_MODE_ECO"]]))
    send(
        tx,
        CAN["SC2_CAN_STEERING_DIGITAL_ID"],
        bytes([steering_digital(headlight=1, left=1)]),
    )
    send(tx, CAN["SC2_CAN_PDC_DIGITAL_ID"], bytes([pdc_digital(direction=1, brake_led=1)]))
    send(tx, CAN["SC2_CAN_BPS_ELECTRICAL_ID"], bps_electrical_payload(hi_raw=45000, lo_raw=33000))

    world = {
        "acc": None,
        "eco": None,
        "headlight": False,
        "left": False,
        "brake": False,
        "reverse": False,
        "fault": False,
    }

    for msg in _drain(rx, count=10):
        data = bytes(msg.data)
        arb = msg.arbitration_id
        if arb == CAN["SC2_CAN_STEERING_THROTTLE_ID"] and len(data) >= 2:
            world["acc"] = min(unpack_u16(data) / THROTTLE_SENT_MAX, 1.0)
        elif arb == CAN["SC2_CAN_STEERING_DRIVE_MODE_ID"] and data:
            world["eco"] = data[0] == CAN["SC2_CAN_DRIVE_MODE_ECO"]
        elif arb == CAN["SC2_CAN_STEERING_DIGITAL_ID"] and data:
            world["headlight"] = bit_set(data[0], STEERING_BIT_HEADLIGHT)
            world["left"] = bit_set(data[0], STEERING_BIT_LEFT)
        elif arb == CAN["SC2_CAN_PDC_DIGITAL_ID"] and data:
            world["reverse"] = bit_set(data[0], PDC_BIT_DIRECTION)
            world["brake"] = bit_set(data[0], PDC_BIT_BRAKE_LED)
        elif arb == CAN["SC2_CAN_BPS_ELECTRICAL_ID"] and len(data) >= 4:
            world["fault"] = cell_voltages_fault(
                decode_cell_voltage_v(data[0:2]),
                decode_cell_voltage_v(data[2:4]),
            )

    if world["fault"]:
        send(tx, CAN["SC2_CAN_PT_FAULT_STATUS_ID"], bytes([CAN["SC2_CAN_PT_FAULT_MASK"]]))
        status = rx.recv(timeout=0.2)
        assert status is not None
        assert status.data[0] & CAN["SC2_CAN_PT_FAULT_MASK"]

    assert world["acc"] == pytest.approx(0.5, abs=0.02)
    assert world["eco"] is True
    assert world["headlight"] and world["left"]
    assert world["brake"] and world["reverse"]
    assert world["fault"] is True
