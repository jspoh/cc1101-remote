"""
Decode sniffed Nice Flor-S captures (.json with pulse_binary) into serial/counter/button.

Mirrors the firmware cipher: main/nice_flor_s.cpp niceFlorSDecrypt + NICE_FLOR_S_SBOX.
Use it to answer "clone vs added": if two remotes decode to the SAME serial they are
clones; different serials mean distinct identities.

    python nice_flor_s_decode.py ../tmp/mg.json ../tmp/vers.json
"""
import json, sys

# 32-byte substitution table -- must match main/nice_flor_s.cpp NICE_FLOR_S_SBOX
SBOX = [25,5,63,97,203,109,69,10,3,7,64,5,71,134,180,74,
        41,158,102,199,93,118,175,101,60,77,143,174,103,148,29,85]
IC = 0xFFFF  # 0xFFFF = plain Nice Flor-S (no O-Code installer code)


def _magic_xor(p, k):
    for i in range(1, 6):
        p[i] = (p[i] ^ k) & 0xFF


def decrypt(data, ic=IC):
    p = [(data >> (8 * i)) & 0xFF for i in range(8)]  # little-endian, like the ESP32
    k = p[4] ^ ((ic >> 8) & 0xFF)
    p[5] = (~p[5]) & 0xFF
    p[4] = (~p[2]) & 0xFF
    p[2] = (~p[0]) & 0xFF
    p[0] = k
    k = p[3] ^ (ic & 0xFF)
    p[3] = (~p[1]) & 0xFF
    p[1] = k
    for y in range(2):
        k = (SBOX[p[0] >> 3] + 0x25) & 0xFF
        _magic_xor(p, k); p[5] &= 0x0f; p[0] ^= k & 0x7
        k = SBOX[p[0] & 0x1f]
        _magic_xor(p, k); p[5] &= 0x0f; p[0] ^= k & 0xe0
        if y == 0:
            p[0], p[1] = p[1], p[0]
    out = 0
    for i in range(8):
        out |= (p[i] & 0xFF) << (8 * i)
    return out


def _pairs_to_bits(chunk):
    bits = []
    for i in range(0, len(chunk), 2):
        pair = chunk[i:i + 2]
        if pair == "10":   bits.append(1)   # HIGH long, LOW short  -> 1
        elif pair == "01": bits.append(0)   # HIGH short, LOW long  -> 0
        else:              return None
    return bits


def decode_pulse(pb):
    # frame = "11" start marks + 104 manchester chars (52 data bits) + stop; some
    # captures drop a leading edge, so scan a few offsets and take the valid one.
    for off in range(0, 6):
        chunk = pb[off:off + 104]
        if len(chunk) < 104:
            continue
        bits = _pairs_to_bits(chunk)
        if not bits or len(bits) != 52:
            continue
        data = 0
        for b in bits:
            data = (data << 1) | b
        P0 = (data >> 48) & 0xF
        if P0 not in (1, 2, 4, 8):          # valid positional button code?
            continue
        P1 = (data >> 44) & 0xF
        dec = decrypt(data)
        return dict(off=off, serial=(dec >> 16) & 0x0FFFFFFF,
                    counter=dec & 0xFFFF, btn=P0, parcel_n=(0xF ^ P0 ^ P1) & 0xF)
    return None


def main(paths):
    for fn in paths:
        print(f"=== {fn} ===")
        for i, e in enumerate(json.load(open(fn))):
            r = decode_pulse(e["pulse_binary"])
            if r:
                print(f"  #{i} serial=0x{r['serial']:07X} counter={r['counter']:5d} "
                      f"btn=0x{r['btn']:X} parcel_n={r['parcel_n']}")
            else:
                print(f"  #{i} DECODE FAILED")
        print()


if __name__ == "__main__":
    args = sys.argv[1:] or ["../tmp/mg.json", "../tmp/vers.json"]
    main(args)
