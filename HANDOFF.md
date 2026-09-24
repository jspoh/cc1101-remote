# HANDOFF — Nice Flor-S "add a new remote" (2026-09-24)

Where we left off, for the next agent/session. Read these in order:
- **[nice_flor_s.md](nice_flor_s.md)** — why ADD not clone, protocol, counter model, decoded evidence.
- **[plan.txt](plan.txt)** — build map + exactly what was implemented + verification/risks.
- **[usage.md](usage.md)** — operator runbook (the serial commands, both enroll methods).
- **[pulse_configs/nice_flor_s_decode.py](pulse_configs/nice_flor_s_decode.py)** — decode any capture to serial/counter/button.

## State: IMPLEMENTED, NOT YET TESTED ON HARDWARE
`arduino-cli` isn't in the dev sandbox, so the firmware is **not compiled/flashed/verified**.
The owner must `make build && make upload && make monitor` on their machine.

## The decision (settled with the owner)
The ESP32 must be its OWN remote (new serial `0xB2511C9`, own rolling counter). Cloning
`0x347A033` is rejected: shared counter breaks the physical fob when the ESP32 leaves. Full
reasoning in nice_flor_s.md. `0x347A033` is the only real serial in the repo; `tmp/mg.json`
(main) + `tmp/vers.json` (side) are that one fob, two buttons.

## Code changed (firmware)
- `main/tx.hpp`: `NICE_TX_CONFIG` = new serial only (`m,b2511c9,1,1|s,b2511c9,1,2|`, old clone
  entries removed); added `NICE_AUTH_SERIAL/NICE_AUTH_SEED` (Method A authoriser); decls for
  `txNiceFlorSHold`, `niceEnrollAuto`, `niceEnrollFob`.
- `main/tx.cpp`: implemented those three (Step-1 hold, Method A, Method B). Reuses
  `niceFlorSBuildUpload`, `txNiceFlorS`, `niceCounterNext`, `txRadioBegin/End`.
- `main/main.ino`: loop() dispatch for `z` (hold), `e` (Method A), `B` (Method B). `m`/`s`
  daily opens already handled by the existing generic `NICE_REMOTES` loop.
- `nice_flor_s.*` (the encoder/cipher): UNCHANGED.

## Commands
`m`/`s` = open main/side (new serial) · `z` = 5s hold · `e` = Method A (auto) · `B` = Method B (fob).

## Next steps (owner, at the gate)
1. `make build` (needs esp32 core + ELECHOUSE_CC1101 lib). Fix any compile issue — code was
   written without a local compiler.
2. `make upload && make monitor`; confirm boot lists the new remote at `next_counter=1`.
3. Enroll in RF range: try `e` first; if Step 2 is rejected or no 3-flash, use `B` (press fob
   ×3). See usage.md.
4. Verify `m`/`s` open the gate; then the CRITICAL regression: unplug ESP32 → physical fob
   still opens; power-cycle ESP32 → `m`/`s` still work.

## Open questions / risks (unverifiable from here)
- **OTA learning may be locked out** ("2nd learning disable"): both RF methods would fail →
  fallback is enrolling the same serial once at the receiver card (usage.md).
- **Method A counter staleness**: `NICE_AUTH_SEED=45777` must exceed the fob's current
  high-water mark, else Step 2 is rejected. Re-sniff the fob and bump it, or use Method B.
- **Per-button vs per-serial learning**: one enroll should cover both `m` and `s`; if not,
  enroll each button once.

## Not done / possible follow-ups
- No web-UI enroll button (only the `/tx?cmd=` opens exist in `main/wifi.cpp`); could mirror
  `e`/`B` there later.
- `pulse_configs/new_nice_remote.py` was considered but NOT created — the new serial was
  generated once and hard-coded into `NICE_TX_CONFIG`. Add a generator only if you need more.
