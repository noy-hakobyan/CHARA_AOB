#pragma once
#include <Arduino.h>

/**
 * @file dm_556_rs_constants.h
 * @brief Register addresses and enumerations for Leadshine‑style DM556RS stepper drivers over Modbus RTU.
 *
 * Context
 *  - Host: ClearCore (Arduino‑style runtime, C++17 OK). Serial1 is the RS‑485 port at 8N1.
 *  - Wire protocol: Modbus RTU. Addresses and data are big‑endian in the PDU; CRC is standard Modbus CRC‑16 (poly 0xA001).
 *  - This header supplies only addresses and value enums; build/send logic lives elsewhere.
 *
 * Conventions
 *  - All register addresses are 16‑bit.
 *  - Unless noted, writes take effect immediately but may also be persisted only after an explicit “save” (see REG_CONTROL_WORD/CW_SAVE_ALL_PARAMS).
 *  - “DI”/“DO” map physical I/O pins on the driver to functions. You assign a function by writing a DI/DO_VALUE into the corresponding REG_DIx/REG_DOx.
 *  - PR0 block holds one programmable motion profile used by this firmware for relative moves.
 *
 * Safe usage notes (for newcomers to these drivers)
 *  - Do not spam NVRAM: CW_SAVE_ALL_PARAMS performs a flash write; call sparingly.
 *  - Always respect inter‑frame silent intervals on RS‑485 (≥3.5 char times) and check for Modbus exceptions when reading back.
 *  - Position in PR0 is a 32‑bit *signed* step count split across POS_HIGH/POS_LOW.
 *  - Velocity is in RPM; Accel/Decel are in ms per 1000 RPM (lower number ⇒ faster accel).
 */

/* ------------------ Modbus Function Codes (RTU PDU: [id][fc][...]) ------------------ */
constexpr uint8_t FC_READ_HOLDING   = 0x03;  // Read Holding Registers (0x0000..)
constexpr uint8_t FC_WRITE_SINGLE   = 0x06;  // Write Single Register (Preset Single)
constexpr uint8_t FC_WRITE_MULTIPLE = 0x10;  // Write Multiple Registers (Preset Multiple)

/* ---------------- Core Control / Motor Basics (Chapter 4.3 in vendor docs) ---------------- */
/**
 * REG_FORCE_ENABLE
 *  - Software enable/disable independent of DI mapping.
 *  - Typical use: set once at init if you do not use a physical Enable input, or to force an axis off.
 *  - Values: 0x0000 = disable, 0x0001 = enable (exact semantics may vary; confirm in manual).
 */
constexpr uint16_t REG_FORCE_ENABLE        = 0x000F; // Pr0.07 — software enable

/**
 * REG_MICROSTEP
 *  - Microstep resolution selector for the drive. Value is an index, not “steps per revolution”.
 *  - The actual microstep table (e.g., 1600/3200/… steps/rev) is vendor‑defined; consult the table in the manual.
 *  - Changing this changes the steps↔mechanical‑unit scaling in your GUI.
 */
constexpr uint16_t REG_MICROSTEP           = 0x0001; // Pr0.01 — micro‑step resolution (model‑specific codes)

/**
 * REG_MOTOR_INDUCTANCE
 *  - Motor inductance parameter used by the current/anti‑resonance algorithm.
 *  - Units and valid range are vendor‑defined (often mH scaled); set per your motor datasheet for best performance.
 */
constexpr uint16_t REG_MOTOR_INDUCTANCE    = 0x0009; // Pr0.04 — motor inductance tuning

/**
 * REG_ALARM_STATUS
 *  - Read-only register containing the current alarm/error code.
 *  - Non-zero value indicates an active fault condition (overcurrent, overtemp, stall, etc.).
 *  - Consult manual for specific error code meanings.
 */
constexpr uint16_t REG_ALARM_STATUS        = 0x2203; // Pr9.51 — alarm/error status code

/**
 * REG_MOTION_STATUS
 *  - Motion status register for polling motor state during move execution.
 *  - 0x0006 = motor moving, 0x0032 = motor stopped, others = idle.
 */
constexpr uint16_t REG_MOTION_STATUS       = 0x1003; // Motion status register

/* ------------- Digital‑Input Function Mapping (write one DI_VAL_* to each) -------------- */
/**
 * REG_DIx_FUNC
 *  - Map each physical DI pin to a function (enable, alarm clear, trigger, limits, jog, etc.).
 *  - Write one of the DI_VAL_* constants below into the chosen DI register.
 *  - “Enable N.O.” vs “Enable N.C.” selects electrical sense without rewiring.
 */
constexpr uint16_t REG_DI1_FUNC            = 0x0145; // Pr4.02 — DI1 (Enable by default on many configs)
constexpr uint16_t REG_DI2_FUNC            = 0x0147; // Pr4.03 — DI2
constexpr uint16_t REG_DI3_FUNC            = 0x0149; // Pr4.04 — DI3
constexpr uint16_t REG_DI4_FUNC            = 0x014B; // Pr4.05 — DI4
constexpr uint16_t REG_DI5_FUNC            = 0x014D; // Pr4.06 — DI5
constexpr uint16_t REG_DI6_FUNC            = 0x014F; // Pr4.07 — DI6
constexpr uint16_t REG_DI7_FUNC            = 0x0151; // Pr4.08 — DI7

