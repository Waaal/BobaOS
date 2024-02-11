# Programmable Interrupt Controller.
Hardware interrupts are generated externally by a different chipset the Programmable Interrupt Controller (PIC).
This chip allowes hardware to interrupt the processor. For example: if you press a key on the keyboard the PIC will invoke a interrupt in the CPU.

## PIC structure
The PIC consits out of 2 chips. One Slave and one Master. Each Chip has 8 Input signals called IRQ (Interrupt Request). On each interrupt line a Hardware component can send a interrupt request to the chip, and the chip sends a interrupt request to the CPU but the orderd by importance. 

The order starts from IRQ 0 and goes to  IRQ 7. IRQ 0 is the moste important and IRQ 8 is the least important.

The slave chip is connectet to the master chip via **IRQ2**. So we have a total of 15 Input lines.

Master handles IRQ 0 - 7.
Slave handles IRQ 8 - 15.

| Master | 
| ------ |
| IRQ0 |
| IRQ1 |
| **IRQ2 (Connection to slave)** |
| IRQ3 |
| IRQ4 |
| IRQ5 |
| IRQ6 |
| IRQ7 |

| Slave | 
| ------ |
| IRQ8 |
| IRQ9 |
| IRQ10|
| IRQ11 |
| IRQ12 |
| IRQ13 |
| IRQ14 |
| IRQ15 |


The IRQ 0 number can be remapped to a Interrupt vector. The others than will follow with the next vector. Example: We map IRQ 0 to interrupt vector 40. IRQ 1 will have 41. IRQ 2 will have 42 and so on.

*Note: By default the IRQs are mapped to interrupt vectors 8 - 15. This is a problem because interrupt vectors 0 - 31 are used by the CPU exceptions, so we need to remap them*

#### Default ISA IRQ`s

| IRQ | Description |
| ------ | ------ |
| 0 | Timer (PIT) |
| 1 | Keyboard |
| 2 | Connection to slave (never used) |
| 3 | COM2 (if enabled) |
| 4 | COM1 (if enabled) |
| 5 | LPT2 (if enabled) |
| 6| Floppy Disk |
| 7 | LPT1 |
| 8 | CMOS real time clock (if enabled) |
| 9 | Free for peripherals |
| 10 | Free for peripherals |
| 11 | Free for peripherals |
| 12 | PS2 Mouse |
| 13 | Intel-Processor |
| 14 | Primary ATA Hard Disk |
| 15 | Secondary ATA Hard Disk |

When a interrupt is fired, the IRQ channel gets masked. So the Chip nows, that he cannot use this channel, till the mask bit is 0.

Each chip has a command register port and a data register port.

| Chip | I/O Address |
| ------ | ------ |
| Master - Command | 0x0020 |
| Master - Data | 0x0021 |
| Slave - Command | 0x00a0 |
| Slave - Data | 0x00a1 |

### Init the PIC
In order for a PIC to work we have to send 2-4 init Initialication Command Word (ICW). The actual number we have to send depends on the configuration. But ICW1 and ICW2 are needed for this chip to work.

The order of the commands we have to send are fix. They cannot be send again.

There are also operating command words (OCW). They can be send during the operating of the chip. These commands are not fix and not needed to work.

#### Sending ICWs
As mention the order of this commands are fix.

**ICW_1:**
First Init command.
**Need to be written to the command register. (If slave exists also to the slave command register)**
```
  7   6   5   4   3   2   1   0
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
```
- 0: 1 = ICW_4 is needed. 0 = ICW_4 is not needed.
- 1: 1 = Single Mode. 0 = Cascade Mode (Has a slave).
- 2: CALL address inerval 1 = Interval of 4. 0 = Interval of 8.
- 3: 1 = Level Trigger mode. 0 = Edge Trigger mode
- 4: 1 = This is first init command followed by at least 1 more init commands. 0 = Not first init command
- 5: Not needed in ICW_1
- 6: Not needed in ICW_1
- 7: Not needed in ICW_1



**ICW_2:**
Specify the starting Interrupt vector number of the first IRQ. (Vector number = Interrupt. Speciefied in IDC table). 
**Need to be written to to data register (Master and slave)**
We remeber first 32 vectors occupied by the CPU.
```
  7   6   5   4   3   2   1   0
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
```
This is now a whole number 8 bit value. So if our starting vector is 32 we write: 00100000 = 32 



**ICW_3**:
Only needed if we have set bit 1 in ICW_1 to cascade. This value speciefies at which IRQ the master and slave are connected. On a normal system this is IRQ2

**IMPORTANT: I think this can be ignored, because it is set by default to IRQ 2**

```
  7   6   5   4   3   2   1   0
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
```
**For Master:**

This is **not** a whole number 8 bit value. This is one register per bit.
So if the connection is on IRQ2 we write 4 which is 00000100 turns bit 2 to 1.

Bit 2 = IRQ2
We write it in the data register of the master


**For Slave:**

This is a whole number 8 bit value and is for identification. So the slave needs to know at what IRQ he is connected to the master. If it is 2 then we write 2 = 00000010.

