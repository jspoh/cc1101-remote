# CC1101 PIN LIST

E07-M1101D

| Pin | Pin Name | Pin Type | Purpose |
| --- | --- | --- | --- |
| 1 | GND | Power | - |
| 2 | VCC | Power | 3.3V |
| 3 | GDO0(General Display Output 0) | Output | Hardware interrupt on new packet arrival / end TX |
| 4 | CSN(Chip Select Not) | Input | Wake switch - Set pin to 0V on TX(ask part to listen to ESP32 instead of the air) |
| 5 | SCK(Serial ClocK) | Input | SPI Clock |
| 6 | MOSI(Master Out Slave In) | Output | Data line from ESP32 to CC1101 for TX |
| 7 | MISO(Master In Slave Out) | Input | Data line from CC1101 from CC1101 to ESP32 for RX |
| 8 | GD02 | Output | Extra interrupt pin |


## Connection config

| CC1101 Pin | ESP32C3 Pin | Remarks |
| --- | --- | --- |
| 1 | GND | |
| 2 | 3V3 | |
| 3 | GPIO0 | Not optional for remote copy use case |
| 4 | GPIO1 | |
| 5 | GPIO2 | |
| 6 | GPIO3 | |
| 7 | GPIO4 | |
| 8 | GPIO9 | Optional, not connected | 

CC1101 Pin 8 is NOT REQUIRED, and do not use GPIO9. Will face issues uploading code.


## Understanding pulses

Most remotes use `OOK` - `On Off Keying` to communicate. 

Meaning they generally send pulses in a binary state, using the short (350us) or long (1050us) pulses to communicate.

Eg. (livingroom_fan.pulse > Light On/Off)

```
Raw: 
Pulses(59): 1117, 369, 377, 1088, 394, 1094, 385, 1085, 1098, 380, 372, 1096, 391, 1084, 394, 1101, 1080, 378, 1110, 368, 1094, 386, 1107, 374, 1084, 395, 368, 1095, 1111, 371, 373, 1092, 1095, 384, 385, 1105, 1076, 401, 375, 1088, 388, 1088, 1097, 387, 367, 1095, 1116, 368, 1094, 387, 382, 1100, 1106, 371, 369, 1094, 1112, 370, 372)

RF signal:
----------___---__________---__________ ...and so on

Binary:
1000 1000 1111 ...and so on
where 1 is (1050, 350) and 0 is (350, 1050)
We can also express as 1001 0101 and use this to check. if `00` or `11` is present, we know the packet is malformed.
```

But if we were to do a copy Tx, we need to clean it up, and use only 350 or 1050 values
