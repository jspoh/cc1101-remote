/**
 * @file tx.cpp
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @date 2026-09-17
 */

#include "tx.hpp"
#include "nice_flor_s.hpp"
#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <Preferences.h>
#include "rx.hpp"


std::unordered_map<char, TX_DATA> TX_CONFIG;
std::vector<NiceRemote> NICE_REMOTES;

// NVS store for rolling counters, keyed by remote serial.
static Preferences niceCounterStore;
static const char* NICE_NVS_NAMESPACE = "nfs";


void initTxConfig() {
  if (RAW_TX_CONFIG.size() == 0) return;

  for (int i=0; i<RAW_TX_CONFIG.size();) {
    const char trigger = RAW_TX_CONFIG[i];
    TX_CONFIG[trigger].trigger = trigger;
    i+=2;

    uint32_t timings[3];
    for (int j=0; j<3; ++j) {
      std::string timing;
      while (RAW_TX_CONFIG[i] != ',') {
        timing += RAW_TX_CONFIG[i];
        ++i; 
      }
      timings[j] = (uint32_t)std::stoul(timing);
      ++i;
    }

    TX_CONFIG[trigger].long_pulse_us = timings[0];
    TX_CONFIG[trigger].short_pulse_us = timings[1];
    TX_CONFIG[trigger].pulse_gap_us = timings[2];

    TX_CONFIG[trigger].pulse_binary.clear();
    while (RAW_TX_CONFIG[i] != '|') {
      TX_CONFIG[trigger].pulse_binary += RAW_TX_CONFIG[i];
      ++i;
    }
    ++i;
  }
}


void initNiceConfig() {
  const std::string& s = NICE_TX_CONFIG;
  if (s.size() == 0) return;

  for (size_t i = 0; i < s.size();) {
    NiceRemote r{};
    r.trigger = s[i];
    i += 2;   // trigger + ','

    // helper: read until one of the delimiters ',' or '|'
    auto field = [&](void) -> std::string {
      std::string out;
      while (i < s.size() && s[i] != ',' && s[i] != '|') out += s[i++];
      ++i;   // consume delimiter
      return out;
    };

    r.serial       = (uint32_t)std::stoul(field(), nullptr, 16);  // serial is hex
    r.seed_counter = (uint16_t)std::stoul(field());               // counter is decimal
    r.btncode      = (uint8_t)std::stoul(field(), nullptr, 16);   // btncode is hex nibble

    NICE_REMOTES.push_back(r);
  }
}


// -------------------------------------------------------------------------
// radio TX enter/leave (shared by both fixed-code and Nice paths)
// -------------------------------------------------------------------------
static void txRadioBegin() {
#ifndef TX_ONLY
  detachInterrupt(digitalPinToInterrupt(CC1101_GDO0));
#endif
  pinMode(CC1101_GDO0, OUTPUT);
  ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
  delay(1);              // let chip finish leaving Rx
  ELECHOUSE_cc1101.SpiStrobe(CC1101_STX);
  ELECHOUSE_cc1101.SetTx();
  delay(2);              // let the chip calibrate before the first pulse
}

static void txRadioEnd() {
#ifndef TX_ONLY
  rxSetup();
#endif
}


void txPulses(const char* pulses, uint32_t long_us, uint32_t short_us, uint32_t gap_us, int repeats) {
#ifndef TX_ONLY
  txRadioBegin();
#endif

  Serial.printf("MARCSTATE: %u\n", ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 0x1F);

  for (int i=0; i<repeats; ++i) {
    bool power = HIGH;
    for (const char* p = pulses; *p != '\0'; ++p) {
      if (*p == ' ') continue;
      digitalWrite(CC1101_GDO0, power);
      delayMicroseconds(*p == '1' ? long_us : short_us);
      power = !power;
    }

    // gap
    digitalWrite(CC1101_GDO0, LOW);
    delayMicroseconds(gap_us);
  }

#ifndef TX_ONLY
  txRadioEnd();
#endif
}


// -------------------------------------------------------------------------
// Nice Flor-S transmit
// -------------------------------------------------------------------------
uint16_t niceCounterNext(uint32_t serial, uint16_t seed) {
  char key[9];
  snprintf(key, sizeof(key), "%08X", serial);   // per-serial NVS key

  niceCounterStore.begin(NICE_NVS_NAMESPACE, false);   // read/write
  uint16_t cur = niceCounterStore.getUShort(key, 0xFFFF);
  if (cur == 0xFFFF) cur = seed;                        // first ever use -> seed
  uint16_t next = (uint16_t)(cur + 1);                  // wraps naturally at 0x10000
  niceCounterStore.putUShort(key, next);
  niceCounterStore.end();

  return cur;   // transmit with the current value, next press uses `next`
}

uint16_t niceCounterPeek(uint32_t serial, uint16_t seed) {
  char key[9];
  snprintf(key, sizeof(key), "%08X", serial);

  niceCounterStore.begin(NICE_NVS_NAMESPACE, true);    // read-only
  uint16_t cur = niceCounterStore.getUShort(key, 0xFFFF);
  niceCounterStore.end();

  return (cur == 0xFFFF) ? seed : cur;                 // unset -> config seed
}

