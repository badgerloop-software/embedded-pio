#ifndef SC2_CAN_IDS_H
#define SC2_CAN_IDS_H

#include <stdint.h>

/*
 * Canonical SC2 classic-CAN identifiers (11-bit). Baud: 250000.
 * Single source of truth for firmware + Python tests (tests/load_can_ids.py).
 */

/* External / BMS / commands */
#define SC2_CAN_MPPT_CAP_DISCHARGE_ID   0x050
#define SC2_CAN_MPPT_OV_RESET_ID        0x051
#define SC2_CAN_BMS_PACK_ID             0x101
#define SC2_CAN_BPS_LIGHT_ID            0x103
#define SC2_CAN_BMS_CHARGE_LIMIT_ID     0x103 /* same arbitration ID; BMS charge limit RX */
#define SC2_CAN_BPS_TEMPERATURE_ID      0x108
#define SC2_CAN_BPS_ELECTRICAL_ID       0x109

/* PDC telemetry TX */
#define SC2_CAN_PDC_ACC_OUT_ID          0x200
#define SC2_CAN_PDC_REGEN_ID            0x201
#define SC2_CAN_PDC_LV_12V_ID           0x202
#define SC2_CAN_PDC_LV_5V_ID            0x203
#define SC2_CAN_PDC_LV_5V_I_ID          0x204
#define SC2_CAN_PDC_CURRENT_IN_ID       0x205
#define SC2_CAN_PDC_BRAKE_PRESSURE_ID   0x206
#define SC2_CAN_PDC_DIGITAL_ID          0x207
#define SC2_CAN_PDC_MPH_ID              0x208
#define SC2_CAN_TEST_PEDAL_ID           0x209

/* Steering command TX / PDC + lighting RX */
#define SC2_CAN_STEERING_DIGITAL_ID     0x300
#define SC2_CAN_STEERING_REGEN_ID       0x301
#define SC2_CAN_STEERING_THROTTLE_ID    0x302
#define SC2_CAN_STEERING_DRIVE_MODE_ID  0x303
#define SC2_CAN_STEERING_HAZARD_ID      0x304

/* MPPT TX: boost/mode then per-string blocks of 5 IDs starting at 0x402 */
#define SC2_CAN_MPPT_BOOST_ID           0x400
#define SC2_CAN_MPPT_MODE_ID            0x401
#define SC2_CAN_MPPT_STRING0_V_ID       0x402
#define SC2_CAN_MPPT_STRING_STRIDE      5
#define SC2_CAN_MPPT_FIELD_V            0
#define SC2_CAN_MPPT_FIELD_I            1
#define SC2_CAN_MPPT_FIELD_TEMP         2
#define SC2_CAN_MPPT_FIELD_DUTY         3
#define SC2_CAN_MPPT_FIELD_TARGET       4

/* Powertrain TX */
#define SC2_CAN_PT_I_12V_ID             0x500
#define SC2_CAN_PT_V_12V_ID             0x501
#define SC2_CAN_PT_SUPP_I_ID            0x502
#define SC2_CAN_PT_BATT_I_ID            0x503
#define SC2_CAN_PT_SUPP_V_ID            0x504
#define SC2_CAN_PT_FAULT_STATUS_ID      0x505

#define SC2_CAN_LIGHTING_HEARTBEAT_ID   0x700
#define SC2_CAN_FAULT_CLEAR_ID          0x7EB

#define SC2_CAN_BPS_TEMPERATURE_DLC     4
#define SC2_CAN_BPS_ELECTRICAL_DLC      6
#define SC2_CAN_PT_FAULT_STATUS_DLC     1
#define SC2_CAN_FAULT_CLEAR_DLC         8

#define SC2_CAN_DRIVE_MODE_ECO          0
#define SC2_CAN_DRIVE_MODE_PWR          1
#define SC2_CAN_PT_FAULT_MASK           0x01

#define SC2_CAN_BPS_PACK_CURRENT_ZERO   0x8000

/* Per-string MPPT telem ID: base 0x402 + stride*i + field */
#define SC2_CAN_MPPT_STRING_ID(string_index, field) \
    (SC2_CAN_MPPT_STRING0_V_ID + (SC2_CAN_MPPT_STRING_STRIDE * (string_index)) + (field))

#endif
