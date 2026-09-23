```
11
01010110
10011010
01011001010110101001011001100110101010101001100101101001
01100101010101101010011001011001  > 
1     > end pulse
```

sync: 110101011010


01 -> 0
10 -> 1

| ID | Description | My bits | Actual bits | Decimal |
| --- | --- | --- | --- | --- |
| SYNC | SYNC | 11 | - | - |
| N0 | button ID | 01 01 01 10 | 0001 | 1 |
| N1 | Retransmission counter | 10 01 10 10 | 1011 | 11 |
| N2 - N12 | 28b Serial + 16b rolling code | NA | NA | NA