We write in the data register of the slave.



**ICW_4:**
More init settings. Only used if we set in ICW_1 bit 0 to 1.
 **Need to be written to the data register. (If slave exists also to the slave data register)**
```
  7   6   5   4   3   2   1   0
| 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
```
- 0: 1 = x86 mode. 0 = MCS-80/85 mode.
- 1: 1 = Auto End Of Interrupt. 0 = Normal End Of Interrupt.
- 2: CALL address inerval 1 = Interval of 4. 0 = Interval of 8.
- 3: ?????
- 4: ?????
- 5: Not needed in ICW_4
- 6: Not needed in ICW_4
- 7: Not needed in ICW_4

## The Programmable Interval Timer (PIT)
The PIT is a external chip with a oscillator, prescaler and 3 independent frequency divider.
On this 3 frequency divider outputs (Channels) we can controll external circuits.
- Channel 0: Directly connectet IRQ0. Use it for purpose to generate IRQ0 inputs. **0x40 I/O address**
- Channel 1: Not in use probably didnt exists anymore. **0x41 I/O address**
- Channel 1: Connected to PC speaker.Frequency of output determines the frequency of the sound. **0x42 I/O address**

The PIT can be configured to frequently send interrupts to the CPU. This can then be use for a Task switching. For Example: We confire the PIC to send 100 interrupts per second to the CPU. The CPU uses this interrupt to switch Task 100 times per second.

The PIT has a frequency ~1.2Mh per channel.

At address **0x43** Mode/Command register. A 8 bit value for the settings of the PIT (write only).

**The PIT is connected to the PIC at IRQ 0.**

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

## Set up a PIT to send 100 interrupts per second (Assembly)
``` assembly
InitPIT:
    mov al, 00110100b     ; Selected Channel = 0
                          ; Access Mode = hibyte/lobyte
                          ; Operating Mode = rate generator
                          ; BCD = 16-bit Binary Mode

    out 0x43, al          ; Write with out to the I/O port of command register.
    mov ax, 11931         ; 100 times per second = 1193182/100 = 11931
    out 0x40, al          ; Write in channel 0. (8 bit value so we write the lobyte of 11931)
    mov al, ah            ; Write highbyte in al
    out 0x40, al          ; Write highbyte in channel 0
``` 
Now our PIT is configured, to send 100 interrupts per second to IRQ0 on master chip.

## Set up a PIC (Assembly)
``` assembly
    mov al, 0x11      ; ICW_1 = 00010001
    out 0x20, al      ; out to master command register 
    out 0xa0, al      ; out to slave command register
    
    mov al, 32        ; ICW_2 (Master)
    out 0x21, al      ; Out to master data register
    mov al, 40        ; ICW_2 (Slave)
    out 0xa1, al      ; Out ti slave data register

    mov al, 4         ; ICW_3 = 00000100 (Master)
    out 0x21, al      ; out to master data register
    mov al, 2         ; ICW_3 = 00000010 (Slave)
    out 0xa1, al      ; out to slave data register

    mov al, 1         ; ICW_4 = 00000001
    out 0x21, al      ; out to master data register
    out 0xa1, al      ; out to slave data register
```
**Explenation:**

**ICW_1:**
  - Bit 0: 1 because we need ICW_4
  - Bit 1: 0 because we are in cascade mode (we have a slave)
  - Bit 2: 0 because we set call interval to 8
  - Bit 3: 0 set level trigger mode to edge mode
  - Bit 4: 1 because this is the first init command 
  - Bit 5: 0 not in use 
  - Bit 6: 0 not in use
  - Bit 7: 0 not in use


**ICW_2 - Master:**

Set it to 32, because interrupt vector at IRQ0 starts at 32. That also means if we receive a interrupt request on IRQ0, we can catch it with a Interrupt handler for 32 in the IDT.


**ICW_2 - Slave:**

We set it to 40, because the interrupt vector at slave IRQ0 starts at 40.


**ICW_3 - Master:**

Set it to 4 because bit 2 is 1. And bit 2 means we have slave connected at IRQ2.


**ICW_3 - Slave:**

Set it to 2. Because this is now a number again and the slave is connected with the master at IRQ2.


**ICW_4:**
  - Bit 0: 1 because we have a x86 bit machine
  - Bit 1: 0 because we dont want auto interrupt
  - Bit 2: 0 because we set call interval to 8
  - Bit 3 and 4: 0 because we dont use a buffer
  - Bit 5: 0 not in use 
  - Bit 6: 0 not in use
  - Bit 7: 0 not in use


And thats how you set up a PIC.

We now have a timer, that sends 100 times a second to IRQ0 at master and the PIC sends it to the CPU. Dont forget to handle the interrupt at vector 32 with the IDT.

## Acknowledgment
The PIC needs acknowledgment for a interrupt so that it can send new interrupts on these lines. 
For example if the PIC sends an interrupt on IRQ 0 and it is mapped to vector 32. You need to acknowledge back that you have handled this interrupt. Otherwies the PIC wont send any new interrupts on vector 32.

### How to send a acknowledgment
[Missing]