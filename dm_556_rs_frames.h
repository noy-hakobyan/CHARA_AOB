#pragma once
#include <Arduino.h>
#include "dm_556_rs_constants.h"

// DM556RS_Frames.h — build-only helpers for Modbus RTU write commands (no I/O)
// Usage summary for C devs new to these drivers and ClearCore:
// - These helpers only build 8-byte Modbus RTU ADUs. They do not transmit.
// - You must call Serial1.write(out, 8) (or your transport) and enforce RS-485 silent time.
// - Function code used here is 0x06 (Write Single Register) only.
// - In the Modbus RTU ADU, addresses/data are big-endian; CRC is LSB-first on the wire.
// - For position moves, PR0 uses a signed 32-bit step target split into two 16-bit words.

// Modbus CRC-16 (poly 0xA001, init 0xFFFF).
// Returns the CRC value; when sending on the wire, send low byte first, then high byte.
// Bitwise implementation (no lookup table). About 8 * len inner iterations.
inline uint16_t modbusCRC(const uint8_t *buf, size_t len) {
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc ^= *buf++;
        for (uint8_t i = 0; i < 8; ++i)
            crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
    return crc; // low byte is sent first on the wire
}

// Build a single FC 0x06 (Write Single Register) request frame.
// Arguments:
// - id: Modbus slave ID (1..247).
// - reg: 16-bit register address (big-endian in PDU).
// - data: 16-bit register payload (big-endian in PDU).
// - out: caller-provided buffer of at least 8 bytes to receive the ADU.
// Frame layout (8 bytes total):
//   out[0] = id
//   out[1] = 0x06
//   out[2] = MB_HIBYTE(reg)
//   out[3] = MB_LOBYTE(reg)
//   out[4] = MB_HIBYTE(data)
//   out[5] = MB_LOBYTE(data)
//   out[6] = CRC low byte (over bytes 0..5)
//   out[7] = CRC high byte
// Note: This function only builds the frame. You must transmit it yourself and respect inter-frame idle.
inline void buildWriteFrame(uint8_t id, uint16_t reg, uint16_t data, uint8_t *out) {
    out[0] = id;
    out[1] = FC_WRITE_SINGLE;          // function 0x06
    out[2] = MB_HIBYTE(reg);
    out[3] = MB_LOBYTE(reg);
    out[4] = MB_HIBYTE(data);
    out[5] = MB_LOBYTE(data);
    uint16_t crc = modbusCRC(out, 6);
    out[6] = crc & 0xFF;               // CRC low byte
    out[7] = crc >> 8;                 // CRC high byte
}

// Convenience: software enable via REG_FORCE_ENABLE (bypasses DI mapping).
// Writes 0x0001 (enable). No response parsing is performed here.
inline void buildEnableFrame (uint8_t id, uint8_t *out) { buildWriteFrame(id, REG_FORCE_ENABLE, 0x0001, out); }

// Convenience: software disable via REG_FORCE_ENABLE.
// Writes 0x0000 (disable).
inline void buildDisableFrame(uint8_t id, uint8_t *out) { buildWriteFrame(id, REG_FORCE_ENABLE, 0x0000, out); }

// Configure PR0 mode to "relative position" per vendor encoding (0x0041).
// Leaves other PR0 parameters unchanged (velocity/accel/decel/position).
inline void buildPR0ModeRelFrame(uint8_t id, uint8_t *out) { buildWriteFrame(id, REG_PR0_MODE, 0x0041, out); }

// Set PR0 velocity in RPM (range is model-specific; see manual).
inline void buildPR0VelocityFrame(uint8_t id, uint16_t rpm, uint8_t *out) { buildWriteFrame(id, REG_PR0_VELOCITY, rpm, out); }

// Set PR0 acceleration parameter (ms per 1000 RPM). Lower value = faster accel.
inline void buildPR0AccelFrame   (uint8_t id, uint16_t val, uint8_t *out) { buildWriteFrame(id, REG_PR0_ACCEL,    val, out); }

// Set PR0 deceleration parameter (ms per 1000 RPM). Lower value = faster decel.
inline void buildPR0DecelFrame   (uint8_t id, uint16_t val, uint8_t *out) { buildWriteFrame(id, REG_PR0_DECEL,    val, out); }

// Set peak current in 0.1A units (e.g., 5 = 0.5A).
inline void buildPeakCurrentFrame(uint8_t id, uint16_t curr, uint8_t *out) { buildWriteFrame(id, REG_PEAK_CURRENT, curr, out); }

// Set microstep resolution via REG_MICROSTEP.
inline void buildMicrostepFrame(uint8_t id, uint16_t code, uint8_t *out) { buildWriteFrame(id, REG_MICROSTEP, code, out); }

// Build two frames for a signed 32-bit relative position (steps).
// Splitting rule (two's-complement):
//   hi = bits 31..16 (uint16_t)((steps32 >> 16) & 0xFFFF)
//   lo = bits 15..0  (uint16_t)( steps32        & 0xFFFF)
// Sequence on the bus for a move:
//   1) write REG_PR0_POS_HIGH
//   2) write REG_PR0_POS_LOW
//   3) issue a trigger (see buildTriggerFrame)
// Caller must provide two separate 8-byte output buffers (outHi, outLo).
inline void buildPR0PositionFrames(uint8_t id, int32_t steps32, uint8_t *outHi, uint8_t *outLo) {
    uint16_t hi = static_cast<uint16_t>((steps32 >> 16) & 0xFFFF);
    uint16_t lo = static_cast<uint16_t>( steps32        & 0xFFFF);
    buildWriteFrame(id, REG_PR0_POS_HIGH, hi, outHi);
    buildWriteFrame(id, REG_PR0_POS_LOW,  lo, outLo);
}

// Build a software trigger frame to execute the programmed PR path.
// Writes 0x0010 to REG_PR_CONTROL (vendor "CTRG" bit pattern).
// Use either this software trigger or a DI mapped to CTRG, not both simultaneously.
inline void buildTriggerFrame(uint8_t id, uint8_t *out) { buildWriteFrame(id, REG_PR_CONTROL, 0x0010, out); }

// Extend with additional builders as required (e.g., FC 0x10 multi-write, jog, homing).
// Keep signatures and behavior stable so existing callers remain compatible.
