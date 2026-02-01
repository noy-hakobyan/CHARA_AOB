# CHARA_AOB

ClearCore control code for DM556RS stepper drivers

This README explains the command format and what each file does.

---

## Startup Behavior

On startup, the system:
1. Initializes SD card and loads motor parameters from `motors.dat` (binary format)
2. Attempts to read `network.txt` from SD card for Ethernet settings
3. If `network.txt` is not found or invalid, uses default settings from `config.h`
4. Initializes all 22 motors (configures parameters but does NOT enable them)
5. Motors only enable when a move command is sent
6. Motors auto-disable after 2 seconds of inactivity

---

## Motor Control Commands

### Move

```
m<id>, <steps>
```

Example:
`m1, 10000` → move Motor 1 forward 10,000 steps
`m1, -8000` → move Motor 1 backward 8,000 steps

**Note:** Sending a move command automatically enables the motor before moving.

### Stop one motor

```
m<id>, s
```

### Stop all motors

```
stop all
```

### Send multiple commands in one line (batch)

```
m1, 10000 + m2, 10000
```

**Warning:** do **not** put two commands for the **same** motor in one line. The second will preempt the first before it finishes.

### Enable / disable motor-state polling (per motor)

```
m<id>, st t   // enable
m<id>, st f   // disable
```

**Tip:** avoid turning polling on for many motors at once on a slow bus.

### Read motor status (one motor)

```
m<id>, read
```

Returns current position, lower/upper limits, and active limit blocks:
```
m1, pos=1500, lo=0, hi=2000, lim=none
```

### Limit Calibration (Admin Mode)

**Enable admin mode first:**
```
admin on
```

**Set lower limit:**
```
m<id>, set lo
```
Sets current position to 0 and marks it as the lower limit.

**Set upper limit:**
```
m<id>, set hi
```
Marks current position as the upper limit.

**Example:**
```
admin on
m1, 0        // move to physical lower end
m1, set lo   // set as lower limit
m1, 100000   // move to physical upper end  
m1, set hi   // set as upper limit
admin off
```

### Motor Parameters (Engineering Mode)

**Enable engineering mode first:**
```
eng on
```

**Change motor parameters:**
```
m<id>, vel=<velocity>, accel=<accel>, decel=<decel>, peak=<peak>, micro=<micro>
```

All parameters are optional; unspecified ones keep their current values.

Parameters:
- `vel` - Velocity (RPM)
- `accel` - Acceleration (ms per 1000 RPM)
- `decel` - Deceleration (ms per 1000 RPM)
- `peak` - Peak current (0.1A units, e.g., 10 = 1.0A)
- `micro` - Microstep resolution (51200 is common)

**Example:**
```
eng on
m1, vel=100, accel=50, decel=50
m1, peak=15
eng off
```

Parameters are saved to SD card and applied to the motor hardware immediately.

### Global Read Commands

**Read all motor parameters:**
```
read all
```

Returns position, limits, velocity, acceleration, deceleration, peak current, and microstep for all 22 motors:
```
=== MOTOR PARAMETERS ===
m1: pos=1500 lo=0 hi=2000 vel=50 accel=50 decel=50 peak=10 micro=51200
m2: pos=0 lo=unset hi=unset vel=50 accel=50 decel=50 peak=10 micro=51200
...
======================
```

**Check driver error codes:**
```
read errors
```

Queries all 22 drivers for alarm/error status:
```
=== DRIVER ERROR CHECK ===
All drivers OK - no errors
```
or
```
m3: ERROR 0x0102
m7: ERROR 0x0205
==========================
```

### Driver Error Codes

Common error codes from DM556RS drivers:

| Code  | Error |
|-------|-------|
| 0x01  | Over-current |
| 0x02  | Over-voltage |
| 0x40  | Current sampling circuit error / current sampling fault |
| 0x80  | Shaft locking error / failed to lock shaft |
| 0x100 | Auto-tuning error / auto-tuning fault |
| 0x200 | EEPROM error / EEPROM fault |

