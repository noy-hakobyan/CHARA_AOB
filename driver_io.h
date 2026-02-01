#ifndef DRIVER_IO_H
#define DRIVER_IO_H

#include <Arduino.h>
#include "config.h"
#include "dm_556_rs_constants.h"
#include "dm_556_rs_frames.h"
#include "runtime_state.h"
#include "nv_store.h"

// Provided by main.ino
MotorState &mById(uint8_t id);

/* ── Dual-port helpers ────────────────────────────────────────────── */
static inline void txPort(HardwareSerial &p, uint8_t *buf) { p.write(buf, 8); }
static inline int  readAvail(HardwareSerial &p)           { return p.available(); }
static inline int  readBytes7(HardwareSerial &p, uint8_t *r){ return p.readBytes(r, 7); }
static inline void flushInput(HardwareSerial &p)          { while (p.available()) p.read(); }

/* ── 8-byte TX with guard ─────────────────────────────────────────── */
static inline void tx(uint8_t *buf) {
  txPort(SerialPortA, buf);
#if USE_COM0
  txPort(SerialPortB, buf);
#endif
  delay(30);
}

/* ── Driver ops ───────────────────────────────────────────────────── */
static inline void enableMotorHW(uint8_t id) {
  uint8_t f[8]; buildEnableFrame(id, f); tx(f);
  mById(id).enabled = true;
}
static inline void disableMotorHW(uint8_t id) {
  uint8_t f[8]; buildDisableFrame(id, f); tx(f);
  mById(id).enabled = false;
}
static inline void initDriver(uint8_t id) {
  uint8_t f[8];
  MotorState &m = mById(id);
  
  // Don't enable motor on startup - only configure parameters
  buildMicrostepFrame(id, m.microstep, f); tx(f);
  buildPeakCurrentFrame(id, m.peakCurr, f); tx(f);
  buildPR0ModeRelFrame(id, f);        tx(f);
  buildPR0VelocityFrame(id, m.velocity, f);  tx(f);
  buildPR0AccelFrame(id, m.accel, f);   tx(f);
  buildPR0DecelFrame(id, m.decel, f);   tx(f);
}

/* ── Enable motor before movement ─────────────────────────────────── */
static inline void ensureMotorEnabled(uint8_t id) {
  MotorState &m = mById(id);
  if (!m.enabled) {
    enableMotorHW(id);
  }
}

/* ── Single-register read (FC 0x03) — dual-bus ────────────────────── */
static inline uint16_t readReg(uint8_t id, uint16_t reg) {
  flushInput(SerialPortA);
#if USE_COM0
  flushInput(SerialPortB);
#endif

  uint8_t req[8] = {id, 0x03, (uint8_t)(reg >> 8), (uint8_t)reg, 0x00, 0x01, 0, 0};
  uint16_t c = modbusCRC(req, 6);
  req[6] = c & 0xFF; req[7] = c >> 8;

  SerialPortA.write(req, 8);
#if USE_COM0
  SerialPortB.write(req, 8);
#endif

  const uint32_t t0 = millis();
  while (millis() - t0 <= 50) {
    if (readAvail(SerialPortA) >= 7) {
      uint8_t r[7]; readBytes7(SerialPortA, r);
#if USE_COM0
      flushInput(SerialPortB);
#endif
      if (r[0] == id && r[1] == 0x03) return (uint16_t(r[3]) << 8) | r[4];
      else return 0xFFFF;
    }
#if USE_COM0
    if (readAvail(SerialPortB) >= 7) {
      uint8_t r[7]; readBytes7(SerialPortB, r);
      flushInput(SerialPortA);
      if (r[0] == id && r[1] == 0x03) return (uint16_t(r[3]) << 8) | r[4];
      else return 0xFFFF;
    }
#endif
  }
  return 0xFFFF;
}

/* ── Relative move using PR0 ──────────────────────────────────────── */
static inline void moveMotor(uint8_t id, int32_t steps) {
  // Enable motor before moving
  ensureMotorEnabled(id);
  
  uint8_t hi[8], lo[8], tr[8];
  buildPR0PositionFrames(id, steps, hi, lo); tx(hi); tx(lo);
  buildTriggerFrame(id, tr); tx(tr);

  MotorState &m = mById(id);
  m.lastMoveMs = millis();
  // record last commanded direction
  m.lastDir = (steps > 0) ? 1 : ((steps < 0) ? -1 : m.lastDir);

  // Controller-tracked position + SD save
  m.position += steps;
  nvSavePosition(id, m.position);
}

/* ── Quick stop (PR control 0x6002 ← 0x0040) ──────────────────────── */
static inline void stopMotor(uint8_t id) {
  uint8_t f[8]; buildWriteFrame(id, 0x6002, 0x0040, f); tx(f);
}

#endif // DRIVER_IO_H
