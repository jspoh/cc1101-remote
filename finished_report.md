# Finished Report — Side Gate Working (session of 2026-09-24)

Scope: **this session only.** The main gate (`m`) was already working coming in; the goal
this session was to get the **side gate (`s`)** opening from the ESP32, plus fix a serious
radio bug discovered along the way. Both are done and confirmed on hardware.

## Outcome
- ✅ **Side gate now opens** via `s` (new serial `b2511c9`, btn `0x2`).
- ✅ **Side gate enrolled** via Method A (`E`) once three things lined up (below).
- ✅ **Radio no longer jams** the physical fob / wifi (was transmitting 24/7 in `TX_ONLY`).
- ✅ **Daily use never touches the old serial** — `m`/`s` only send `b2511c9`; `0x347A033`
  is used *only* inside Method A enrollment.

## What actually got the side gate working
The side gate had been dead-silent on `s`. The fix was a combination, not one thing:

1. **Stopped the radio from jamming (root cause).** In `TX_ONLY` the CC1101 was left keyed
   in TX permanently (boot did `STX`/`SetTx`, and `txRadioBegin/End` were compiled out under
   `#ifndef TX_ONLY`). A CC1101 parked in TX leaks on 433 MHz and desensitised the nearby
   receiver — which is why even the *physical fob* went intermittent while the board was
   powered. Discovered by: unplug the ESP32 → fob + wifi immediately recovered.
   - Fix: `setup()` now strobes `SIDLE` at boot; every transmit path calls `txRadioBegin()`
     (enter TX) then `txRadioEnd()` (→ `SIDLE`) in `TX_ONLY` too. Radio is now idle except
     for the few ms of an actual send.

2. **Kept the Method A authoriser counter ahead of the fob.** Method A step 2 replays the
   old serial `0x347A033` as authoriser; the receiver rejects any counter ≤ its high-water
   mark. The fob kept climbing (last sniffed at **46009**), so a stale seed got rejected.
   - Fix: `niceCounterNext` now uses `max(seed, stored_flash)`, and `NICE_AUTH_SEED` was
     bumped above the fob's current counter before each attempt.

3. **Doubled the enroll presses for link margin.** With a marginal link, one press per step
   wasn't reliably heard. Method A's step-2 and step-3 presses were changed from 1 burst to
   `bursts = 2` (2 × 4 parcels = **8 parcels per press**). Daily `m`/`s` stay at 4.

With the board no longer jamming, the authoriser counter above 46009, and 8-parcel presses,
running **`E`** enrolled the side gate and `s` now opens it.

## Code changes made this session
- **`main/nice_flor_s.hpp`** — `NFS_PARCELS` 16 → **4** (one press ~1.6 s → ~400 ms of
  airtime; less loop-blocking, faster repeats).
- **`main/tx.cpp`**
  - `niceCounterNext` — `max(seed, flash)` so a bumped seed can't be overridden by a stale
    stored value.
  - `txRadioEnd` — strobes `SIDLE` in `TX_ONLY`; `txRadioBegin/End` calls now run in
    `TX_ONLY` too (radio idles between sends).
  - `niceEnrollAuto` — step-2 and step-3 presses use `bursts = 2` (8 parcels).
  - `niceEnrollFob` — drains the serial buffer before waiting, so the leftover newline after
    the `B` command no longer skips the "press any key" pause.
- **`main/main.ino`**
  - `setup()` (`TX_ONLY`) — idle (`SIDLE`) at boot instead of parking in TX.
  - Enrollment dispatch: `E` = Method A **side** gate; `B` = Method B **side** gate
    (`e` = Method A main; `z` = 5 s hold main — unchanged).
- **`main/tx.hpp`** — `NICE_AUTH_SEED` raised above the fob's high-water mark.
- **`remote_templates/gate-remote.html`** — hold buttons: after the `HOLD_MS` debounce the
  first press fires once, then while still held it repeats (`FIRST_REPEAT_MS` before the 2nd,
  `REPEAT_MS` thereafter) to help a weak link land. Sim/vibrate only on the first press.

## Command reference (current)
| Cmd | Action |
|-----|--------|
| `m` / `s` | open main / side gate (new serial `b2511c9`) — daily use |
| `z` | 5 s hold of the new serial (main) |
| `e` / `E` | Method A auto-enroll — main / **side** |
| `B` | Method B fob-assisted enroll — side |

## Confirmed / not
- Confirmed on hardware: `s` opens the side gate; fob + wifi fine with the board powered.
- The old serial `0x347A033` is referenced only in `niceEnrollAuto` — verified by grep; no
  daily path uses it.
- Cloning the old serial for daily use was rejected (would desync the fob and require an
  endless counter chase) — the ADD-a-new-serial approach stands.
