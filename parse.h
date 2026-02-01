#ifndef PARSE_H
#define PARSE_H

#include <Arduino.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "driver_io.h"
#include "nv_store.h"
#include "runtime_state.h"
#include "laser.h"

// Provided by main.ino
extern EthernetClient client;
extern uint8_t  fanSetpoint;
extern bool     pollEnabled[23];
extern bool     g_adminMode;
extern bool     g_engineeringMode;

// Case-insensitive helpers
static inline bool ieq2(const char *a, char b0, char b1) {
  return a && (tolower(a[0]) == tolower(b0)) && (tolower(a[1]) == tolower(b1)) && a[2] == '\0';
}
static inline bool ieq1(const char *a, char b0) {
  return a && (tolower(a[0]) == tolower(b0)) && a[1] == '\0';
}
static inline bool ieqStr(const char *a, const char *b) {
  if (!a || !b) return false;
  while (*a && *b) {
    if (tolower(*a++) != tolower(*b++)) return false;
  }
  return *a == '\0' && *b == '\0';
}

static inline void printLineBoth(const String &s) {
  Serial.println(s);
  client.println(s);
}

// Unified status formatter
static inline String fmtStatus(uint8_t id) {
  MotorState &m = mById(id);
  String lo = m.hasLower ? String(m.lower) : String("unset");
  String hi = m.hasUpper ? String(m.upper) : String("unset");
  const char *lim = "none";
  if (m.blockNeg) lim = "neg";
  else if (m.blockPos) lim = "pos";
  return "m" + String(id) + ", pos=" + String(m.position) +
         ", lo=" + lo + ", hi=" + hi + ", lim=" + lim;
}