Multiple errors can be present simultaneously (bitwise OR'd). For example:
- `0x0102` = Over-voltage (0x02) + Auto-tuning error (0x100)
- `0x0201` = Over-current (0x01) + EEPROM error (0x200)

---

### Admin Mode

```
admin on   // enable
admin off  // disable
admin      // query status
```

When **admin mode is ON**:
- Soft limits are **ignored** during relative moves
- Direction blocks from limit switches are **ignored**
- Can set limits with `m<id>, set lo` and `m<id>, set hi`

When **admin mode is OFF** (default):
- Soft limits are **enforced**
- Direction blocks from limit switches are **enforced**

### Engineering Mode

```
eng on     // enable
eng off    // disable
eng        // query status
```

When **engineering mode is ON**:
- Can change motor parameters: `m<id>, vel=..., accel=..., decel=..., peak=..., micro=...`
- Parameters are saved to SD card and applied to hardware

When **engineering mode is OFF** (default):
- Attempting to use parameter syntax returns error

---

### Fan control

```
fg   // fan on
fs   // fan off
```

### Laser control

```
laser on   // turn on
laser off  // turn off
```

---

## Motor Polling Output

### Motion State Polling
When enabled (`m<id>, st t`), the system polls motor motion state every ~100ms (cycling through enabled motors):

```
m1 moving
m1 stopped
```

### Limit Switch Status
Limit switches for M1 and M2 are polled every ~10ms and send status updates when state changes:

```
m1, pos=1500, lo=0, hi=2000, lim=pos
m1, pos=1500, lo=0, hi=2000, lim=none
```

Fields:
- `pos=` - Current position (steps)
- `lo=` - Lower limit value (or "unset")
- `hi=` - Upper limit value (or "unset")
- `lim=` - Active limit: `pos` (DI2), `neg` (DI3), or `none`

---

## Limit Switches

### Hardware Setup
- **DI2** (register 0x0147) = Positive limit (POT/PL)
- **DI3** (register 0x0149) = Negative limit (NOT/NL)
- Currently monitored for: **M1 and M2 only**
- Supports both Normally-Open (N.O) and Normally-Closed (N.C) switches (auto-learned on startup)

### To Monitor Additional Motors
Edit `monitors.h`, function `monitorLimitSwitches_M12()`:
```cpp
static const uint8_t kLimitPollIds[] = { 1, 2, 3, 4, 5 };  // Add motor IDs here
```

---

## Network Configuration

### Method 1: SD Card Text File (Recommended)

Create `network.txt` on the SD card with:
```
IP=192.168.2.149
GATEWAY=192.168.3.1
SUBNET=255.255.254.0
DNS=192.168.3.1
PORT=8888
```

This file is read on startup. If missing or invalid, the system falls back to defaults and prints an error.

### Method 2: Hardcoded Defaults

Edit `config.h` for default values (used if `network.txt` is not found).

---

## Data Storage

### Motor Parameters and Positions (`motors.dat`)
- **Format:** Binary (compact, fast)
- **Location:** SD card root
- **Contents per motor:** position, lower limit, upper limit, flags, velocity, acceleration, deceleration, peak current, microstep
- **Size:** 360 bytes total (22 motors × 16 bytes + 8-byte header)
- **Updated:** After every move command, whenever limits are set
- **Magic number:** 0x414F4231 ("AOB1")

### Network Settings (`network.txt`)
- **Format:** Plain text key=value
- **Location:** SD card root
- **Read on:** Startup only
- **Editable:** Yes, with any text editor on a PC

---

## Motor Auto-Disable

- Motors enable automatically when a move command is sent
- Motors disable automatically after **2 seconds** of inactivity (no new move commands)
- This saves power and reduces heat

---

## TCP Communication

### Protocol
- Raw TCP with newline-delimited text messages
- Port: 8888 (configurable via `network.txt`)
- One client connection at a time

### Message Format
Commands: `<command>\r\n` or `<command>\n`
Responses: `<status>\r\n` or `<status>\n`

### Examples
```
→ m1, 1000
← (motor moves, then sends status)

→ admin on
← admin=on

→ m1, st t
← (motor polling enabled)
m1 moving
m1 stopped
```

---

## Files

* **CHARA_AOB_V1.ino**
  Main sketch. Initializes Ethernet/serial, reads network settings from SD card, starts services, runs the cooperative loop (accepts TCP connections, parses commands, polls motor/limit states, auto-disables idle motors).

* **config.h**
  Project settings: default ClearCore IP/gateway/subnet/DNS/port, baud rate, motion presets (speed, acceleration, deceleration), peak current, microstep resolution, pin selection, auto-disable timeout (2000ms).

* **network_config.h**
  Network settings parser. Reads `network.txt` from SD card, parses IP/gateway/subnet/DNS/port, applies settings or falls back to defaults from `config.h` if file is missing or invalid.

* **EthernetTcp.h**
  ClearCore TCP base class helpers.

* **dm_556_rs_constants.h**
  DM556RS driver constants: Modbus register addresses, control values, frame structure.

* **dm_556_rs_frames.h**
  Low-level Modbus RTU frame builders: read/write operations, position frames, trigger frames, enable/disable commands. Used by `driver_io.h`.

* **driver_io.h**
  High-level driver I/O: enable/disable motor, `ensureMotorEnabled()` (called before move), configure PR0 (mode/velocity/accel/decel), send relative move (with auto-enable and position tracking), quick stop, single register reads.

* **fan.h**
  Fan PWM control on IO0. On/off commands and state change reporting.

* **laser.h**
  Laser control via IO1 pin. On/off commands.

* **monitors.h**
  Polling and state reporting: motor motion state (0x0006=stopped, 0x0032=moving), limit switches (M1/M2 DI2=positive, DI3=negative). Sends updates when state changes over TCP and serial.

* **motor_ids.h**
  `constexpr` slave IDs for Motors 1–22 on Modbus.

* **motor_init.h**
  Motor initialization: calls `initDriver()` for each motor. Configures microstep, peak current, PR0 mode/velocity/accel/decel. Motors are NOT enabled during init; they enable only on first move command.

* **motor_state.cpp**
  Global `MotorState motors[22]` array definition with default values (position, limits, velocity, etc.).

* **nv_store.h**
  Non-volatile SD card storage: reads/writes `motors.dat` binary file. Header validation (magic="AOB1", version=1), per-motor entry (position, limits, flags, velocity, accel, decel, peak current, microstep).

* **parse.h**
  Command parser for TCP input. Tokenizes lines, handles commands: move, stop, polling toggle, fan, laser, admin/engineering modes. Supports `+` batching for multiple motors. Sends responses over both serial and TCP.

* **runtime_state.h**
  Declares `struct MotorState` (position, limits, enable flag, block flags, velocity settings, microstep), extern array, and `mById()` helper.
