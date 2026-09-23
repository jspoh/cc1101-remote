/**
 * @file tx.hpp
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @date 2026-09-17
 */

#ifndef __TX_HPP__
#define __TX_HPP__

#include "global.h"
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>


// =========================================================================
// FIXED-CODE remotes (fans etc.): raw long/short OOK replay. Unchanged.
// Format per entry:  trigger,long_us,short_us,gap_us,pulse_binary|
// =========================================================================
inline const std::string RAW_TX_CONFIG =
R"(f,1201,397,13000,1010101010101001101010010101010110011001011010100|l,1203,395,13000,1010101010101001101010010101010110011001011001010|o,1200,398,13000,1010101010101001101010010101010110011001011001100|1,1201,396,13000,1010101010101001101010010101010110011001010101100|2,1199,398,13000,1010101010101001101010010101010110011001010110100|3,1198,400,13000,1010101010101001101010010101010110011001011010010|4,1199,398,13000,1010101010101001101010010101010110011001100110010|5,1195,403,13000,1010101010101001101010010101010110011001100101100|6,1201,396,13000,1010101010101001101010010101010110011001100101010|h,1204,394,13000,1010101010101001101010010101010110011001101010010|H,1199,399,13000,1010101010101001101010010101010110011001100110100|c,1201,397,13000,1010101010101001101010010101010110011001101001010|-,1201,397,13000,1010101010101001101010010101010110011001010110010|+,1199,399,13000,1010101010101001101010010101010110011001010101010|)";

class TX_DATA {
public:
  char trigger;
  uint32_t long_pulse_us;
  uint32_t short_pulse_us;
  uint32_t pulse_gap_us;
  std::string pulse_binary;

  TX_DATA() {}
  TX_DATA(char t, uint32_t lpu, uint32_t spu, uint32_t pgu, const std::string& pb) : trigger{t}, long_pulse_us{lpu}, short_pulse_us{spu}, pulse_gap_us{pgu}, pulse_binary{pb} {}
};

extern std::unordered_map<char, TX_DATA> TX_CONFIG;

void initTxConfig();
void txPulses(const char* pulses, uint32_t long_us, uint32_t short_us, uint32_t gap_us, int repeats);


// =========================================================================
// ROLLING-CODE remotes (Nice Flor-S). The counter changes every press, so we
// store the remote's identity (serial/button/seed counter) and SYNTHESISE a
// fresh frame each time rather than replaying a capture.
// Format per entry:  trigger,serial_hex,seed_counter,btncode_hex|
//   e.g.  g,0436c682,1092,1|
// btncode is the 4-bit positional code: 1:0x1 2:0x2 3:0x4 4:0x8
// =========================================================================
inline const std::string NICE_TX_CONFIG =
R"(m,347a033,45777,1|s,347a033,45777,2|)";   // filled by pulse_configs/json_to_pulse.py from a nice_flor_s .json

struct NiceRemote {
  char trigger;
  uint32_t serial;
  uint16_t seed_counter;   // starting point; NVS keeps the live value going forward
  uint8_t btncode;
};

extern std::vector<NiceRemote> NICE_REMOTES;

void initNiceConfig();

/**
 * Transmit a full Nice Flor-S press (16 parcels) `bursts` times.
 * Uses `counter` as-is for this transmission; the caller is responsible for
 * persisting the next counter (see niceCounterNext).
 */
void txNiceFlorS(uint32_t serial, uint16_t counter, uint8_t btncode, int bursts);

/**
 * Return the counter to use for the next press of `serial`, advancing the
 * value stored in NVS (flash). First call seeds NVS from `seed` if nothing is
 * stored.
 */
uint16_t niceCounterNext(uint32_t serial, uint16_t seed);

/**
 * Read the persisted next-counter for `serial` WITHOUT advancing it (returns
 * `seed` if nothing is stored yet). Used to print live counters on init.
 */
uint16_t niceCounterPeek(uint32_t serial, uint16_t seed);


#endif
