#ifndef CAN_IDS_H
#define CAN_IDS_H

#include <stdint.h>

/*
 * Canonical SC2 classic-CAN identifiers (11-bit). Baud: 250000.
 * Single source of truth for firmware + Python tests (tests/load_can_ids.py).
 */

/* External / BMS / commands */
#define CAN_MPPT_CAP_DISCHARGE   0x050
#define CAN_MPPT_OV_RESET        0x051
#define CAN_BMS_PACK             0x101
#define CAN_BPS_LIGHT            0x103
#define CAN_BMS_CHARGE_LIMIT     0x103 /* same arbitration ID; BMS charge limit RX */
#define CAN_BPS_TEMPERATURE      0x108
#define CAN_BPS_ELECTRICAL       0x109

/* PDC telemetry TX */
#define CAN_PDC_ACC_OUT          0x200
#define CAN_PDC_REGEN            0x201
#define CAN_PDC_LV_12V           0x202
#define CAN_PDC_LV_5V            0x203
#define CAN_PDC_LV_5V_I          0x204
#define CAN_PDC_CURRENT_IN       0x205
#define CAN_PDC_BRAKE_PRESSURE   0x206
#define CAN_PDC_DIGITAL          0x207
#define CAN_PDC_MPH              0x208
#define CAN_TEST_PEDAL           0x209

/* Steering command TX / PDC + lighting RX */
#define CAN_STEERING_DIGITAL     0x300
#define CAN_STEERING_REGEN       0x301
#define CAN_STEERING_THROTTLE    0x302
#define CAN_STEERING_DRIVE_MODE  0x303
#define CAN_STEERING_HAZARD      0x304

/* MPPT TX: boost/mode then per-string blocks of 5 IDs starting at 0x402 */
#define CAN_MPPT_BOOST           0x400
#define CAN_MPPT_MODE            0x401
#define CAN_MPPT_STRING0_V       0x402
#define CAN_MPPT_STRING_STRIDE   5
#define CAN_MPPT_FIELD_V         0
#define CAN_MPPT_FIELD_I         1
#define CAN_MPPT_FIELD_TEMP      2
#define CAN_MPPT_FIELD_DUTY      3
#define CAN_MPPT_FIELD_TARGET    4

/* Powertrain TX */
#define CAN_PT_I_12V             0x500
#define CAN_PT_V_12V             0x501
#define CAN_PT_SUPP_I            0x502
#define CAN_PT_BATT_I            0x503
#define CAN_PT_SUPP_V            0x504
#define CAN_PT_FAULT_STATUS      0x505

#define CAN_LIGHTING_HEARTBEAT   0x700
#define CAN_FAULT_CLEAR          0x7EB

#define CAN_BPS_TEMPERATURE_DLC  4
#define CAN_BPS_ELECTRICAL_DLC   6
#define CAN_PT_FAULT_STATUS_DLC  1
#define CAN_FAULT_CLEAR_DLC      8

#define CAN_DRIVE_MODE_ECO       0
#define CAN_DRIVE_MODE_PWR       1
#define CAN_PT_FAULT_MASK        0x01

#define CAN_BPS_PACK_CURRENT_ZERO 0x8000

/* Per-string MPPT telem ID: base 0x402 + stride*i + field */
#define CAN_MPPT_STRING(string_index, field) \
    (CAN_MPPT_STRING0_V + (CAN_MPPT_STRING_STRIDE * (string_index)) + (field))

#endif