/* --- Digital‑Input Function Values (payloads for REG_DIx_FUNC) ----------- */
/**
 * DI_VAL_ENABLE / DI_VAL_ENABLE_NC
 *  - Binds a DI to the drive’s main enable with the desired polarity (N.O./N.C.).
 * DI_VAL_TRIGGER_CMD
 *  - “CTRG” (command trigger): starts the programmed motion (e.g., PR0) when toggled.
 * DI_VAL_ALARM_CLR
 *  - Clears latched alarms when pulsed.
 * Limit/home inputs
 *  - Positive/Negative/Origin switches for soft stops and homing; wire with proper polarity and debouncing.
 */
constexpr uint16_t DI_VAL_INVALID          = 0x0000; // Unused / no function
constexpr uint16_t DI_VAL_ENABLE           = 0x0008; // Enable (Normally Open)
constexpr uint16_t DI_VAL_ENABLE_NC        = 0x0088; // Enable (Normally Closed)
constexpr uint16_t DI_VAL_ALARM_CLR        = 0x0007; // Alarm clear (momentary)
constexpr uint16_t DI_VAL_TRIGGER_CMD      = 0x0020; // CTRG: trigger programmed motion
constexpr uint16_t DI_VAL_TRIGGER_HOME     = 0x0021; // Start homing routine
constexpr uint16_t DI_VAL_EMG_STOP         = 0x0022; // Quick stop / E‑stop decel
constexpr uint16_t DI_VAL_JOG_PLUS         = 0x0023; // Jog + (uses jog speed/accel settings)
constexpr uint16_t DI_VAL_JOG_MINUS        = 0x0024; // Jog −
constexpr uint16_t DI_VAL_POT_LIMIT        = 0x0025; // Positive limit switch
constexpr uint16_t DI_VAL_NOT_LIMIT        = 0x0026; // Negative limit switch
constexpr uint16_t DI_VAL_ORG_SWITCH       = 0x0027; // Origin / home switch

/* ------------- Digital‑Output Function Mapping (write one DO_VAL_* to each) -------------- */
/**
 * REG_DOx_FUNC
 *  - Map each DO to a status source. Useful for wiring “busy”, “in‑position”, or “alarm” to external logic/LEDs.
 */
constexpr uint16_t REG_DO1_FUNC            = 0x0157; // Pr4.11 — DO1
constexpr uint16_t REG_DO2_FUNC            = 0x0159; // Pr4.12 — DO2
constexpr uint16_t REG_DO3_FUNC            = 0x015B; // Pr4.13 — DO3

/* --- Digital‑Output Function Values (payloads for REG_DOx_FUNC) ---------- */
/**
 * Typical choices:
 *  - DO_VAL_ALARM to light a fault indicator.
 *  - DO_VAL_INPOS_OK to signal motion completion.
 *  - DO_VAL_HOME_OK after homing success.
 */
constexpr uint16_t DO_VAL_COMMAND_OK       = 0x0020; // Last command accepted
constexpr uint16_t DO_VAL_PATH_OK          = 0x0021; // Path ready/OK
constexpr uint16_t DO_VAL_HOME_OK          = 0x0022; // Homing completed
constexpr uint16_t DO_VAL_INPOS_OK         = 0x0023; // In‑position reached
constexpr uint16_t DO_VAL_BRAKE            = 0x0024; // Brake control (if supported)
constexpr uint16_t DO_VAL_ALARM            = 0x0025; // Alarm active

/* ---------------- Current & Stand‑by Parameters --------------------- */
/**
 * Peak and standby current control heating and holding torque.
 *  - REG_PEAK_CURRENT: peak phase current in 0.1 A units (e.g., 35 ⇒ 3.5 A).
 *  - REG_LOCKED_CUR_PERCENT: current as % while shaft is “locked” (holding).
 *  - Standby: after inactivity delay (REG_STANDBY_DELAY_MS), reduce to REG_STANDBY_CUR_PERCENT.
 */
constexpr uint16_t REG_PEAK_CURRENT        = 0x0191; // Pr5.00 — peak current (0.1 A units)
constexpr uint16_t REG_LOCKED_CUR_PERCENT  = 0x0197; // Pr5.03 — locked/hold current (% of peak)
constexpr uint16_t REG_STANDBY_DELAY_MS    = 0x01D1; // Pr5.32 — delay to standby (ms)
constexpr uint16_t REG_STANDBY_CUR_PERCENT = 0x01D3; // Pr5.33 — standby current (% of peak)

