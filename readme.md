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

## Special considerations

Using the example above, that alone would not have worked. We did not include the gap(us) between each frame.

If we were to expand FRAME_GAP_US to 50000us(50ms), we would see that the pulse would look like this instead

```bash
Pulses(239): 1178, 351, 385, 1085, 396, 1091, 373, 1093, 1111, 367, 391, 1095, 383, 1076, 388, 1108, 1095, 364, 1114, 368, 1090, 382, 1100, 380, 1109, 365, 365, 1096, 1109, 374, 388, 1091, 1094, 389, 385, 1077, 1104, 374, 394, 1093, 388, 1087, 1099, 379, 373, 1094, 1110, 367, 1116, 362, 384, 1099, 1116, 350, 394, 1093, 1110, 366, 369, '6651', 1120, 366, 389, 1080, 377, 1114, 368, 1094, 1112, 369, 374, 1091, 395, 1094, 384, 1085, 1096, 382, 1100, 388, 1088, 368, 1097, 382, 1101, 388, 361, 1107, 1099, 378, 372, 1119, 1083, 369, 393, 1096, 1086, 370, 403, 1091, 369, 1119, 1088, 369, 371, 1115, 1098, 389, 1089, 373, 395, 1093, 1085, 392, 366, 1096, 1109, 373, 380, '6643', 1114, 379, 372, 1095, 387, 1087, 371, 1116, 1099, 380, 359, 1109, 374, 1091, 394, 1094, 1091, 394, 1096, 358, 1101, 389, 1087, 368, 1121, 358, 400, 1095, 1084, 369, 393, 1095, 1112, 370, 374, 1092, 1094, 390, 384, 1104, 375, 1092, 1095, 389, 358, 1101, 1117, 369, 1094, 389, 384, 1079, 1104, 373, 394, 1091, 1092, 368, 396, '6646', 1118, 363, 380, 1099, 382, 1091, 368, 1098, 1108, 373, 382, 1091, 394, 1072, 383, 1100, 1104, 373, 1119, 365, 1087, 371, 1104, 375, 1119, 364, 386, 1080, 1104, 374, 393, 1093, 1111, 367, 389, 1090, 1110, 345, 388, 1117, 370, 1093, 1112, 366, 370, 1096, 1108, 373, 1114, 378, 372, 1094, 1117, 367, 364, 1115, 1085, 370, 388)
```

In this case, the repeat is 4, and in between each repeat occurence, there is a delay(gap) of about `6650us`.

After taking into account this gap, the remote worked flawlessly.

```
59pulses * 4repeats = 236pulses
236pulses + 3gaps = 239pulses
```
