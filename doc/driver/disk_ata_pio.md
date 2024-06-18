# ATA PIO
Advanced Technology Attachment (ATA) is a interface to connect hard drives and CD-ROM drives. 
PIO stands for Programmed input-output and is the simplest form of a disk driver. PIO mode is always supportet by all (S)ATA-compliant drives (so every normal consumer hard drive ever build).

ATA PIO is extremly slow, because every byte must be send trough the CPU'S IO port bus. So it is not recommend to use ATA PIO as the driver for all disk operation on the system, but in the boot and setup process where there is no multitasking, other processes etc... a ATA PIO driver is extremly usefull and simple. The kernel can load all of its important stuff with this and later switch to a AHCI driver.


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
| 0x1F6 | R/W | Drive / Head Register | 
| 0x1F7 | R | Status register | 
| 0x1F7 | W | Command register | 
| 0x3F6 | R/W | Device control register | 

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
| 0x176 | R/W | Drive Head register  | 
| 0x177 | R | Status register | 
| 0x177 | W | Command register | 
| 0x376 | R/W | Device control register | 

*Note: If there are more drives/buses their ports are 0x1E0-0x1E7 for primary and  0x160-0x167 for secondary*

Normaly the disk we boot from is set default as the Primary disk (0x1F0) by the BIOS.


### Status Bits
The state bits of the status register.
| Bit  | Name | Descritpion |
| ------ | ------ | ------ |
| 0 | ERR| Inidcates a error.|
| 1 | IDX | -|
| 2 | CORR | - |
| 3 | DRQ | Is set if the driver is ready for receiving/sending data. |
| 4 | SRV | - |
| 5| DF | - |
| 6 | RDY | Bit is clear if driv is spun down.  |
| 7 | BSY | If the drive is bussy sending/receiving data. Wait for it to clear | 


### Drive/Head register bits
The state bits of the status register.
| Bit  | Name | Descritpion |
| ------ | ------ | ------ |
| 0 - 3 | | In LBA addressing bits 24 to 27 of LBA |
| 4 | DRV | Selects drive number |
| 5 | 1 | Always set |
| 6 | LBA | IF clear CHS addressing is used. Else LBA. |
| 7 | 1 | Always set |



### Device control register bytes
The state bits of the status register.
| Bit  | Name | Descritpion |
| ------ | ------ | ------ |
| 0 | n/a | Always 0 |
| 1 | nIEN | Set this to stop the current device to send IRQs. |
| 2 | SRST | Set (and clear after 5us) to do a software reset on the master and slave |
| 3 | n/a | - |
| 4 | n/a | - |
| 5| n/a | - |
| 6 | n/a | - |
| 7 | HOB | - | 


## Reading from ATA PIO
To read from a ATA PIO device we should fist initialize the ATA PIO it. 
So we should make sure that:
- Reset all driver (for first time user after boot)
- A drive is connected to a port
- Select drive
- Send IDENTIFY Command


### Reset driver
To do a software reset we set the SRST bit in the Device control register (0x3F6). Then we wait for 5us and clear it again.


This will reset the Master and Slave.

### Find drive connected to port
If a port is not connected to a drive, the drive will always return 0xFF. So we can read the status register of a drive and if the return value is 0xFF we know that no drive is connected to this port.

### Select drive
To select a drive we send 0xA0 for Primary or 0xB0 for secondary to the Drive / Head register (0x1F6).

### Identifiy command
With the Identify command we get some info about the disk currently connected to the port. To send a identify command we first need to set 0 the following registers: (0x1F2 - 0x1F5). 


Then we write 0xEC (IDENTIFY command) to the command register (0x1F7). Then we wait till the BSY bit clears and then wait till the DRQ bit is set. 

Now we need to read 256 * 2 bytes. These 512 contains info about the disc.


Interesting bits of the 256 uint16_t bytes indetify struct:
- (uint16_t) 27 - 46: Contains model string of current disk (40 bytes).
- (uint16_t) 60 - 61: Used as 32 bit number contains total number of sectors in LBA format (in LBA28). (4 bytes)
- (uint16_t) 100 - 103: Used as 64 bit number contains total number of sectors in LBA format (in LBA48). (8 bytes)

### Read 
First check if the BSY bit is clear.


We can put the disk controller in read mode by sending 0x20 to the command register (0x1F7). When we send 0x20 to the command register, we need to make sure that all the other values, LBA and Total number of sectors are in there appropriate register.


After we send the command, the command register turns into the Status register. Now we need to read from the Status register port. If the DRQ bit is set we can read the data.


We read from the Data register (0x1F0) 2 bytes at a time.


## Stuff to know
- If we want to read and we allow the disk to send IRQs (nIEN = 0 in Device control register) and our intterrupts are enabled then it is possible that the data is send to the IRQ interrupt handler and if we try to read the data per IO bus we get garbage. So the IRQ then steals our data.

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
    // ATAPIO_PRIMARY_IO = 0x1F0; 
    uint8_t base = ATAPIO_PRIMARY_IO;

    //Wait till bsy bit is clear
    atapio_wait_bsy(base);

    outb(base + ATAPIO_REG_ERROR, 0x00);
    outb(base + ATAPIO_REG_SECTOR, total);
    outb(base + ATAPIO_REG_LBA0, (unsigned char)(lba & 0xFF)); //Bits 0 - 7 from address
    outb(base + ATAPIO_REG_LBA1, (unsigned char)(lba >> 8) & 0xFF); //Bits 8 - 15 from address
    outb(base + ATAPIO_REG_LBA2, (unsigned char)(lba >> 16) & 0xFF); //Bits 16- 23 from address
    outb(base + ATAPIO_REG_DRIVESELECT, 0xE0 | ((lba >> 24) & 0x0F)); //Bits 24- 27 from address, bits 28 - 31 are set to 1110
    outb(base + ATAPIO_REG_COMMAND, 0x20);

    //Wait till drq bit is set
    atapio_wait_drq(base);

    unsigned short* ptr = (unsigned short*)buf;
    for(int i = 0; i < total; i++)
    {
        //Copy from hard disk to memory
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