# Usage — enrolling & operating the ESP32 as a Nice Flor-S remote

Operator runbook for the serial commands. Commands are single characters typed into the
serial monitor (`make monitor`, 115200 baud). The ESP32's own serial is **0xB2511C9**.

## Command quick reference
| Command | What it does |
|---|---|
| `m` | open **main** gate (NEW serial, btn 0x1) — daily use |
| `s` | open **side** gate (NEW serial, btn 0x2) — daily use |
| `z` | 5-second **hold** of the NEW serial (enroll Step 1, standalone) |
| `e` | **Method A** — auto enroll (ESP32 sends the old serial itself) |
| `B` | **Method B** — fob-assisted enroll (you press the physical fob ×3) |

> Don't reuse `f l o 1-6 h H c - + d` — those are the fan remote / debug keys.

## Before anything
- Be **within RF range of the gate** (the receiver must hear the ESP32 to learn).
- ESP32 flashed; `make monitor` open. Boot dump lists the new remote at `next_counter=1`.
- During enroll, watch the receiver's red LED: **3 slow flashes = learned OK.**

---

## Daily use (after enrollment)
- `m` → open **main** gate    `s` → open **side** gate

Never use the old serial for daily opens — it desyncs your physical fob.

---

## METHOD A — auto, no physical remote  (command: `e`)
The ESP32 plays both roles.

1. Stand in range of the gate.
2. Type `e`. The ESP32 runs automatically:
   - Step 1: holds the NEW code ~6 s
   - Step 2: sends the OLD code (0x347A033) ×3
   - Step 3: sends the NEW code once to confirm
3. Watch the receiver LED → **3 slow flashes = done.**
4. Test: `m` opens main, `s` opens side.

⚠️ Method A can fail if the physical fob has been used a lot since the last capture. The
receiver only accepts a counter **above its high-water mark**, and the fob keeps pushing that
mark up. If Step 2 is rejected → use **Method B**, or re-sniff the fob
(`pulse_configs/nice_flor_s_decode.py`) and raise `NICE_AUTH_SEED` in `main/tx.hpp` above the
fob's current counter, then re-flash.

---

## METHOD B — hold-5s + physical fob  (command: `B`, or manual `z`→fob→`m`)
Most reliable — the fob sends its own next counter, always accepted.

**Guided (`B`):**
1. Stand in range, physical old fob in hand.
2. Type `B`. The ESP32 holds the NEW code ~6 s, then prints:
   `Now press your OLD fob 3x, then send any key to confirm...`
3. Press the **physical old fob 3×** (slowly), then send any key in the monitor.
4. The ESP32 sends the NEW code once. LED → **3 flashes = done.** Test `m`/`s`.

**Manual (building blocks):**
1. Type `z` (5 s hold of NEW).  2. Press the physical fob **3×**.  3. Type `m` (confirm).
4. LED → 3 flashes. Test `m`/`s`.

---

## If BOTH RF methods fail (no 3 flashes)
The receiver likely has OTA/remote learning locked out. Enroll the **same** serial once at
the receiver card — no reflash:

1. Open the receiver housing; find the learn button + LED (PDF Fig. 8/10).
2. **Fast way:** hold the receiver button (LED on) → send `m` from the ESP32 until the LED
   goes off → release.
   **Normal way:** momentary press the receiver button (LED on ~5 s) → send `m` → wait 1 s →
   send `m` again → LED flashes 3×.
3. Test `m`/`s`.

---

## Always verify (regression)
1. Disable/unplug the ESP32, press the **physical fob** → it must still open the gate (its
   counter is intact). Method B guarantees this; after Method A, double-check.
2. Power-cycle the ESP32 → `m`/`s` still work (counter persisted in NVS).
