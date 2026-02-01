#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Ethernet.h>

/* ── Network ──────────────────────────────────────────────────────── */
// Static IP for ClearCore
static inline IPAddress deviceIP()      { return IPAddress(192, 168, 2, 149); }

// Gateway (Put CHARA one)
static inline IPAddress deviceGateway() { return IPAddress(192, 168, 3, 1); }

// DNS server 
static inline IPAddress deviceDNS()     { return deviceGateway(); }

// Netmask 255.255.254.0 
static inline IPAddress deviceSubnet()  { return IPAddress(255, 255, 254, 0); }

#define PORT_NUM            8888
#define MAX_PACKET_LENGTH   256

/* ── RS-485 / Modbus (DM556RS) ────────────────────────────────────── */
#define SerialPort          Serial1
#define MODBUS_BAUD         19200UL

/* Optional second RS-485 port */
#define USE_COM0            1
#define SerialPortA         Serial1   // COM-1
#define SerialPortB         Serial0   // COM-0

/* PR0 motion presets */
#define RPM                 50u
#define ACCEL               50u
#define DECEL               50u
#define PEAK_CURRENT        10u    /* 0.1A units: 5 = 0.5A */
#define MICROSTEP           51200u /* steps per revolution */

/* Auto-disable */
#define DISABLE_TIMEOUT_MS  2000UL

/* Fan PWM */
#define FAN_PWM_PIN         IO0
#define FAN_PRESET          60

/* ── SD-card persistence ──────────────────────────────────────────── */
#ifndef SD_CS_PIN
#define SD_CS_PIN 4
#endif
#ifndef NV_FILE_NAME
#define NV_FILE_NAME "aob_nv.bin"
#endif
#ifndef NV_IMAGE_BYTES
#define NV_IMAGE_BYTES 2048
#endif

/* ── Laser output ─────────────────────────────────────────────────── */
#define LASER_PIN           IO1    // drive via laser.h

#endif // CONFIG_H
