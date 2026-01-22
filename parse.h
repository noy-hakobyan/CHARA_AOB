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

static inline bool ieq2(const char *a, char b0, char b1) {
  return a && (tolower(a[0]) == tolower(b0)) && (tolower(a[1]) == tolower(b1)) && a[2] == '\0';
}
static inline bool ieq1(const char *a, char b0) {
  return a && (tolower(a[0]) == tolower(b0)) && a[1] == '\0';
}
static inline bool ieqStr(const char *a, const char *b) {
  if (!a || !b) return false;
  while (*a && *b) { if (tolower(*a++) != tolower(*b++)) return false; }
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
  return "m" + String(id) + ", pos=" + String(m.position) + ", lo=" + lo + ", hi=" + hi + ", lim=" + lim;
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
  uint8_t id = atoi(tok + 1);
  if (id < 1 || id > 22) return;

  char *t1 = strtok(nullptr, " ,\t");
  if (!t1) return;

  // Branch: "st t|f" to toggle polling
  if (ieq2(t1, 's', 't')) {
    char *t2 = strtok(nullptr, " ,\t");
    if (!t2) return;
    if (ieq1(t2, 't')) {
      pollEnabled[id] = true;
      Serial.print("Motor-"); Serial.print(id); Serial.println(" state polling: ON");
      client.print("Motor-"); client.print(id); client.println(" state polling: ON");
    } else if (ieq1(t2, 'f')) {
      pollEnabled[id] = false;
      Serial.print("Motor-"); Serial.print(id); Serial.println(" state polling: OFF");
      client.print("Motor-"); client.print(id); client.println(" state polling: OFF");
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

  // Absolute move: "MoveToXXXX" or "MoveTo XXXX"
  if ( (t1[0] == 'M' || t1[0] == 'm') &&
       (t1[1] == 'o' || t1[1] == 'O') &&
       (t1[2] == 'v' || t1[2] == 'V') &&
       (t1[3] == 'e' || t1[3] == 'E') &&
       (t1[4] == 'T' || t1[4] == 't') &&
       (t1[5] == 'o' || t1[5] == 'O') ) {

    // Either "MoveTo12345" or "MoveTo 12345"
    const char *p = t1 + 6;
    if (*p == '\0') {
      char *t2 = strtok(nullptr, " ,\t");
      if (!t2) { printLineBoth("err=MoveToMissingTarget"); return; }
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

  // Quick stop: "s"
  if (ieq1(t1, 's')) {
    stopMotor(id);
    printLineBoth(fmtStatus(id));
    return;
  }

  // Default: steps with limits unless admin mode ON
  long steps = atol(t1);
  if (steps == 0) { printLineBoth(fmtStatus(id)); return; }

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
    if (steps == 0) { printLineBoth(fmtStatus(id)); return; }
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
      if (idx) { token[idx] = '\0'; parseSingle(token); idx = 0; }
      continue;
    }
    if (idx < sizeof(token) - 1) token[idx++] = c;
  }
  if (idx) { token[idx] = '\0'; parseSingle(token); }
}

#endif // PARSE_H
