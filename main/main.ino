#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <bitset>

#include "global.h"
#include "tx.hpp"
#include "wifi.hpp"

#ifndef TX_ONLY
#include "rx.hpp"
#endif


void setup() {
  Serial.begin(115200);

#ifdef TX_ONLY
  wifiSetup();
#endif

  ELECHOUSE_cc1101.setSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CSN);
  ELECHOUSE_cc1101.setGDO0(CC1101_GDO0);
  ELECHOUSE_cc1101.Init();

  delay(1000);

  if (ELECHOUSE_cc1101.getCC1101()){         // Check the CC1101 Spi connection.
    Serial.println("CC1101 Connection OK");
  } else{
    Serial.println("CC1101 Connection Error");
  }

  ELECHOUSE_cc1101.setMHZ(433.92);
  ELECHOUSE_cc1101.setCCMode(0);    // 0 for raw data (eg. garage door controller), 1 for comms with other CC1101

  /*
  0 = 2-FSK (2-level Frequency Shift Keying) — the carrier frequency itself shifts slightly up or down to represent a 1 or a 0. Simple, robust, and what your code uses.
  1 = GFSK (Gaussian FSK) — same idea as 2-FSK, but the frequency shifts are smoothed out (Gaussian-filtered) instead of sudden jumps. This keeps the signal's bandwidth narrower and reduces interference with neighboring channels. Very commonly used in practice.
  2 = ASK/OOK (Amplitude Shift Keying / On-Off Keying) — instead of shifting frequency, the amplitude (strength) of the signal is toggled: full power = 1, no power = 0. This is what most cheap remotes, garage doors, and simple sensors use, since it's easy and cheap to generate on the transmitting end.
  3 = 4-FSK — like 2-FSK, but uses four distinct frequency levels instead of two, letting it encode 2 bits per symbol instead of 1 (higher data rate, but more sensitive to noise).
  4 = MSK (Minimum Shift Keying) — a special, more bandwidth-efficient variant of FSK where the frequency shifts are timed precisely to avoid abrupt phase changes. Efficient but more complex to work with.
  */ 
  ELECHOUSE_cc1101.setModulation(2);

  /*
  The general concept: in a normal CC1101-to-CC1101 link, every packet starts with a fixed bit pattern called a sync word (16 or 32 bits, configurable). The receiver is constantly listening to raw noise, and it uses the sync word like a "wake up, this is real data" signal. Once it sees that exact bit pattern, it knows a packet is starting and switches into "capture the following bytes" mode. Without this, the receiver has no way to distinguish real transmissions from random RF noise.

  The setting controls how strictly that sync word has to match:

  0 = No preamble/sync — the chip does no sync-word filtering at all. It just passes through whatever it receives continuously. This is what you want in raw/remote mode, since a garage door remote or generic sensor doesn't transmit any CC1101-style sync word — there's nothing for the chip to "match" against.
  1 = 15/16 bits — a looser match (1 bit of the 16-bit sync word is allowed to be wrong).
  2 = 16/16 bits — an exact 16-bit match required. Fewer false triggers, but more likely to miss a packet if there's a bit error.
  3 = 30/32 bits — like mode 1 but with a doubled (32-bit) sync word for extra reliability.
  4-7 — same patterns as above, but additionally require the signal strength to be above a carrier-sense threshold before triggering.
  */
  ELECHOUSE_cc1101.setSyncMode(0);

  /*
  The general concept: a CRC is a small checksum value calculated from the data in a packet and appended to the end of it.
  */
  ELECHOUSE_cc1101.setCrc(0);

  ELECHOUSE_cc1101.setPA(12);   // 12 - max

#ifdef TX_ONLY
  pinMode(CC1101_GDO0, OUTPUT);
  digitalWrite(CC1101_GDO0, LOW);

  // Stay IDLE until an actual transmit. Leaving the chip keyed in TX (as before)
  // parks the radio transmitting 24/7 and jams nearby 433 MHz receivers. Each send
  // now enters TX via txRadioBegin() and returns here via txRadioEnd() -> SIDLE.
  ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
#else
  rxSetup();
