import json 
import os


class TX_CONFIG:
  def __init__(self, trigger="", lpu=0,spu=0,pgu=0,pb="") -> None:
    self.trigger = trigger
    self.long_pulse_us = lpu
    self.short_pulse_us = spu
    self.pulse_gap_us = pgu
    self.pulse_binary = pb
    
  def __str__(self) -> str:
    return f"{self.trigger},{self.long_pulse_us},{self.short_pulse_us},{self.pulse_gap_us},{self.pulse_binary}|"


ls = os.listdir()
json_files = []
for file in ls:
  # print(file[-6:])
  if file[-5:] == ".json" or file[-11:] == ".json.secret":
    json_files.append(file)
    
print("Processing:\n" + "\n".join(json_files) + "\n")


for file in json_files:
  print("Starting", file)
  out = ""
  with open(file, "r") as f:
    data = json.load(f)
  # print(data)
  
  for keybind, config in data.items():
    tc = TX_CONFIG(keybind, config["long_pulse_us"], config["short_pulse_us"], config["pulse_gap_us"], config["pulse_binary"])
    out += str(tc)   
    
  out_path = file.replace(".json", ".pulse")
  with open(out_path, "w") as f:
    f.write(out)
    
  print(file, "done:", out_path)
  

# print(out)

