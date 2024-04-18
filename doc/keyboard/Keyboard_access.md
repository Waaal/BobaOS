# Keyboard access
- Keyboard access is interrupt driven
- Each process has its own keyboard buffer
- Keyboard buffers can be pushed and popped from
- Keyboard parses scancodes which we must parse to ascii or utf

TODO: Move PS2 specific text to keyboard drvier ps2

*We rember the ps2 keyboard sends interrupts at [IRQ1 vector](STCQ/HardwareInterrupts.md#default-isa-irqs)*

## Keyboard access is interrupt driven
Each time a key is pressed on the keyboard a interrupt is raised and the IDT searches for our interrupt handler. If we mapped our PIC start to int 0x20 (which is standard) then the ps2 keyboard is int 0x21. We need to read the scancode and send back a acknowledgement to the PIC.

## Each process has its own keyboard buffer
We want that each process has its own keyboard buffer, because we want the process that is currently in the focus of the user to get the keystroke. We also can emit a event on a keystroke so a process can subscribe to this event, but the main process that gets the keystroke is the process in our focus.