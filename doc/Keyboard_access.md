# Keyboard access
- Keyboard access is interrupt driven
- Each process has its own keyboard buffer
- Keyboard buffers can be pushed and popped from
- Keyboard parses scancodes which we must parse to ascii or utf

*We rember the ps2 keyboard sends interrupts at [IRQ1 vector](STCQ/HardwareInterrupts.md#Default_ISA_IRQ`s)*

## Keyboard access is interrupt driven
Each time a key is pressed on the keyboard a interrupt is raised and the IDT searches for our interrupt handler. If we mapped our PIC start to int 0x20 (which is standard) then the ps2 keyboard is int 0x21. We need to read the scancode and send back a acknowledgement to the PIC.