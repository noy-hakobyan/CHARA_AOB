#ifndef LASER_H
#define LASER_H

#include <Arduino.h>
#include "config.h"

// Drive IO1 as constant logic 1 at boot; allow runtime on/off
static inline void laserSetup() {
  pinMode(LASER_PIN, OUTPUT);
  digitalWrite(LASER_PIN, HIGH);
}

static inline void laserSet(bool on) {
  digitalWrite(LASER_PIN, on ? HIGH : LOW);
}

static inline bool laserIsOn() {
  return digitalRead(LASER_PIN) == HIGH;
}

#endif // LASER_H
