#ifndef FAN_H
#define FAN_H

#include <Arduino.h>
#include "config.h"

extern uint8_t fanSetpoint; // written by FG/FS commands
extern bool    fanWasOn;    // state-change reporting

static inline void fanSetup() {
  pinMode(FAN_PWM_PIN, OUTPUT);
  analogWrite(FAN_PWM_PIN, 0);
}

static inline void fanRefresh() {
  analogWrite(FAN_PWM_PIN, fanSetpoint);
  const bool fanOn = (fanSetpoint > 0);
  if (fanOn != fanWasOn) {
    if (fanOn) Serial.println("Fan ON");
    else       Serial.println("Fan OFF");
    fanWasOn = fanOn;
  }
}

#endif // FAN_H
