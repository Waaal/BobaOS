# PS2 Keyboard
The PS2 keyboard is interrupt driven and is connected via the PIC at IRQ 1. [See here](STCQ/HardwareInterrupts.md#default-isa-irqs)* 
Each time a key is pressed a interrupt is send via IRQ 1.


Important PS2 IO Ports
| IO Port | Type | Purpose |
| ------ | ------ | ------ |
| 0x60 | Read/Write | Data |
| 0x64 | Read | Status |
| 0x64 | Write | Command |


**Data:** Over this port we read and write data to the PS2 Keyboard or controller. Here we also read the scancodes.


**Status:**


**Command:** For sending commands to the PS2 Controller. A important command is the 0xAE which enables the PS2 Keyboard.


## Init the PS2 Keyboad
For the ps2 keyboard to work we need to send 0xAE to the command register which enables the ps2 keyboard.

## Get keypresses
The PS2 is a extremly simple protocoll by sending scancodes to the keyboard data port. So when we receive a interrupt on IRQ1 we just need to read the scancode from the data port (1byte). The scancode is 1 byte large and represents a key press on the keyboard. 


After we read from the Data port we should read another byte from the data port because somethimes the PS2 keyboard sends some additional info.


*Note: We should only poll from the data port if the bit 0 of the FLAGS is 0, so we can be sure that the key is there*

## Differenciate between key reased and key down
Sometimes it is imporant to know if we pressed a key down or if we released a key. To know this the ps2 keyboard has a simple trick.
The last bit of a scancode is used as a "masked" bit. If the last bit is 0 we pressed the key down. If the last bit is 1 we relaesd the key.


So we just need to mask our scancode with 0x80 (128) to know if we pressed down or reales a key.

## Implemenatation
This function gets called by the IDT hander on IRQ 1 interrupt
``` c
void classic_keyboard_handle_interrupt()
{
	uint8_t scancode = 0;
	scancode = insb(KEYBOARD_INPUT_PORT); //Read the scancode
	insb(KEYBOARD_INPUT_PORT); //Ignores rouge byte maybe send
	
	//If the key is releast we do nothing, we just want key pressed
	if(scancode & CLASSIC_KEYBOARD_KEY_RELEASED)
	{
		return;
	}

	uint8_t c = classic_keyboard_scancode_to_char(scancode);
	if(c != 0)
	{
		//Push key to Virtual keyboard layer
		keyboard_push(c);
	}
}
```
