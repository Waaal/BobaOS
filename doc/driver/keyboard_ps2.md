# PS2 Keyboard
The PS2 keyboard is interrupt driven and is connected via the PIC at IRQ 1. [See here](STCQ/HardwareInterrupts.md#default-isa-irqs)* 
Each time a key is pressed a interrupt is send via IRQ 1.
The PS2 is a extremly simple protocoll by sending scancodes to


Important PS2 IO Ports
| IO Port | Type | Purpose |
| ------ | ------ | ------ |
| 0x60 | Read/Write | Data |
| 0x64 | Read | Status |
| 0x64 | Write | Command |


**Data:** Over this port we read and write data to the PS2 Keyboard or controller.


**Status:**


**Command:** For sending commands to the PS2 Controller. A important command is the 0xAE which enables the PS2 Keyboard.
