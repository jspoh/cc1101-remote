# Nice Flor-S: clone vs. "add a new remote"

Working notes on adding a remote to a Nice Flor-S gate, why we must **add** (not clone),
why Flipper Zero never touches the receiver, and what our sniffed captures prove.

## Decision: ADD a new serial, never clone

**Cloning is a no-go.** A clone reuses the physical fob's serial `0x347A033` *and* shares its
rolling counter. Every ESP32 transmission advances the receiver's expected counter for that
serial. The moment the ESP32 is unplugged, the physical remote is left *behind* the receiver's
counter window → the receiver rejects it as a replay and **the real remote stops working**.

**Adding fixes this.** If the ESP32 has its **own** serial, it has its **own** counter. The
physical fob and the ESP32 never share counter state, so either can be used or removed without
breaking the other. This is mandatory, not a nicety.

## Two different operations

| | Clone (rejected) | Add / enroll (required) |
| --- | --- | --- |
| What it is | Transmit an existing authorised serial with a rolling counter | Get a **brand-new** serial into the receiver's authorised list |
| Counter | **Shared** with the physical fob → breaks it when ESP32 leaves | **Independent** → fob and ESP32 coexist |
| Needs the receiver? | No | **Yes — receiver must be in range to learn the new serial** |

## What Flipper Zero actually does

Flipper's `nice_flor_s.c` is **transmit-only** — it builds the 16-parcel frame and sends it,
auto-incrementing the counter (`nice_flor_s.c:173-266`). No enrol/learn/pairing/memorise logic
exists. Flipper has no "clone mode" vs "add mode"; it just synthesises RF. Enrollment is the
**receiver** reacting to transmitted signals, not anything Flipper does.

## How a new serial legitimately gets authorised (official Nice manual)

`flor-s-programming-instructions.pdf` gives three ways. **All require the receiver present and
powered** — it is the receiver that stores the new code:

1. **Fast way** — press the button *on the receiver card*, then transmit the new remote.
2. **Normal way** — momentary press on the receiver card (LED on 5 s), transmit + confirm.
3. **Remotely** — *no physical access to the receiver box*, but the receiver must be in RF
   range. Using an already-authorised **OLD** remote to bless a **NEW** one:
   1. Transmit **NEW** for ≥5 s (hold),
   2. then transmit **OLD** 3× (three presses),
   3. transmit **NEW** once to confirm. Receiver LED flashes 3× = success.

Method 3 is what we use: the ESP32 already knows the authorised serial `0x347A033`, so it can
act as **OLD** and authorise its own **NEW** serial — all by transmitting, while in range of the
gate. A security lockout ("2nd learning disabling function", PDF p.2/6) can disable method 3.

## The vendor puzzle (resolved)

The vendor added remotes **at his shop, with the gate/receiver at our home** — receiver never in
range. Since adding requires the receiver to store the new serial, he could **not** have added.
He either **cloned** `0x347A033` into new fobs, or loaded serials our gate was **pre-programmed**
with at install. Either way it is not the path we want (cloning breaks the fob; pre-programmed
serials we don't know). We do our own **method-3 add**, in range of the gate.

## Counter model (why Method A is fragile)

The receiver keeps **one number per serial**: the highest counter it has accepted so far — a
**high-water mark** `R`. For an incoming frame with counter `C`:

- `C <= R`  → **rejected** (replay / "too old")
- `R < C <= R + window`  → **accepted**, and `R` moves up to `C` (window ≈ a few hundred–~1000)
- `C` far beyond the window → needs **two consecutive** valid codes to resync

It is a high-water mark, **not** a list of used values: anything at or below `R` is dead, even
counters never actually sent. If `R` jumps 100→500, then 101–499 are all rejected too.

Consequence for enrollment Step 2 (the OLD/authoriser presses): they must land **above the
fob's current `R`** and within the window. Because the **physical fob** drives `R` every time
it's pressed, a fixed stored old-counter goes stale. That's why **Method A** (ESP32 sends the
old serial from a fixed seed) can fail, while **Method B** (physical fob sends its own next
counter, always exactly `R+1`) never does.

## What our sniffed data proves

Decoding every capture in the repo with the project's own cipher
(`pulse_configs/nice_flor_s_decode.py`, mirrors `niceFlorSDecrypt` + `NICE_FLOR_S_SBOX`):

| File | Button (gate) | Serial | Counters |
| --- | --- | --- | --- |
| `tmp/vers.json` (7 presses) | 0x2 (side gate) | **0x347A033** | 45762–45768 |
| `tmp/mg.json` (6 presses) | 0x1 (main gate) | **0x347A033** | 45769–45774 |

Counters run **contiguously 45762 → 45774 across both files**, so this is **one physical remote,
two buttons** (main + side), pressed 13 times — not two remotes. Decode is self-validating:
one serial, monotonic counter, valid button/parcel structure on all 13 frames. `0x347A033` is
the **only** serial recorded anywhere in the repo (`alyssa_gate.json`, `main/tx.hpp`).

## Status: IMPLEMENTED

The ESP32 now has its **own** serial `0xB2511C9` (random 28-bit, ≠ `0x347A033`) in
`NICE_TX_CONFIG` and both enrollment methods are coded. See `plan.txt` for the build map,
`usage.md` for the operator runbook, and `HANDOFF.md` for where we left off.

Serial commands (in `main/main.ino` loop dispatch):

| Cmd | Action |
| --- | --- |
| `m` / `s` | daily open — main / side gate, NEW serial `0xB2511C9`, own NVS counter |
| `z` | 5-second hold of the NEW serial (enroll Step 1, standalone) |
| `e` | **Method A** — auto enroll: ESP32 sends NEW hold → OLD `0x347A033` ×3 → NEW once |
| `B` | **Method B** — fob-assisted: ESP32 holds NEW, you press the physical fob ×3, then NEW once |

Key properties:
- Daily operation uses **only** the NEW serial + its own NVS counter — never `0x347A033`.
- Method A transmits `0x347A033` only during the one-time enroll (3 presses); its seed
  (`NICE_AUTH_SEED = 45777`) must be above the fob's current high-water mark or Step 2 is
  rejected (re-sniff the fob and bump it, or use Method B).
- Must be run **within RF range of the gate**; both RF methods need OTA learning not locked out.
- "Pressing any key" authorises the **serial**, so one enroll should make both `m` (0x1) and
  `s` (0x2) work; if the receiver learns per-button, enroll each once.