void txNiceFlorS(uint32_t serial, uint16_t counter, uint8_t btncode, int bursts) {
  // static so the ~1.7 KB pair of buffers isn't on the stack
  static bool     levels[1728];   // == niceFlorSUploadMax()
  static uint32_t durs[1728];

  const size_t count = niceFlorSBuildUpload(serial, counter, btncode, levels, durs, sizeof(durs) / sizeof(durs[0]));
  if (count == 0) {
    Serial.println("[nice_flor_s] upload buffer too small, aborting");
    return;
  }
  if (!niceFlorSHasTable()) {
    Serial.println("[nice_flor_s] warning: SBOX table empty, transmitting garbage");
  }

#ifndef TX_ONLY
  txRadioBegin();
#endif

  Serial.printf("MARCSTATE: %u\n", ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 0x1F);

  for (int b = 0; b < bursts; ++b) {
    for (size_t i = 0; i < count; ++i) {
      digitalWrite(CC1101_GDO0, levels[i] ? HIGH : LOW);
      delayMicroseconds(durs[i]);
    }
  }
  digitalWrite(CC1101_GDO0, LOW);   // leave carrier off

#ifndef TX_ONLY
  txRadioEnd();
#endif
}


// -------------------------------------------------------------------------
// Nice Flor-S enrollment (ADD a new remote) -- see nice_flor_s.md / plan.txt
// -------------------------------------------------------------------------

// Step 1 building block: "hold the button" ~hold_ms at a FIXED counter.
void txNiceFlorSHold(uint32_t serial, uint16_t counter, uint8_t btncode, uint32_t hold_ms) {
  static bool     levels[1728];   // == niceFlorSUploadMax()
  static uint32_t durs[1728];

  const size_t count = niceFlorSBuildUpload(serial, counter, btncode, levels, durs, sizeof(durs) / sizeof(durs[0]));
  if (count == 0) {
    Serial.println("[nice_flor_s] hold: upload buffer too small, aborting");
    return;
  }
  if (!niceFlorSHasTable()) {
    Serial.println("[nice_flor_s] warning: SBOX table empty, transmitting garbage");
  }

#ifndef TX_ONLY
  txRadioBegin();
#endif
  const uint32_t start = millis();
  do {
    for (size_t i = 0; i < count; ++i) {
      digitalWrite(CC1101_GDO0, levels[i] ? HIGH : LOW);
      delayMicroseconds(durs[i]);
    }
  } while ((uint32_t)(millis() - start) < hold_ms);
  digitalWrite(CC1101_GDO0, LOW);
#ifndef TX_ONLY
  txRadioEnd();
#endif
}

// METHOD A: ESP32 does the whole sequence, using the old serial as authoriser.
void niceEnrollAuto(const NiceRemote& fresh) {
  Serial.printf("[enroll A] NEW serial=0x%07X btn=0x%X  OLD(auth) serial=0x%07X\n",
                fresh.serial, fresh.btncode, (uint32_t)NICE_AUTH_SERIAL);

  Serial.println("[enroll A] step1: hold NEW ~6s");
  const uint16_t c1 = niceCounterNext(fresh.serial, fresh.seed_counter);
  txNiceFlorSHold(fresh.serial, c1, fresh.btncode, 6000);

  Serial.println("[enroll A] step2: OLD x3 (authorise)");
  for (int k = 0; k < 3; ++k) {
    const uint16_t co = niceCounterNext(NICE_AUTH_SERIAL, NICE_AUTH_SEED);
    Serial.printf("  OLD press %d serial=0x%07X counter=%u\n", k + 1, (uint32_t)NICE_AUTH_SERIAL, co);
    txNiceFlorS(NICE_AUTH_SERIAL, co, fresh.btncode, 1);
    delay(600);
  }

  Serial.println("[enroll A] step3: NEW once (confirm)");
  const uint16_t c3 = niceCounterNext(fresh.serial, fresh.seed_counter);
  txNiceFlorS(fresh.serial, c3, fresh.btncode, 1);

  Serial.println("[enroll A] sent. Watch the receiver LED for 3 slow flashes = learned.");
}

// METHOD B: ESP32 sends NEW; operator presses the physical fob 3x for step 2.
void niceEnrollFob(const NiceRemote& fresh) {
  Serial.printf("[enroll B] NEW serial=0x%07X btn=0x%X\n", fresh.serial, fresh.btncode);

  Serial.println("[enroll B] step1: hold NEW ~6s");
  const uint16_t c1 = niceCounterNext(fresh.serial, fresh.seed_counter);
  txNiceFlorSHold(fresh.serial, c1, fresh.btncode, 6000);

  Serial.println("[enroll B] Now press your OLD fob 3x, then send any key to confirm...");
  while (Serial.read() < 0) { delay(10); }

  Serial.println("[enroll B] step3: NEW once (confirm)");
  const uint16_t c3 = niceCounterNext(fresh.serial, fresh.seed_counter);
  txNiceFlorS(fresh.serial, c3, fresh.btncode, 1);

  Serial.println("[enroll B] sent. Watch the receiver LED for 3 slow flashes = learned.");
}