#endif

  initTxConfig();
  initNiceConfig();

  Serial.printf("Verify .pulse (fixed-code):\n");
  for (const auto& [trigger, td] : TX_CONFIG) {
    Serial.printf("%c: %u %u %u %s\n", TX_CONFIG[trigger].trigger, TX_CONFIG[trigger].long_pulse_us, TX_CONFIG[trigger].short_pulse_us, TX_CONFIG[trigger].pulse_gap_us, TX_CONFIG[trigger].pulse_binary.c_str());
    Serial.flush();
  }

  Serial.printf("Verify Nice Flor-S remotes (rolling-code):\n");
  for (const auto& r : NICE_REMOTES) {
    // next_counter is the live value persisted in flash (falls back to the
    // config seed until the first press writes NVS)
    const uint16_t next_counter = niceCounterPeek(r.serial, r.seed_counter);
    Serial.printf("%c: serial=0x%07X btncode=0x%X seed=%u next_counter=%u\n",
                  r.trigger, r.serial, r.btncode, r.seed_counter, next_counter);
    Serial.flush();
  }

  Serial.println("CC1101 setup complete");
}


void loop() {
  static uint32_t prev = (unsigned)millis();
  uint32_t now = (unsigned)millis();
  uint32_t dt = now - prev;
  prev = now;

#ifndef TX_ONLY
  rxLoop(dt);
#endif

  const int serial_ipt = Serial.read();

  if (serial_ipt == 'd') {
    Serial.println("Transmitting..");
    txPulses("11010110011001010101011010101010101010101010011010011010100101101001101010101001100101010101100101010110101", 1000, 500, 18800, 8);
    // Serial.println("Tx Done");
    Serial.printf("Tx Done, MARCSTATE: %u\n", ELECHOUSE_cc1101.SpiReadStatus(CC1101_MARCSTATE) & 0x1F);
  }

  for (const auto& [trigger, td] : TX_CONFIG) {
    if (serial_ipt == trigger) {
      Serial.printf("Tx using TX_CONFIG from .pulse file with trigger %c\n", trigger);
      txPulses(td.pulse_binary.c_str(), td.long_pulse_us, td.short_pulse_us, td.pulse_gap_us, 8);
      Serial.printf("Tx trigger with %c completed\n", trigger);
    }
  }

  // rolling-code remotes: advance the persisted counter, then synthesise a press
  for (const auto& r : NICE_REMOTES) {
    if (serial_ipt == r.trigger) {
      const uint16_t counter = niceCounterNext(r.serial, r.seed_counter);
      Serial.printf("Tx Nice Flor-S %c: serial=0x%07X counter=%u btncode=0x%X\n", r.trigger, r.serial, counter, r.btncode);
      txNiceFlorS(r.serial, counter, r.btncode, 1);   // 1 burst of 16 parcels == one press (one tap)
      Serial.printf("Tx Nice Flor-S %c completed\n", r.trigger);
    }
  }

  // Nice Flor-S enrollment / hold commands (ADD the ESP32 as a new remote; see usage.md).
  //   z = 5s hold of NEW serial   e = Method A main gate   E = Method A side gate   B = Method B side gate (fob-assisted)
  // This receiver learns per-BUTTON, not per-serial, so each gate must be enrolled
  // separately: 'e' authorises with btn 0x1 (main), 'E'/'B' with btn 0x2 (side).
  if (serial_ipt == 'z' || serial_ipt == 'e' || serial_ipt == 'E' || serial_ipt == 'B') {
    const char want = (serial_ipt == 'E' || serial_ipt == 'B') ? 's' : 'm';   // E, B = side gate; else main
    const NiceRemote* fresh = nullptr;
    for (const auto& r : NICE_REMOTES) {
      if (r.trigger == want) { fresh = &r; break; }
    }
    if (!fresh) {
      Serial.printf("[enroll] no '%c' remote entry in NICE_TX_CONFIG\n", want);
    } else if (serial_ipt == 'z') {
      const uint16_t c = niceCounterNext(fresh->serial, fresh->seed_counter);
      Serial.printf("[hold] NEW serial=0x%07X counter=%u btn=0x%X ~6s\n", fresh->serial, c, fresh->btncode);
      txNiceFlorSHold(fresh->serial, c, fresh->btncode, 6000);
      Serial.println("[hold] done");
    } else if (serial_ipt == 'e' || serial_ipt == 'E') {
      niceEnrollAuto(*fresh);
    } else if (serial_ipt == 'B') {
      niceEnrollFob(*fresh);
    }
  }

  wifiEventHandler();
}
