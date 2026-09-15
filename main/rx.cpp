/**
 * @file rx.cpp
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @brief 
 * @version 0.1
 * @date 2026-09-16
 * 
 * @copyright Copyright (c) 2026
 * 
 */


#include "rx.hpp"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

volatile uint32_t rxFrame[MAX_PULSES];    // populate with pulse timings
volatile uint32_t lastRssiChangeTime = 0;
volatile bool rxFrameReady = false;
// change might not define a frame, but a frame must have changes
volatile uint32_t numPulsesThisFrame = 0;
volatile uint32_t numPulsesThisChange =  0;


void IRAM_ATTR onRssiChange() {
  uint32_t now = micros();

  if (rxFrameReady) return;

  uint32_t delta = now - lastRssiChangeTime;
  lastRssiChangeTime = now;

  if (delta > FRAME_GAP_US) {
    if (numPulsesThisChange > MIN_PULSES) {
      numPulsesThisFrame = numPulsesThisChange;
      rxFrameReady = true;
    }
    numPulsesThisChange = 0;
    return;
  }

  if (delta < MIN_PULSE_US) {
    // drop pulse, is noise
    numPulsesThisChange = 0;
    return;
  }

  rxFrame[numPulsesThisChange++] = delta;

  if (numPulsesThisChange >= MAX_PULSES) {
    // Don't call print in IRAM_ATTR interrupt!!
    // Serial.println("Critical error: Frame will overflow MAX_PULSES value, terminating early to prevent crash");
      numPulsesThisFrame = numPulsesThisChange;
      rxFrameReady = true;
      numPulsesThisChange = 0;
      return;
  }
}


void pulseToBinary(const volatile uint32_t* frame, uint32_t sz, std::bitset<MAX_PULSES>* out, bool* out_drop, uint32_t* out_long_us, uint32_t* out_short_us) {
  auto isShort = [](uint32_t t) { return t >= 250 && t <= 550; };
  auto isLong  = [](uint32_t t) { return t >= 850 && t <= 1400; };

  out->reset();

  uint32_t sum_long_dur_us = 0;
  int num_long = 0;
  uint32_t sum_short_dur_us = 0;
  int num_short = 0;

  for (uint32_t i=0; i<sz; ++i) {
    if (isLong(frame[i])) {
      out->set(i);
      sum_long_dur_us += frame[i];
      ++num_long;
    }
    else if (isShort(frame[i])) {
      sum_short_dur_us += frame[i];
      ++num_short;
    }
    else {
      // malformed packet
      *out_drop = true;
      return;
    }
  }

  // calculate average pulse dur
  if (num_long == 0 || num_short == 0) {
    *out_drop = true;
    return;
  }
  *out_long_us = sum_long_dur_us / num_long;
  *out_short_us = sum_short_dur_us / num_short;

  // verify packet is valid (no long+long or short+short combos)
  for (uint32_t i=0; i+1<sz; i+=2) {
    // first and second bit
    const bool fbit = (*out)[i];
    const bool sbit = (*out)[i+1];
    if (fbit == sbit) {
      *out_drop = true;
      return;
    }
  }

  *out_drop = false;
  return;
}

void rxSetup() {
  ELECHOUSE_cc1101.SetRx();
  pinMode(CC1101_GDO0, INPUT);
  attachInterrupt(digitalPinToInterrupt(CC1101_GDO0), onRssiChange, CHANGE);
}


void rxLoop(uint32_t dt_ms) {
  static uint32_t elapsed_ms = 0;

  if (elapsed_ms < PRINT_NOISE_DUR_MS) {
    elapsed_ms += dt_ms;
    Serial.print(ELECHOUSE_cc1101.getRssi());   // received signal strength indicator -30dBm(decibel watts) is strong, -90dBm is weak
    Serial.println(" dBm");
  }

  /*
  From testing, background RSSI fluctuates around 93-96dBm
  */

  if (rxFrameReady) {
    // copy to allow interrupt to keep running
    const uint32_t n = numPulsesThisFrame;
    static uint32_t rxFrameCopy[MAX_PULSES];
    memcpy(rxFrameCopy, (const void*)rxFrame, n*sizeof(rxFrame[0]));
    rxFrameReady = false;

    static const bool printRawPulseTimings = true;
    if (printRawPulseTimings) {
      Serial.printf("Pulses(%d): ", n);
      for (uint16_t i=0; i<n; ++i) {
        Serial.printf("%d%s", rxFrameCopy[i], i+1>=n ? "" : ", ");
      }
      Serial.printf(")\n");
    }

    bool dropPacket = false;
    std::bitset<MAX_PULSES> pulseBinary;
    uint32_t long_pulse_us = 0;
    uint32_t short_pulse_us = 0;
    pulseToBinary(rxFrameCopy, n, &pulseBinary, &dropPacket, &long_pulse_us, &short_pulse_us);

    if (!dropPacket) {
      Serial.printf("\n--\nPulse binary(%d) - Read left to right:\nLong pulse (us): %d\nShort pulse (us): %d\n", n, long_pulse_us, short_pulse_us);
      for (uint32_t i=0; i<n; ++i) {
        Serial.printf("%d%s", pulseBinary[i] ? 1 : 0, i != 0 && i%4 == 0 ? " " : "");
      }
      Serial.printf("\n--\n");
    }
  }
}
