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
