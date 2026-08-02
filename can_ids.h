#ifndef SC2_CAN_IDS_H
#define SC2_CAN_IDS_H

#include <stdint.h>

/*
 * Canonical SC2 classic-CAN identifiers (11-bit). Baud: 250000.
 * Single source of truth for firmware + Python tests (tests/load_can_ids.py).
 */

/* External / BMS / commands */
#define SC2_CAN_MPPT_CAP_DISCHARGE_ID   0x050u
#define SC2_CAN_MPPT_OV_RESET_ID        0x051u
#define SC2_CAN_BMS_PACK_ID             0x101u
#define SC2_CAN_BPS_LIGHT_ID            0x103u
#define SC2_CAN_BPS_TEMPERATURE_ID      0x108u
#define SC2_CAN_BPS_ELECTRICAL_ID       0x109u

/* PDC telemetry TX */
#define SC2_CAN_PDC_ACC_OUT_ID          0x200u
#define SC2_CAN_PDC_REGEN_ID            0x201u
#define SC2_CAN_PDC_LV_12V_ID           0x202u
#define SC2_CAN_PDC_LV_5V_ID            0x203u
#define SC2_CAN_PDC_LV_5V_I_ID          0x204u
#define SC2_CAN_PDC_CURRENT_IN_ID       0x205u
#define SC2_CAN_PDC_BRAKE_PRESSURE_ID   0x206u
#define SC2_CAN_PDC_DIGITAL_ID          0x207u
#define SC2_CAN_PDC_MPH_ID              0x208u
#define SC2_CAN_TEST_PEDAL_ID           0x209u

/* Steering command TX / PDC + lighting RX */
#define SC2_CAN_STEERING_DIGITAL_ID     0x300u
#define SC2_CAN_STEERING_REGEN_ID       0x301u
#define SC2_CAN_STEERING_THROTTLE_ID    0x302u
#define SC2_CAN_STEERING_DRIVE_MODE_ID  0x303u
#define SC2_CAN_STEERING_HAZARD_ID      0x304u

/* MPPT base IDs (string telem: 0x402 + 5*i) */
#define SC2_CAN_MPPT_BOOST_ID           0x400u
#define SC2_CAN_MPPT_MODE_ID            0x401u
#define SC2_CAN_MPPT_STRING0_V_ID       0x402u

/* Powertrain TX */
#define SC2_CAN_PT_I_12V_ID             0x500u
#define SC2_CAN_PT_V_12V_ID             0x501u
#define SC2_CAN_PT_SUPP_I_ID            0x502u
#define SC2_CAN_PT_BATT_I_ID            0x503u
#define SC2_CAN_PT_SUPP_V_ID            0x504u
#define SC2_CAN_PT_FAULT_STATUS_ID      0x505u

#define SC2_CAN_LIGHTING_HEARTBEAT_ID   0x700u
#define SC2_CAN_FAULT_CLEAR_ID          0x7EBu

#define SC2_CAN_BPS_TEMPERATURE_DLC     4u
#define SC2_CAN_BPS_ELECTRICAL_DLC      6u
#define SC2_CAN_PT_FAULT_STATUS_DLC     1u
#define SC2_CAN_FAULT_CLEAR_DLC         8u

#define SC2_CAN_DRIVE_MODE_ECO          0u
#define SC2_CAN_DRIVE_MODE_PWR          1u
#define SC2_CAN_PT_FAULT_MASK           0x01u

#define SC2_CAN_BPS_PACK_CURRENT_ZERO   0x8000u

#endif
