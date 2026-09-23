/**
 * @file nice_flor_s.cpp
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @brief Nice Flor-S cipher + framing. See nice_flor_s.hpp for the protocol map.
 * @date 2026-09-23
 */

#include "nice_flor_s.hpp"

/*
 * 32-byte Nice Flor-S substitution table ("rainbow table"), supplied by the
 * remote owner for their own device. The cipher indexes this with values 0..31
 * (p[0] & 0x1f  and  p[0] >> 3). Wrong bytes here -> wrong serial/counter.
 */
static const uint8_t NICE_FLOR_S_SBOX[32] = {
    25,  5,  63, 97, 203, 109, 69,  10,  3,   7,  64,  5,  71, 134, 180, 74,
    41, 158, 102, 199, 93, 118, 175, 101, 60,  77, 143, 174, 103, 148, 29, 85
};

// -------------------------------------------------------------------------
// small helpers
// -------------------------------------------------------------------------

static inline uint32_t durDiff(uint32_t a, uint32_t b) {
  return a > b ? a - b : b - a;
}
static inline bool isShort(uint32_t d) { return durDiff(d, NFS_TE_SHORT) < NFS_TE_DELTA; }
static inline bool isLong(uint32_t d)  { return durDiff(d, NFS_TE_LONG)  < NFS_TE_DELTA; }
static inline bool isMark(uint32_t d)  { return durDiff(d, NFS_TE_SHORT * NFS_MARK_MULT) < NFS_TE_DELTA; }

// magic_xor: XOR bytes p[1..5] with k (identical in encrypt and decrypt).
static inline void magicXor(uint8_t* p, uint8_t k) {
  for (uint8_t i = 1; i < 6; i++) p[i] ^= k;
}

bool niceFlorSHasTable() {
  for (int i = 0; i < 32; i++)
    if (NICE_FLOR_S_SBOX[i] != 0) return true;
  return false;
}

// -------------------------------------------------------------------------
// cipher (ported verbatim from Flipper subghz_protocol_nice_flor_s_encrypt_ic
// / _decrypt_ic_buffer; ESP32 is little-endian so p[0] is the LSB, same as the
// original ARM target)
// -------------------------------------------------------------------------

uint64_t niceFlorSEncrypt(uint64_t data, uint16_t ic) {
  uint8_t* p = (uint8_t*)&data;
  uint8_t k = 0;

  for (uint8_t y = 0; y < 2; y++) {
    k = NICE_FLOR_S_SBOX[p[0] & 0x1f];
    magicXor(p, k);
    p[5] &= 0x0f;
    p[0] ^= k & 0xe0;

    k = NICE_FLOR_S_SBOX[p[0] >> 3] + 0x25;   // +0x25: fixed offset from the ref cipher
    magicXor(p, k);
    p[5] &= 0x0f;
    p[0] ^= k & 0x7;

    if (y == 0) { k = p[0]; p[0] = p[1]; p[1] = k; }
  }

  // final permutation; ic == 0xFFFF -> plain Flor-S (the XORs become ~ inversions)
  p[5] = ~p[5] & 0x0f;
  k = ~p[4];
  p[4] = p[0] ^ (uint8_t)(ic >> 8);
  p[0] = ~p[2];
  p[2] = k;
  k = p[1] ^ (uint8_t)ic;
  p[1] = ~p[3];
  p[3] = k;

  return data;
}

uint64_t niceFlorSDecrypt(uint64_t data, uint16_t ic) {
  uint8_t* p = (uint8_t*)&data;
  uint8_t k = 0;

  k = p[4] ^ (uint8_t)(ic >> 8);
  p[5] = ~p[5];
  p[4] = ~p[2];
  p[2] = ~p[0];
  p[0] = k;
  k = p[3] ^ (uint8_t)ic;
  p[3] = ~p[1];
  p[1] = k;

  for (uint8_t y = 0; y < 2; y++) {
    k = NICE_FLOR_S_SBOX[p[0] >> 3] + 0x25;
    magicXor(p, k);
    p[5] &= 0x0f;
    p[0] ^= k & 0x7;

    k = NICE_FLOR_S_SBOX[p[0] & 0x1f];
    magicXor(p, k);
    p[5] &= 0x0f;
    p[0] ^= k & 0xe0;

    if (y == 0) { k = p[0]; p[0] = p[1]; p[1] = k; }
  }

  return data;
}

