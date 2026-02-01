#ifndef MONITORS_H
#define MONITORS_H

#include <Arduino.h>
#include "driver_io.h"
#include "runtime_state.h"

// Optionally echo to Ethernet client like other prints
extern EthernetClient client;
extern bool pollEnabled[23];

/* Helper: unified status line like "m1, pos=..., lo=..., hi=..., lim=..." */
static inline String fmtStatusLine(uint8_t id) {
  MotorState &m = mById(id);
  String lo = m.hasLower ? String(m.lower) : String("unset");
  String hi = m.hasUpper ? String(m.upper) : String("unset");
  const char *lim = "none";
  if (m.blockNeg) lim = "neg";
  else if (m.blockPos) lim = "pos";
  return "m" + String(id) + ", pos=" + String(m.position) + ", lo=" + lo + ", hi=" + hi + ", lim=" + lim;
}

void monitorMotionStates() {
  static uint32_t lastPoll = 0;
  static uint8_t  nextId   = 1;
  static uint16_t prev[23]  = {0xFFFF, 0xFFFF, 0xFFFF};

  if (millis() - lastPoll < 100) return;
  lastPoll = millis();

  // find next enabled ID to poll
  uint8_t id = 0;
  for (uint8_t i = 0; i < 22; ++i) {
    uint8_t cand = nextId;
    if (pollEnabled[cand]) { id = cand; break; }
    nextId = (nextId == 22) ? 1 : (uint8_t)(nextId + 1);
  }
  if (!id) return;

  uint16_t ms = readReg(id, 0x1003);
  if (ms != 0xFFFF && (ms == 0x0006 || ms == 0x0032) && ms != prev[id]) {
    // Build complete message as single string
    String msg = "m" + String(id) + " " + (ms == 0x0032 ? "stopped" : "moving");
    
    // Serial message
    Serial.println(msg);
    
    // Ethernet: send as single println call
    if (client && client.connected()) {
      client.println(msg);
    }
  }
  prev[id] = ms;

  delayMicroseconds(5000);
  nextId = (id == 22) ? 1 : (uint8_t)(id + 1);
}


/* ── Limit-switch polling ONLY for M1 and M2 ───────────────────────
   DI2 (0x0147) = Positive limit (POT/PL)
   DI3 (0x0149) = Negative limit (NOT/NL)
   
   Each limit independently blocks motion in that direction.
   Sends status update when any limit state changes.
*/
static const uint8_t kLimitPollIds[] = { 1, 2 };
static const uint8_t kLimitPollCount = sizeof(kLimitPollIds) / sizeof(kLimitPollIds[0]);
static uint8_t  lsInited[23]       = {0};
static uint8_t  lsIdleLevel_DI2[23] = {0};  // DI2 idle (positive limit)
static uint8_t  lsIdleLevel_DI3[23] = {0};  // DI3 idle (negative limit)
static uint8_t  lsPrevPressed_DI2[23] = {0};
static uint8_t  lsPrevPressed_DI3[23] = {0};
static uint32_t lsLastPollMs[23]  = {0};

static inline void monitorLimitSwitches_M12() {
  const uint32_t now = millis();
  for (uint8_t i = 0; i < kLimitPollCount; ++i) {
    const uint8_t id = kLimitPollIds[i];

    if (now - lsLastPollMs[id] < 10) continue;      // per-ID ≈10 ms
    lsLastPollMs[id] = now;

    uint16_t di = readReg(id, 0x0179);              // DI status: bit0..6 = DI1..DI7
    if (di == 0xFFFF) continue;

    uint8_t di2 = (di & 0x0002) ? 1 : 0;            // DI2 (bit 1) = positive limit
    uint8_t di3 = (di & 0x0004) ? 1 : 0;            // DI3 (bit 2) = negative limit
    
    if (!lsInited[id]) {
      lsIdleLevel_DI2[id] = di2;                    // learn idle level (NO/NC agnostic)
      lsIdleLevel_DI3[id] = di3;
      lsInited[id] = 1;
    }
    
    uint8_t pressed_di2 = (di2 != lsIdleLevel_DI2[id]);  // DI2 engaged
    uint8_t pressed_di3 = (di3 != lsIdleLevel_DI3[id]);  // DI3 engaged

    MotorState &m = mById(id);
    bool changed = false;

    // Handle DI2 (positive limit)
    if (pressed_di2 != lsPrevPressed_DI2[id]) {
      changed = true;
      m.blockPos = pressed_di2;
    }
    
    // Handle DI3 (negative limit)
    if (pressed_di3 != lsPrevPressed_DI3[id]) {
      changed = true;
      m.blockNeg = pressed_di3;
    }

    if (changed) {
      String s = fmtStatusLine(id);
      Serial.println(s);
      if (client && client.connected()) {
        client.println(s);
      }
    }

    lsPrevPressed_DI2[id] = pressed_di2;
    lsPrevPressed_DI3[id] = pressed_di3;

    delayMicroseconds(5000);                        // modest RS-485 guard
  }
}

#endif // MONITORS_H
