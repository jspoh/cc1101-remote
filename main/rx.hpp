/**
 * @file rx.hpp
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @date 2026-09-16
 */

#ifndef __RX_H__
#define __RX_H__

#include <stdint.h>
#include <bitset>
#include <Arduino.h>
#include "global.h"

static constexpr uint32_t PRINT_NOISE_DUR_MS = 10000;

// --- fixed-code (raw OOK long/short) path, unchanged: for the fans etc. ---
void pulseToBinary(const uint32_t* frame, uint32_t sz, std::bitset<MAX_PULSES>* out, bool* out_drop, uint32_t* out_long_us, uint32_t* out_short_us);

void IRAM_ATTR onRssiChange();

void rxSetup();

void rxLoop(uint32_t dt_ms);

#endif
