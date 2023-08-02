# How to handle hardware interrutps.
Hardware interrupts are generated externally by a different chipset the Programmable Interrupt Controller (PIC).
## The Programmable Interrupt Controller (PIC)
The PIC consits out of 2 chips. One Slave and one Master. Each Chip has 8 Input signals called IRQ (Interrupt Request). On each interrupt line a Hardware component can send a interrupt request to the chip, and the chip sends a interrupt request to the CPU but orderd by importance. 

The order starts from IRQ 0 and goes to  IRQ 7. IRQ 0 is the moste important and IRQ 8 is the least important. (The keyboards sends on IRQ 1)

The slave chip is connectet to the master chip via **IRQ2**. So we have a total of 15 Input lines.

| Slave | 
| ------ |
| IRQ0 |
| IRQ1 |
| **IRQ2** |
| IRQ3 |
| IRQ4 |
| IRQ5 |
| IRQ6 |
| IRQ7 |

| Master | 
| ------ |
| IRQ0 |
| IRQ1 |
| **IRQ2** |
| IRQ3 |
| IRQ4 |
| IRQ5 |
| IRQ6 |
| IRQ7 |


Each chip has a command port and a data port.

| Chip | I/O Address |
| ------ | ------ |
| Master - Command | 0x0020 |
| Master - Data | 0x0021 |
| Slave - Command | 0x00a0 |
| Slave - Data | 0x00a1 |


## The Programmable Interval Timer (PIT)
The PIT is a external chip with a oscillator, prescaler and 3 independent frequency divider.
On this 3 frequency divider outputs (Channels) we can controll external circuits.
- Channel 0: Directly connectet IRQ0. Use it for purpose to generate IRQ0 inputs. **0x40 I/O address**
- Channel 1: Not in use probably didnt exists anymore. **0x41 I/O address**
- Channel 1: Connected to PC speaker.Frequency of output determines the frequency of the sound. **0x42 I/O address**


At address **0x43** Mode/Command register. A 8 bit value for the settings of the PIT (write only).


Content of the Mode/Command register
```
 7           6      5      4       3      2      1    0
| Selected Channel | Access Mode | Operating mode |BCD|
```

**BCD:** 0 = 16-bit binary mode, 1 = four digit BCD mode
**Operating mode:**
- 0 0 0 = Mode 0 (Interrupt on terminal count)
- 0 0 1 = Mode 1 (hardware re-triggable on one-shot)
- 0 1 0 = Mode 2 (rate generator)
- 0 1 1 = Mode 3 (square wave generator)
- 1 0 0 = Mode 4 (software triggered strobe)
- 1 0 1 = Mode 5 (hardware triggered strobe)
- 1 1 0 = Mode 6 (rate generator (same as Mode 2))
- 1 1 1 = Mode 7 (square wave genarator (same as Mode 3))

**Access mode:**
- 0 0 = Latch count value command
- 0 1 = Access Mode: lo-byte only
- 1 0 = Access Mode: hi-byte only
- 1 1 = Access Mode: hi-byte/lo-byte

**Selected Channel**: Channel 0, Channel 1, Channel 2, or Read-back command





