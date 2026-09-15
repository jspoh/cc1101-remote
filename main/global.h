/**
 * @file global.h
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @brief 
 * @version 0.1
 * @date 2026-09-16
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef __GLOBAL_H__
#define __GLOBAL_H__


#define CC1101_GDO0 0
#define CC1101_CSN  1
#define CC1101_SCK  2
#define CC1101_MOSI 3
#define CC1101_MISO 4
// #define CC1101_GD02 5


// microseconds (us)
#define MIN_PULSE_US 150
#define FRAME_GAP_US 4000   // silence after 4000 us is a gap
#define MIN_PULSES 24       // ignore frarmes shorter than 24 MIN_PULSES
#define MAX_PULSES 256       


#endif
