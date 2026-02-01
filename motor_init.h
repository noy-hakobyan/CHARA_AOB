#ifndef MOTOR_INIT_H
#define MOTOR_INIT_H

#include "driver_io.h"
#include "motor_ids.h"

static inline void initAllDrivers() {
  initDriver(MOTOR1_ID);
  initDriver(MOTOR2_ID);
  initDriver(MOTOR3_ID);
  initDriver(MOTOR4_ID);
  initDriver(MOTOR5_ID);
  initDriver(MOTOR6_ID);
  initDriver(MOTOR7_ID);
  initDriver(MOTOR8_ID);
  initDriver(MOTOR9_ID);
  initDriver(MOTOR10_ID);
  initDriver(MOTOR11_ID);
  initDriver(MOTOR12_ID);
  initDriver(MOTOR13_ID);
  initDriver(MOTOR14_ID);
  initDriver(MOTOR15_ID);
  initDriver(MOTOR16_ID);
  initDriver(MOTOR17_ID);
  initDriver(MOTOR18_ID);
  initDriver(MOTOR19_ID);
  initDriver(MOTOR20_ID);
  initDriver(MOTOR21_ID);
  initDriver(MOTOR22_ID);
}

#endif // MOTOR_INIT_H
