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
| 3 | GPIO0 | |
| 4 | GPIO1 | |
| 5 | GPIO2 | |
| 6 | GPIO3 | |
| 7 | GPIO4 | |
| 8 | GPIO9 | | 

CC1101 Pin 8 is NOT REQUIRED, and do not use GPIO9. Will face issues uploading code.
