ENTRY(_start)

/*
OUTPUT_FORMAT(binary)
*/

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

SECTIONS
{
	. = 0x100000;
	.text : ALIGN(4096)
	{
		*(.text)
	}
	.rodata : ALIGN(4096)
	{
		*(.rodata)
	}
	.data : ALIGN(4096)
	{
		*(.data)
	}
	.bss : ALIGN(4096)
	{
		*(.bss)
		*(.COMMON)
	}
	.text.asm : ALIGN(4096)
	{
		*(.text.asm)
	}
}
