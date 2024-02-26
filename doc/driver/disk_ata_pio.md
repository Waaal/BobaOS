# ATA PIO
Advanced Technology Attachment (ATA) is a interface to connect hard drives and CD-ROM drives. 
PIO stands for Programmed input-output and is the simplest form of a disk driver. PIO mode is always supportet by all (S)ATA-compliant drives (so every normal consumer hard drive ever build).

ATA PIO is extremly slow, because every byte must be send trough the CPU'S IO port bus. So it is not recommend to use ATA PIO as the driver for all disk operation on the system, but in the boot and setup process where there is no multitasking, other processes etc... a ATA PIO driver is extremly usefull and simple. The kernel can load all of its important stuff with this and later switch to a AHCI driver.

**ATA PIO bits**
- Total 8 bit
- LBA 28 bit

## Primary/Secondary
Disk controller chips support 2 ATA buses per chip. These two are called primary and secondary. 
The IO-ports of the Primary and Secondary drives are standardized. When the system boots the PCI disk controller is in Legacy/Compability mode and supports these standardized IO-port settings.

W = write mode. R = read mode

**Primary**
| Port  | Mode | Descritpion |
| ------ | ------ | ------ |
| 0x1F0 | R/W | Data register |
| 0x1F1 | R | Error register |
| 0x1F1 | W | Features register |
| 0x1F2 | R/W | Total sectors to read/write (8-bit) |
| 0x1F3 | R/W | 0 - 7 of starting LBA |
| 0x1F4| R/W | 8 - 15 of starting LBA  |
| 0x1F5 | R/W | 16 - 23 of starting LBA  |
| 0x1F6 | R/W | 24 - 27 of starting LBA and bits 28-31 needs to be 1110 | 
| 0x1F7 | R | Status register | 
| 0x1F7 | W | Command register | 

**Secondary**
| Port  | Mode | Descritpion |
| ------ | ------ | ------ |
| 0x170 | R/W | Data register |
| 0x171 | R | Error register |
| 0x171 | W | Features register |
| 0x172 | R/W | Total sectors to read/write (8-bit) |
| 0x173 | R/W | 0 - 7 of starting LBA |
| 0x174| R/W | 8 - 15 of starting LBA  |
| 0x175 | R/W | 16 - 23 of starting LBA  |
| 0x176 | R/W | 24 - 27 LBA and bits 28-31 needs to be 1110 | 
| 0x177 | R | Status register | 
| 0x177 | W | Command register | 

*Note: If there are more drives/buses their ports are 0x1E0-0x1E7 for primary and  0x160-0x167 for secondary*

Normaly the disk we boot from is set default as the Primary disk (0x1F0) by the BIOS.

## Reading from ATA PIO
We can put the disk controller in read mode by sending 0x20 to the command register (0x1F7). When we send 0x20 to the command register, we need to make sure that all the other values, LBA and Total number of sectors are in there appropriate register.

After we send the command, the command register turns into the Status register. Now we need to read from the Status register port. If we get back a number where bit 4 is set, we know the disk is ready and we can start reading.

We read from the Data register (0x1F0) 2 bytes at a time.

## Simple Implementation

### C - Read
``` c
//outb = outputs byte to port
//outw = outputs 2 byte to port

//insb = reads byte from port
//insw = reads 2 bytes from port

//lba = logical block starting address
//total = total blocks to read
//buf = buffer to save read content
int disk_read_sector(int lba, int total, void* buf)
{
    outb(0x1F2, total);
    outb(0x1F3, (unsigned char)lba);        //Bits 0 - 7 of lba
    outb(0x1F4, (unsigned char)lba >> 8);   //Bits 8 - 15 of lba
    outb(0x1F5, (unsigned char)lba >> 16);  //Bits 16 - 23 of lba
    outb(0x1F6, (unsigned char)(lba >> 24) | 0xE0);  //Bits 24 - 27 of lba, bits 28 - 31 = 1110
    outb(0x1F7, 0x20); //Put in read mode

    unsigned short* ptr = (unsigned short*)buf;
    for(int i = 0; i < total; i++)
    {
        //Wait for buffer to be ready
        char c = 0x0;
        while(!(c & 0x8))
        {
            c = insb(0x1F7);
        }

        // Copy from hard disk to memory
        //256 because we read 2 bytes at a time. One sector is 512 bytes so (256*2 = 512)
        for(int i = 0; i < 256; i++)
        {
            *ptr = insw(0x1F0);
            ptr++;
        }
    }

    return 0;
}
```