/* ---------------- RS‑485 Communication Setup ------------------------ */
/**
 * RS‑485 parameters are stored on the drive.
 *  - REG_RS485_BAUD: enumerated baud rate selector (vendor table; e.g., 0x0005/0x0006/etc.).
 *  - REG_RS485_ID: Modbus slave ID (1..247). Change on a single‑device bus to avoid collisions.
 *  - REG_RS485_DATA_TYPE: data format (usually 8N1). Leave at factory default unless required.
 */
constexpr uint16_t REG_RS485_BAUD          = 0x01BD; // Pr5.22 — baud rate selector (enum)
constexpr uint16_t REG_RS485_ID            = 0x01BF; // Pr5.23 — slave ID
constexpr uint16_t REG_RS485_DATA_TYPE     = 0x01C1; // Pr5.24 — data format (parity/stop)

/* ---------------- PR (Programmable Profile) Motion Registers --------------------- */
/**
 * REG_PR_CONTROL
 *  - Path control/trigger register. Writing the documented bit pattern starts PR execution (“CTRG” in software form).
 *  - Use either a DI mapped to CTRG *or* write to this register from software; do not double‑trigger.
 */
constexpr uint16_t REG_PR_CONTROL          = 0x6002; // Pr8.02 — path control / software trigger

/**
 * PR0 block: one profile slot used by this firmware.
 *  - MODE: 0 = relative position, 1 = absolute position, 2 = constant velocity.
 *  - POS_HIGH/LOW: signed 32‑bit position target split into two 16‑bit words (big‑endian order).
 *  - VELOCITY: target speed in RPM (range per manual, often 0..5000).
 *  - ACCEL/DECEL: time (ms) to change by 1000 RPM. Lower value = sharper accel/brake.
 */
constexpr uint16_t REG_PR0_MODE            = 0x6200; // 0 = rel pos, 1 = abs, 2 = vel
constexpr uint16_t REG_PR0_POS_HIGH        = 0x6201; // position high‑word (bits 31..16 of steps)
constexpr uint16_t REG_PR0_POS_LOW         = 0x6202; // position low‑word  (bits 15..0  of steps)
constexpr uint16_t REG_PR0_VELOCITY        = 0x6203; // RPM (0..model max)
constexpr uint16_t REG_PR0_ACCEL           = 0x6204; // ms per 1000 RPM (accel)
constexpr uint16_t REG_PR0_DECEL           = 0x6205; // ms per 1000 RPM (decel)

/* ---------------- Control Word / Maintenance ----------------------------- */
/**
 * REG_CONTROL_WORD
 *  - Multi‑function control register (vendor‑defined codes).
 *  - CW_SAVE_ALL_PARAMS: persist current parameter set to non‑volatile memory. Use sparingly.
 *  - CW_JOG_CW / CW_JOG_CCW: software jog commands (respect jog speed/accel settings).
 */
constexpr uint16_t REG_CONTROL_WORD        = 0x1801; // Control word / maintenance
constexpr uint16_t CW_SAVE_ALL_PARAMS      = 0x2211; // Save current params to NVRAM (flash write)
constexpr uint16_t CW_JOG_CW               = 0x4001; // Software jog CW
constexpr uint16_t CW_JOG_CCW              = 0x4002; // Software jog CCW

/* ----------------- Helper Macros (host‑side byte packing) ------------------------------------ */
/**
 * MB_HIBYTE / MB_LOBYTE
 *  - Extract big‑endian high/low bytes from 16‑bit values when building Modbus PDUs.
 *  - Example: request = { id, FC, MB_HIBYTE(addr), MB_LOBYTE(addr), MB_HIBYTE(val), MB_LOBYTE(val), ... }
 */
#define MB_HIBYTE(x)   static_cast<uint8_t>((x) >> 8)
#define MB_LOBYTE(x)   static_cast<uint8_t>((x) & 0xFF)

/* -------------------------------- Usage patterns (examples) ------------------------------------
 * 1) Map DI1 to Enable (N.O.) and DI2 to CTRG trigger:
 *    write(REG_DI1_FUNC, DI_VAL_ENABLE);
 *    write(REG_DI2_FUNC, DI_VAL_TRIGGER_CMD);
 *
 * 2) Configure PR0 for a relative move and trigger:
 *    write(REG_PR0_MODE, 0);                    // relative
 *    write(REG_PR0_VELOCITY, rpm);
 *    write(REG_PR0_ACCEL, accel_ms_per_krpm);
 *    write(REG_PR0_DECEL, decel_ms_per_krpm);
 *    write(REG_PR0_POS_HIGH, steps >> 16);
 *    write(REG_PR0_POS_LOW,  steps & 0xFFFF);
 *    write(REG_PR_CONTROL, <vendor bit pattern>); // or pulse DI mapped to CTRG
 *
 * 3) Persist configuration to NVRAM:
 *    write(REG_CONTROL_WORD, CW_SAVE_ALL_PARAMS); // avoid frequent writes
 *
 * Expand this file with additional registers/values as required by your application.
 * When unsure about a value’s encoding or range, check the model‑specific table in the drive manual.
 * ---------------------------------------------------------------------------------------------- */
