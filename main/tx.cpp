/**
 * @file tx.cpp
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @brief 
 * @version 0.1
 * @date 2026-09-17
 * 
 * @copyright Copyright (c) 2026
 * 
 */


#include "tx.hpp"
#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>


std::unordered_map<char, TX_DATA> TX_CONFIG;


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


void txPulses(const char* pulses, uint32_t long_us, uint32_t short_us, uint32_t gap_us, int repeats) {
#ifndef TX_ONLY
  detachInterrupt(digitalPinToInterrupt(CC1101_GDO0));
  
  pinMode(CC1101_GDO0, OUTPUT);

  ELECHOUSE_cc1101.SpiStrobe(CC1101_SIDLE);
  delay(1);   // let chip finish leaving Rx
  ELECHOUSE_cc1101.SpiStrobe(CC1101_STX);

  ELECHOUSE_cc1101.SetTx();

  delay(2);   // let the chip calibrate before the first pulse
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
  rxSetup();
#endif
}
