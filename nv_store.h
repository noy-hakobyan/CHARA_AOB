#ifndef NV_STORE_H
#define NV_STORE_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "config.h"
#include "runtime_state.h"

/* Layout:
   [Header] { magic(4)="AOB1", version(2)=1, pad(2)=0 }  -> 8 bytes
   22 * Entry { position(int32), lower(int32), upper(int32), flags(uint8), pad[3] } -> 16 bytes each
   Total used bytes: 8 + 22*16 = 360; file size rounded to NV_IMAGE_BYTES
*/

struct NvHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t pad;
};

struct NvEntry {
  int32_t position;
  int32_t lower;
  int32_t upper;
  uint8_t flags;      // bit0: hasLower, bit1: hasUpper
  uint16_t velocity;  // RPM
  uint16_t accel;     // ms per 1000 RPM
  uint16_t decel;     // ms per 1000 RPM
  uint16_t peakCurr;  // 0.1A units
  uint16_t microstep; // microstep code
};

static const uint32_t NV_MAGIC   = 0x414F4231UL; // "AOB1"
static const uint16_t NV_VERSION = 1;

static inline int entryOffset(uint8_t id) {
  // id 1..22
  return (int)sizeof(NvHeader) + (int)(id - 1) * (int)sizeof(NvEntry);
}

static inline bool nv_sd_ready() {
  static bool inited = false;
  static bool ok = false;
  if (inited) return ok;
  ok = SD.begin(SD_CS_PIN);
  inited = true;
  return ok;
}

static inline bool nv_read_all(uint8_t *buf, size_t len) {
  if (!nv_sd_ready()) return false;
  File f = SD.open(NV_FILE_NAME, FILE_READ);
  if (!f) return false;
  size_t n = f.read(buf, len);
  f.close();
  return n == len;
}

static inline bool nv_write_all(const uint8_t *buf, size_t len) {
  if (!nv_sd_ready()) return false;
  SD.remove(NV_FILE_NAME);
  File f = SD.open(NV_FILE_NAME, FILE_WRITE);
  if (!f) return false;
  size_t n = f.write(buf, len);
  f.flush();
  f.close();
  return n == len;
}

static inline void nv_zero_file() {
  static uint8_t z[NV_IMAGE_BYTES] = {0};
  nv_write_all(z, sizeof(z));
}

static inline uint8_t *nv_buf() {
  static uint8_t buf[NV_IMAGE_BYTES];
  return buf;
}

static inline void nvInit() {
  if (!nv_sd_ready()) return;

  bool needs_init = false;

  if (!SD.exists(NV_FILE_NAME)) {
    needs_init = true;
  } else {
    File f = SD.open(NV_FILE_NAME, FILE_READ);
    if (!f) {
      needs_init = true;
    } else {
      uint32_t sz = f.size();
      NvHeader hdr{};
      size_t n = f.read((uint8_t *)&hdr, sizeof(hdr));
      f.close();
      if (sz != NV_IMAGE_BYTES || n != sizeof(hdr) || hdr.magic != NV_MAGIC || hdr.version != NV_VERSION) {
        needs_init = true;
      }
    }
  }

  if (needs_init) {
    nv_zero_file();

    uint8_t *buf = nv_buf();
    memset(buf, 0, NV_IMAGE_BYTES);

    NvHeader hdr{};
    hdr.magic = NV_MAGIC;
    hdr.version = NV_VERSION;
    hdr.pad = 0;
    memcpy(buf, &hdr, sizeof(hdr));

    nv_write_all(buf, NV_IMAGE_BYTES);
  }
}

static inline void nvLoadAllFromDisk() {
  if (!nv_sd_ready()) return;

  uint8_t *buf = nv_buf();
  if (!nv_read_all(buf, NV_IMAGE_BYTES)) return;

  NvHeader hdr{};
  memcpy(&hdr, buf, sizeof(hdr));
  if (hdr.magic != NV_MAGIC || hdr.version != NV_VERSION) return;

  for (uint8_t id = 1; id <= 22; ++id) {
    NvEntry e{};
    int off = entryOffset(id);
    memcpy(&e, buf + off, sizeof(e));

    MotorState &m = mById(id);
    m.position = e.position;
    m.lower    = e.lower;
    m.upper    = e.upper;
    m.hasLower = (e.flags & 0x01) != 0;
    m.hasUpper = (e.flags & 0x02) != 0;
    m.velocity = e.velocity ? e.velocity : RPM;
    m.accel = e.accel ? e.accel : ACCEL;
    m.decel = e.decel ? e.decel : DECEL;
    m.peakCurr = e.peakCurr ? e.peakCurr : PEAK_CURRENT;
    m.microstep = e.microstep ? e.microstep : MICROSTEP;
  }
}

/* --- Entry load/store without <functional> or lambdas --- */
static inline bool nvLoadEntry(uint8_t id, NvEntry &out) {
  if (!nv_sd_ready() || id < 1 || id > 22) return false;

  uint8_t *buf = nv_buf();
  if (!nv_read_all(buf, NV_IMAGE_BYTES)) return false;

  NvHeader hdr{};
  memcpy(&hdr, buf, sizeof(hdr));
  if (hdr.magic != NV_MAGIC || hdr.version != NV_VERSION) return false;

  int off = entryOffset(id);
  memcpy(&out, buf + off, sizeof(out));
  return true;
}

static inline bool nvStoreEntry(uint8_t id, const NvEntry &in) {
  if (!nv_sd_ready() || id < 1 || id > 22) return false;

  uint8_t *buf = nv_buf();
  if (!nv_read_all(buf, NV_IMAGE_BYTES)) return false;

  NvHeader hdr{};
  memcpy(&hdr, buf, sizeof(hdr));
  if (hdr.magic != NV_MAGIC || hdr.version != NV_VERSION) return false;

  int off = entryOffset(id);
  memcpy(buf + off, &in, sizeof(in));

  return nv_write_all(buf, NV_IMAGE_BYTES);
}

/* --- Convenience save helpers used by the rest of the code --- */
static inline void nvSavePosition(uint8_t id, int32_t pos) {
  NvEntry e{};
  if (!nvLoadEntry(id, e)) return;
  e.position = pos;
  nvStoreEntry(id, e);
}

static inline void nvSaveLower(uint8_t id, int32_t lower, bool set = true) {
  NvEntry e{};
  if (!nvLoadEntry(id, e)) return;
  e.lower = lower;
  if (set) e.flags = (uint8_t)(e.flags | 0x01);
  nvStoreEntry(id, e);
}

static inline void nvSaveUpper(uint8_t id, int32_t upper, bool set = true) {
  NvEntry e{};
  if (!nvLoadEntry(id, e)) return;
  e.upper = upper;
  if (set) e.flags = (uint8_t)(e.flags | 0x02);
  nvStoreEntry(id, e);
}

static inline void nvSaveMotorParams(uint8_t id, uint16_t vel, uint16_t acc, uint16_t dec, uint16_t peak, uint16_t micro) {
  NvEntry e{};
  if (!nvLoadEntry(id, e)) return;
  e.velocity = vel;
  e.accel = acc;
  e.decel = dec;
  e.peakCurr = peak;
  e.microstep = micro;
  nvStoreEntry(id, e);
}

#endif // NV_STORE_H
