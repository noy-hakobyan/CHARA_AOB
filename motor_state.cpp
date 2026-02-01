#include "runtime_state.h"
#include "motor_ids.h"
#include "config.h"

MotorState motors[22] = {
  {MOTOR1_ID,  false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR2_ID,  false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR3_ID,  false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR4_ID,  false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR5_ID,  false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR6_ID,  false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR7_ID,  false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR8_ID,  false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR9_ID,  false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR10_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR11_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR12_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR13_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR14_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR15_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR16_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR17_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR18_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR19_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR20_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR21_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP},
  {MOTOR22_ID, false, 0, 0, false, false, 0, 0, 0, false, false, RPM, ACCEL, DECEL, PEAK_CURRENT, MICROSTEP}
};
