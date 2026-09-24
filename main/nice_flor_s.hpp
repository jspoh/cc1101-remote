/**
 * @file nice_flor_s.hpp
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @brief Self-contained Nice Flor-S decoder + encoder for CC1101 OOK.
 * @date 2026-09-23
 *
 * Ported from the Flipper Zero subghz nice_flor_s protocol, stripped of the
 * Flipper framework. Plain Nice Flor-S only (no O-Code / Nice One variants).
 *
 * ---------------------------------------------------------------------------
 * PROTOCOL STRUCTURE (what one button press looks like on the air)
 * ---------------------------------------------------------------------------
 * A press transmits 16 "parcels" back to back. Each parcel is identical except
 * for a 4-bit rotating index (n = 0..15) that lets the receiver order them.
 *
 * One parcel, OOK, MSB first:
 *   [header]  LOW  for 37 * TE_SHORT   (~18.5 ms of silence -> this is the
 *                                       "pulse_gap_us" you were measuring)
 *   [start]   HIGH for  3 * TE_SHORT, then LOW for 3 * TE_SHORT
 *   [52 data bits] each bit is Manchester-coded across two pulses:
 *                   bit 1 = HIGH TE_LONG  then LOW TE_SHORT
 *                   bit 0 = HIGH TE_SHORT then LOW TE_LONG
 *   [stop]    HIGH for  3 * TE_SHORT
 *
 * The 52 data bits (call the value `data`, MSB = bit 51):
 *   bits 48..51  P0  = button positional code (1:0x1 2:0x2 3:0x4 4:0x8)
 *   bits 44..47  P1  = 0xF ^ P0 ^ n           (parcel index, rotates 0..15)
 *   bits  0..43  enc = encrypt( (serial<<16) | counter )   44-bit ciphertext
 *
 * The cipher is a byte permutation + XOR keyed by a 32-byte substitution
 * table (the "rainbow table"). Decrypting `data` recovers:
 *   counter = dec & 0xFFFF          (16-bit rolling counter, +1 each press)
 *   serial  = (dec >> 16) & 0x0FFFFFFF   (28-bit remote id)
 *   btncode = (dec >> 48) & 0xF          (== P0, cipher leaves bits >=48 alone)
 * ---------------------------------------------------------------------------
 */

#ifndef __NICE_FLOR_S_HPP__
#define __NICE_FLOR_S_HPP__

#include <stdint.h>
#include <stddef.h>

// Symbol timings (us). Flipper uses te_short=500, te_long=1000, te_delta=300.
// Matches your alyssa_gate capture (short ~500, long ~1000).
static constexpr uint32_t NFS_TE_SHORT = 500;
static constexpr uint32_t NFS_TE_LONG  = 1000;
static constexpr uint32_t NFS_TE_DELTA = 300;   // per-symbol timing tolerance

static constexpr uint32_t NFS_HEADER_MULT = 37; // header LOW = 37 * TE_SHORT
static constexpr uint32_t NFS_MARK_MULT   = 3;  // start/stop marks = 3 * TE_SHORT
static constexpr uint32_t NFS_DATA_BITS   = 52; // fixed Flor-S frame length
static constexpr int      NFS_PARCELS     = 4;  // parcels per full transmission (was 16; 4 = ~400ms/press for faster repeats)

// 0xFFFF = plain Nice Flor-S (no installer code / O-Code).
static constexpr uint16_t NFS_IC_PLAIN = 0xFFFFu;

/** Decoded contents of one Flor-S frame. */
struct NiceFlorSFrame {
  uint32_t serial;   // 28-bit remote id
  uint16_t counter;  // 16-bit rolling counter
  uint8_t  btncode;  // 4-bit positional button code (0x1/0x2/0x4/0x8)
};

/**
 * True once NICE_FLOR_S_SBOX holds a non-zero table. When false, decode/encode
 * still run but the cipher output is meaningless (see header note).
 */
bool niceFlorSHasTable();

/**
 * Decode a captured OOK frame into serial/counter/button.
 * @param durs  inter-edge durations (us) as captured by the RX ISR. The leading
 *              ~18.5 ms header LOW is assumed already consumed as a frame gap, so
 *              durs[0] is the HIGH start mark. Pulses strictly alternate
 *              HIGH,LOW,HIGH,... (durs[i] is HIGH when i is even).
 * @param n     number of durations.
 * @param out   filled on success.
 * @return true if a well-formed 52-bit frame was decoded (cipher validity still
 *              depends on the table being present).
 */
bool niceFlorSDecode(const uint32_t* durs, size_t n, NiceFlorSFrame* out);

/**
 * Encrypt a (serial, counter) pair into the 44-bit Flor-S body.
 * Exposed mainly for testing; encode uses it internally.
 */
uint64_t niceFlorSEncrypt(uint64_t plain44, uint16_t ic = NFS_IC_PLAIN);

/** Inverse of niceFlorSEncrypt. */
uint64_t niceFlorSDecrypt(uint64_t enc, uint16_t ic = NFS_IC_PLAIN);

/**
 * Build the full 16-parcel level/duration upload for one press.
 * Writes pairs into out_levels[]/out_durs[]; returns the count written.
 * out arrays must hold at least niceFlorSUploadMax() entries.
 */
size_t niceFlorSBuildUpload(
    uint32_t serial, uint16_t counter, uint8_t btncode,
    bool* out_levels, uint32_t* out_durs, size_t out_cap);

/** Upper bound on entries produced by niceFlorSBuildUpload(). */
size_t niceFlorSUploadMax();

#endif
