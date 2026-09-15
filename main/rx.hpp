/**
 * @file rx.h
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @brief 
 * @version 0.1
 * @date 2026-09-16
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef __RX_H__
#define __RX_H__

#include <stdint.h>
#include <bitset>
#include <Arduino.h>
#include "global.h"

static constexpr uint32_t PRINT_NOISE_DUR_MS = 10000;

void pulseToBinary(const uint32_t* frame, uint32_t sz, std::bitset<MAX_PULSES>* out, bool* out_drop, uint32_t* out_long_us, uint32_t* out_short_us);

void IRAM_ATTR onRssiChange();

void rxSetup();

void rxLoop(uint32_t dt_ms);

#endif