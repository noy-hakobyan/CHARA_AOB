/*********************************************************************
 * ClearCore ⇆ DM556RS — Ethernet-driven motion demo + Fan / Laser
 *********************************************************************/

#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include "ClearCore.h"
#include "EthernetTcpServer.h"

#include "config.h"
#include "dm_556_rs_constants.h"
#include "runtime_state.h"
#include "dm_556_rs_frames.h"
#include "motor_ids.h"
#include "driver_io.h"
#include "parse.h"
#include "monitors.h"
#include "motor_init.h"
#include "fan.h"
#include "nv_store.h"
#include "laser.h"

/* Helper with external linkage (no 'static') */
MotorState &mById(uint8_t id) { return motors[id - 1]; }

/* Global admin mode flag */
bool g_adminMode = false;

/* ─── Ethernet server and RX line buffer ───────────────────────────── */
static unsigned char packetReceived[MAX_PACKET_LENGTH];
static EthernetServer server(PORT_NUM);
EthernetClient client;

/* ─── Fan control (PWM on IO0) ─────────────────────────────────────── */
uint8_t  fanSetpoint = 0;    // 0..255
bool     fanWasOn    = false;

/* ─── Motion-state polling enable flags (per motor) ────────────────── */
bool pollEnabled[23] = { false, false, false };

/* ─── Debug helper: read key DM556RS registers for one motor ─────────
   Reads:
     0x0179  DI status (Pr4.28)
     0x6200  PR0 mode (Pr9.00)
     0x1003  motion-state/status word (you already use this)
     0x2203  current alarm/status
*/
static void debugMotor(uint8_t id) {
  uint16_t di    = readReg(id, 0x0179);
  uint16_t pr0   = readReg(id, 0x6200);
  uint16_t ms    = readReg(id, 0x1003);
  uint16_t alarm = readReg(id, 0x2203);

  Serial.println("===== DM556RS DEBUG =====");
  Serial.print("M"); Serial.print(id);
  Serial.print(" DI(0x0179)   = 0x"); Serial.println(di, HEX);

  Serial.print("M"); Serial.print(id);
  Serial.print(" PR0(0x6200)  = 0x"); Serial.println(pr0, HEX);

  Serial.print("M"); Serial.print(id);
  Serial.print(" MS(0x1003)   = 0x"); Serial.println(ms, HEX);

  Serial.print("M"); Serial.print(id);
  Serial.print(" ALARM(0x2203)= 0x"); Serial.println(alarm, HEX);

  Serial.println("=========================");
}

/* ─── setup() — hardware bring-up ──────────────────────────────────── */
void setup() {
  Serial.begin(9600);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) {}

  fanSetup();
  laserSetup();               // IO1 initial HIGH; runtime toggled via "laser on/off"

  // SD init + NV image prepare/load
  nvInit();
  nvLoadAllFromDisk();

  while (Ethernet.linkStatus() == LinkOFF) {
    delay(500);
  }

  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

  IPAddress ip      = deviceIP();
  IPAddress dns     = deviceDNS();
  IPAddress gateway = deviceGateway();
  IPAddress subnet  = deviceSubnet();

  Ethernet.begin(mac, ip, dns, gateway, subnet);
  server.begin();

  // RS-485 buses: COM-1 always; COM-0 optional
  SerialPort.begin(MODBUS_BAUD);
#if USE_COM0
  Serial0.begin(MODBUS_BAUD);
#endif

  delay(500);
  initAllDrivers();

  Serial.println("System ready. Commands: Mx,steps | Mx st t|f | Mx,s | stop all | Mx set lo|hi | Mx read | admin on|off | laser on|off | FG / FS");
  client.println("System ready. Commands: Mx,steps | Mx st t|f | Mx,s | stop all | Mx set lo|hi | Mx read | admin on|off | laser on|off | FG / FS");

  // Debug one motor at startup (change ID as needed)

}

/* ─── loop() — cooperative scheduler ───────────────────────────────── */
void loop() {
  
  fanRefresh();
  monitorLimitSwitches_M12();   // ONLY M1 and M2
  monitorMotionStates();

  EthernetClient nc = server.accept();
  if (nc.connected()) {
    if (client.connected()) {
      client.stop();
    }
    client = nc;
  }

  if (client.connected()) {
    int len = 0;
    while (client.available() > 0 && len < MAX_PACKET_LENGTH - 1) {
      int16_t r = client.read();
      if (r < 0) break;
      char c = static_cast<char>(r);
      packetReceived[len++] = c;
      if (c == '\n' || c == '\r') break;
    }
    if (len) {
      packetReceived[len] = '\0';
      parseLine(reinterpret_cast<char *>(packetReceived));
      memset(packetReceived, 0, len);
    }
  }

  uint32_t now = millis();
  for (uint8_t id = 1; id <= 22; ++id) {
    MotorState &m = mById(id);
    if (m.enabled && now - m.lastMoveMs >= DISABLE_TIMEOUT_MS) {
      disableMotorHW(id);
    }
  }
  
  EthernetMgr.Refresh();

}
