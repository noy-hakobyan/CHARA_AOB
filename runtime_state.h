#ifndef RUNTIME_STATE_H
#define RUNTIME_STATE_H
#include <Arduino.h>

struct MotorState {
  uint8_t  id;
  bool     enabled;
  uint32_t lastMoveMs;

  // Controller-tracked position & manual endpoints
  int32_t  position;   // last known position
  bool     hasLower;   // lower endpoint valid
  bool     hasUpper;   // upper endpoint valid
  int32_t  lower;      // lower endpoint
  int32_t  upper;      // upper endpoint

  // Direction/limit-switch derived blocking
  int8_t   lastDir;    // -1 last commanded negative, +1 positive, 0 none
  bool     blockNeg;   // block further negative moves (e.g., hit min limit while moving -)
  bool     blockPos;   // block further positive moves (e.g., hit max limit while moving +)
};

extern MotorState motors[22];

// Provided by main
MotorState &mById(uint8_t id);

// Global admin mode: when true, limits/blocks are bypassed
extern bool g_adminMode;

#endif