// -------------------------------------------------------------------------
// decode
// -------------------------------------------------------------------------

bool niceFlorSDecode(const uint32_t* durs, size_t n, NiceFlorSFrame* out) {
  // Need: 2 start marks + 52 bits * 2 pulses = 106 pulses (stop mark optional).
  if (n < 2 + 2 * NFS_DATA_BITS) return false;

  size_t i = 0;

  // start: HIGH mark then LOW mark (both ~3*TE_SHORT)
  if (!isMark(durs[i]) || !isMark(durs[i + 1])) return false;
  i += 2;

  uint64_t data = 0;
  for (uint32_t b = 0; b < NFS_DATA_BITS; b++) {
    const uint32_t hi = durs[i];       // HIGH half (even offset from start)
    const uint32_t lo = durs[i + 1];   // LOW half
    i += 2;

    uint8_t bit;
    if (isShort(hi) && isLong(lo))      bit = 0;
    else if (isLong(hi) && isShort(lo)) bit = 1;
    else return false;                 // malformed pulse pair

    data = (data << 1) | bit;
  }

  const uint64_t dec = niceFlorSDecrypt(data, NFS_IC_PLAIN);
  out->counter = (uint16_t)(dec & 0xFFFF);
  out->serial  = (uint32_t)((dec >> 16) & 0x0FFFFFFFu);  // 28-bit
  out->btncode = (uint8_t)((dec >> 48) & 0xF);           // == P0, cipher-invariant
  return true;
}

// -------------------------------------------------------------------------
// encode
// -------------------------------------------------------------------------

size_t niceFlorSUploadMax() {
  // per parcel: header(1) + start(2) + 52 bits * 2 + stop(1) = 108
  return (size_t)NFS_PARCELS * (1 + 2 + 2 * NFS_DATA_BITS + 1);
}

// append one (level,duration) entry
static inline void emit(bool* lv, uint32_t* du, size_t* idx, bool level, uint32_t dur) {
  lv[*idx] = level;
  du[*idx] = dur;
  (*idx)++;
}

size_t niceFlorSBuildUpload(
    uint32_t serial, uint16_t counter, uint8_t btncode,
    bool* out_levels, uint32_t* out_durs, size_t out_cap) {

  if (out_cap < niceFlorSUploadMax()) return 0;

  // encrypt once; plain Flor-S reuses the body across all 16 parcels
  const uint64_t plain = ((uint64_t)(serial & 0x0FFFFFFFu) << 16) | counter;
  const uint64_t enc   = niceFlorSEncrypt(plain, NFS_IC_PLAIN) & 0x0FFFFFFFFFFFull; // low 44 bits

  size_t idx = 0;
  for (int n = 0; n < NFS_PARCELS; n++) {
    const uint8_t p1 = (uint8_t)(0xF ^ btncode ^ n) & 0xF;   // rotating parcel index
    const uint64_t data =
        ((uint64_t)(btncode & 0xF) << 48) | ((uint64_t)p1 << 44) | enc;

    // header: LOW 37*TE_SHORT (this is the inter-parcel gap)
    emit(out_levels, out_durs, &idx, false, NFS_TE_SHORT * NFS_HEADER_MULT);
    // start marks
    emit(out_levels, out_durs, &idx, true,  NFS_TE_SHORT * NFS_MARK_MULT);
    emit(out_levels, out_durs, &idx, false, NFS_TE_SHORT * NFS_MARK_MULT);

    // 52 data bits, MSB first
    for (int j = NFS_DATA_BITS; j > 0; j--) {
      const bool bit = (data >> (j - 1)) & 1ull;
      if (bit) {                       // bit 1 = HIGH long, LOW short
        emit(out_levels, out_durs, &idx, true,  NFS_TE_LONG);
        emit(out_levels, out_durs, &idx, false, NFS_TE_SHORT);
      } else {                         // bit 0 = HIGH short, LOW long
        emit(out_levels, out_durs, &idx, true,  NFS_TE_SHORT);
        emit(out_levels, out_durs, &idx, false, NFS_TE_LONG);
      }
    }

    // stop mark
    emit(out_levels, out_durs, &idx, true, NFS_TE_SHORT * NFS_MARK_MULT);
  }

  return idx;
}
