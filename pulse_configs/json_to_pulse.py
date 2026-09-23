"""
Converts remote .json files into the compact strings the firmware compiles in.

Two remote kinds are supported, chosen per-entry by an optional "type" field:

  fixed-code (default)  -> RAW_TX_CONFIG  (raw long/short OOK, replayed as-is)
      { "long_pulse_us", "short_pulse_us", "pulse_gap_us", "pulse_binary" }
      emitted as:  trigger,long,short,gap,pulse_binary|   -> <name>.pulse

  "type": "nice_flor_s" -> NICE_TX_CONFIG (rolling code, synthesised each press)
      { "serial", "counter", "btncode" }
      emitted as:  trigger,serial_hex,counter,btncode_hex|  -> <name>.nice

Paste .pulse contents into RAW_TX_CONFIG and .nice contents into NICE_TX_CONFIG
in main/tx.hpp.
"""

import json
import os


def fixed_entry(trigger, cfg):
    return f'{trigger},{cfg["long_pulse_us"]},{cfg["short_pulse_us"]},{cfg["pulse_gap_us"]},{cfg["pulse_binary"]}|'


def nice_entry(trigger, cfg):
    # int(..., 0) accepts "0x1c" or plain decimal; serial/btncode re-emitted as hex
    serial = int(str(cfg["serial"]), 0)
    counter = int(str(cfg["counter"]), 0)
    btncode = int(str(cfg["btncode"]), 0)
    return f"{trigger},{serial:07x},{counter},{btncode:x}|"


ls = os.listdir()
json_files = [f for f in ls if f.endswith(".json") or f.endswith(".json.secret")]

print("Processing:\n" + "\n".join(json_files) + "\n")

for file in json_files:
    print("Starting", file)
    with open(file, "r") as f:
        data = json.load(f)

    fixed_out = ""
    nice_out = ""
    for keybind, cfg in data.items():
        if cfg.get("type") == "nice_flor_s":
            nice_out += nice_entry(keybind, cfg)
        else:
            fixed_out += fixed_entry(keybind, cfg)

    base = file.replace(".json.secret", "").replace(".json", "")
    if fixed_out:
        with open(base + ".pulse", "w") as f:
            f.write(fixed_out)
        print(f"  {base}.pulse  -> paste into RAW_TX_CONFIG")
    if nice_out:
        with open(base + ".nice", "w") as f:
            f.write(nice_out)
        print(f"  {base}.nice   -> paste into NICE_TX_CONFIG")
        print(f"    {nice_out}")

    print(file, "done")