// ─── Single token parser ────────────────────────────────────────────
static inline void parseSingle(char *cmd) {
  if (!cmd || !*cmd) return;

  // Global: stop all
  if (ieqStr(cmd, "stop all")) {
    for (uint8_t id = 1; id <= 22; ++id) stopMotor(id);
    printLineBoth("all, stop");
    return;
  }

  // Global: admin
  if (ieqStr(cmd, "admin on"))  { g_adminMode = true;  printLineBoth("admin=on");  return; }
  if (ieqStr(cmd, "admin off")) { g_adminMode = false; printLineBoth("admin=off"); return; }
  if (ieqStr(cmd, "admin"))     { printLineBoth(g_adminMode ? "admin=on" : "admin=off"); return; }

  // Global: engineering mode
  if (ieqStr(cmd, "eng on"))  { g_engineeringMode = true;  printLineBoth("eng=on");  return; }
  if (ieqStr(cmd, "eng off")) { g_engineeringMode = false; printLineBoth("eng=off"); return; }
  if (ieqStr(cmd, "eng"))     { printLineBoth(g_engineeringMode ? "eng=on" : "eng=off"); return; }

  // Global: read all motor parameters from SD card
  if (ieqStr(cmd, "read all")) {
    printLineBoth("=== MOTOR PARAMETERS ===");
    for (uint8_t id = 1; id <= 22; ++id) {
      MotorState &m = mById(id);
      String lo = m.hasLower ? String(m.lower) : String("unset");
      String hi = m.hasUpper ? String(m.upper) : String("unset");
      String info = "m" + String(id) + ": pos=" + String(m.position) +
                    " lo=" + lo + " hi=" + hi +
                    " vel=" + String(m.velocity) + " accel=" + String(m.accel) +
                    " decel=" + String(m.decel) + " peak=" + String(m.peakCurr) +
                    " micro=" + String(m.microstep);
      printLineBoth(info);
    }
    printLineBoth("======================");
    return;
  }

  // Global: read error codes from all drivers
  if (ieqStr(cmd, "read errors")) {
    printLineBoth("=== DRIVER ERROR CHECK ===");
    bool hasErrors = false;
    for (uint8_t id = 1; id <= 22; ++id) {
      uint16_t errorCode = readReg(id, REG_ALARM_STATUS);
      delay(10);  // Small delay to avoid serial buffer overrun
      if (errorCode != 0) {
        hasErrors = true;
        printLineBoth("m" + String(id) + ": ERROR 0x" + String(errorCode, HEX));
      }
    }
    if (!hasErrors) {
      printLineBoth("All drivers OK - no errors");
    }
    printLineBoth("==========================");
    return;
  }

  // Global: laser
  if (ieqStr(cmd, "laser on"))  { laserSet(true);  printLineBoth("laser=on");  return; }
  if (ieqStr(cmd, "laser off")) { laserSet(false); printLineBoth("laser=off"); return; }
  if (ieqStr(cmd, "laser"))     { printLineBoth(laserIsOn() ? "laser=on" : "laser=off"); return; }

  // Fan: FG / FS
  if ((cmd[0] == 'F' || cmd[0] == 'f') && cmd[2] == '\0') {
    if (cmd[1] == 'G' || cmd[1] == 'g') { fanSetpoint = FAN_PRESET; printLineBoth("fan=on");  return; }
    if (cmd[1] == 'S' || cmd[1] == 's') { fanSetpoint = 0;          printLineBoth("fan=off"); return; }
  }

  // Expect "Mx ..."
  char *tok = strtok(cmd, " ,\t");
  if (!tok || (tok[0] != 'M' && tok[0] != 'm')) return;
  uint8_t id = (uint8_t)atoi(tok + 1);
  if (id < 1 || id > 22) return;

  char *t1 = strtok(nullptr, " ,\t");
  if (!t1) return;

  // Polling toggle: "st t|f"
  if (ieq2(t1, 's', 't')) {
    char *t2 = strtok(nullptr, " ,\t");
    if (!t2) return;
    if (ieq1(t2, 't')) {
      pollEnabled[id] = true;
      String msg = "Motor-" + String(id) + " polling enabled";
      printLineBoth(msg);
    } else if (ieq1(t2, 'f')) {
      pollEnabled[id] = false;
      String msg = "Motor-" + String(id) + " polling disabled";
      printLineBoth(msg);
    }
    return;
  }

  // Read SD-stored endpoints + current RAM position: "read"
  if (ieqStr(t1, "read")) {
    printLineBoth(fmtStatus(id));
    return;
  }

  // Endpoint calibration: "set lo" / "set hi"
  if (ieqStr(t1, "set")) {
    char *t2 = strtok(nullptr, " ,\t");
    if (!t2) return;
    MotorState &m = mById(id);
    if (ieqStr(t2, "lo")) {
      m.position = 0;
      m.lower = 0;
      m.hasLower = true;
      nvSavePosition(id, 0);
      nvSaveLower(id, 0, true);
      printLineBoth(fmtStatus(id));
      return;
    }
    if (ieqStr(t2, "hi")) {
      m.upper = m.position;
      m.hasUpper = true;
      nvSaveUpper(id, m.upper, true);
      printLineBoth(fmtStatus(id));
      return;
    }
    return;
  }

  // Send driver configuration: "send cfg"
  if (ieqStr(t1, "send")) {
    char *t2 = strtok(nullptr, " ,\t");
    if (t2 && ieqStr(t2, "cfg")) {
      // Re-apply hard-coded driver parameters to this DM556RS
      initDriver(id);
      printLineBoth("m" + String(id) + ", cfg_sent");
    }
    return;
  }

  // Absolute move: "MoveToXXXX" or "MoveTo XXXX"
  if ( (t1[0] == 'M' || t1[0] == 'm') &&
       (t1[1] == 'o' || t1[1] == 'O') &&
       (t1[2] == 'v' || t1[2] == 'V') &&
       (t1[3] == 'e' || t1[3] == 'E') &&
       (t1[4] == 'T' || t1[4] == 't') &&
       (t1[5] == 'o' || t1[5] == 'O')) {

    // Either "MoveTo12345" or "MoveTo 12345"
    const char *p = t1 + 6;
    if (*p == '\0') {
      char *t2 = strtok(nullptr, " ,\t");
      if (!t2) {
        printLineBoth("err=MoveToMissingTarget");
        return;
      }
      p = t2;
    }

    long target = atol(p);
    MotorState &m = mById(id);

    // Apply soft limits in absolute space if not in admin mode
    if (!g_adminMode) {
      if (m.hasLower && target < m.lower) target = m.lower;
      if (m.hasUpper && target > m.upper) target = m.upper;
    }

    long steps = target - m.position;

    if (!g_adminMode) {
      // Respect direction blocks
      if (steps < 0 && m.blockNeg) { printLineBoth(fmtStatus(id)); return; }
      if (steps > 0 && m.blockPos) { printLineBoth(fmtStatus(id)); return; }
    }

    if (steps == 0) {
      // Already at (clamped) target
      printLineBoth(fmtStatus(id));
      return;
    }

    // Report what we are about to do
    String info = "m" + String(id) +
                  ", moveto target=" + String(target) +
                  ", steps=" + String(steps);
    printLineBoth(info);

    enableMotorHW(id);
    moveMotor(id, steps);
    printLineBoth(fmtStatus(id));
    return;
  }

  // Engineering mode: "m1, vel=100, accel=100, decel=100, mcurr=1, hcurr=0"
  if (g_engineeringMode && strchr(t1, '=')) {
    // This is a key=value parameter, parse engineering parameters
    MotorState &m = mById(id);
    uint16_t new_vel = m.velocity;
    uint16_t new_accel = m.accel;
    uint16_t new_decel = m.decel;
    uint16_t new_peak = m.peakCurr;
    uint16_t new_micro = m.microstep;
    
    // Parse t1 first (could be "vel=100")
    char *eq = strchr(t1, '=');
    if (eq) {
      *eq = '\0';
      long val = atol(eq + 1);
      if (ieqStr(t1, "vel"))     new_vel = (uint16_t)val;
      else if (ieqStr(t1, "accel")) new_accel = (uint16_t)val;
      else if (ieqStr(t1, "decel")) new_decel = (uint16_t)val;
      else if (ieqStr(t1, "peak"))  new_peak = (uint16_t)val;
      else if (ieqStr(t1, "micro")) new_micro = (uint16_t)val;
    }
    
    // Parse remaining tokens
    char *tok = strtok(nullptr, " ,\t");
    while (tok) {
      eq = strchr(tok, '=');
      if (eq) {
        *eq = '\0';
        long val = atol(eq + 1);
        if (ieqStr(tok, "vel"))     new_vel = (uint16_t)val;
        else if (ieqStr(tok, "accel")) new_accel = (uint16_t)val;
        else if (ieqStr(tok, "decel")) new_decel = (uint16_t)val;
        else if (ieqStr(tok, "peak"))  new_peak = (uint16_t)val;
        else if (ieqStr(tok, "micro")) new_micro = (uint16_t)val;
      }
      tok = strtok(nullptr, " ,\t");
    }
    
    // Update motor state
    m.velocity = new_vel;
    m.accel = new_accel;
    m.decel = new_decel;
    m.peakCurr = new_peak;
    m.microstep = new_micro;
    
    // Save to SD card
    nvSaveMotorParams(id, new_vel, new_accel, new_decel, new_peak, new_micro);
    
    // Apply to hardware
    initDriver(id);
    
    String resp = "m" + String(id) + ", vel=" + String(new_vel) +
                  ", accel=" + String(new_accel) + ", decel=" + String(new_decel) +
                  ", peak=" + String(new_peak) + ", micro=" + String(new_micro, HEX);
    printLineBoth(resp);
    return;
  }

  // Error: parameter syntax without engineering mode
  if (!g_engineeringMode && strchr(t1, '=')) {
    printLineBoth("ERROR: Engineering mode required to change parameters. Use 'eng on' first.");
    return;
  }

  // Quick stop: "s"
  if (ieq1(t1, 's')) {
    stopMotor(id);
    printLineBoth(fmtStatus(id));
    return;
  }

  // Default: steps with limits unless admin mode ON
  long steps = atol(t1);
  if (steps == 0) {
    printLineBoth(fmtStatus(id));
    return;
  }

  MotorState &m = mById(id);

  if (!g_adminMode) {
    if (steps < 0 && m.blockNeg) { printLineBoth(fmtStatus(id)); return; }
    if (steps > 0 && m.blockPos) { printLineBoth(fmtStatus(id)); return; }

    int32_t desired = m.position + steps;
    if (steps < 0 && m.hasLower && desired < m.lower) {
      steps = m.lower - m.position;   // clamp ≤ 0
    }
    if (steps > 0 && m.hasUpper && desired > m.upper) {
      steps = m.upper - m.position;   // clamp ≥ 0
    }
    if (steps == 0) {
      printLineBoth(fmtStatus(id));
      return;
    }
  }

  enableMotorHW(id);
  moveMotor(id, steps);
  printLineBoth(fmtStatus(id));
}

// ─── Line parser (+ delimiter) ──────────────────────────────────────
static inline void parseLine(char *line) {
  char token[64];
  uint8_t idx = 0;
  for (char *p = line; *p; ++p) {
    char c = *p;
    if (c == '+' || c == '\n' || c == '\r') {
      if (idx) {
        token[idx] = '\0';
        parseSingle(token);
        idx = 0;
      }
      continue;
    }
    if (idx < sizeof(token) - 1) token[idx++] = c;
  }
  if (idx) {
    token[idx] = '\0';
    parseSingle(token);
  }
}

#endif // PARSE_H
