# FAT - Filesystem
*This document explains the Fat filesystem*
## General
Fat is a extremely simple filesystem. FAT stands for File Allocation Table. The Filesystem works with a File Allocation Table, which is a struct at the start of the disk, which holds all the clusters on the disk. It also has a Root Directory which holds all the files that are saved on the disk. The FAT file systems stores data with clusters. One cluster holds multiple sectors. Both the size of a sector and the Numbers of sectors per clusters are changable in the BIOS Parameter Block.

There is Fat12, Fat16 and Fat32.

A Fat Volume is seperated into sector. Each sector is 512 bytes long on a standard fat(12,16) disk.

## Fat Disk 
A typically FAT disk is orderd into 4 regions:
- Boot Sector / Reserved
- File Allocation Tables
- Root Directory
- Data

### Boot Sector

The Boot sector is normaly 1 sector long (0x0E: number of reserved sectors) and is split into the BIOS Parameter Block and the Extended Boot Record. They need to contain the following:

**BIOS Parameter Block:**

| Offset | Size (in bytes) |Descritpion |
| ------ | ------ | ------ |
| 0x00 | 3 | The first 3 bytes dissassemble to JMP SHORT 3C NOP. The reason is: if this is a boot disk, then JMP SHORT would jump to your entry point of the bootloader. So if you boot from disk disk, chang 3C to your entry point (.start)|
| 0x03 | 8 | OEM identifier. Used by microsoft example: MSWIN4.1. Can now be any string with the length of 8 |
| 0x0B | 2 | Numbers of bytes per sector. Normaly 512 |
| 0x0D | 1 | Numbers of sectors per cluster |
| 0x0E | 2 | Numbers of reserved secors |
| 0x10 | 1 | Number of File allocation tables |
| 0x11 | 2 | Number of Root Directory entrys |
| 0x13 | 2 | Number of Total sectors. If 0 then number is greater than 65535 and is stored in Large sector count (0x20) |
| 0x15 | 1 | Media Type signature (0x0F0 = Floppy disk) |
| 0x16 | 2 | Numers of sectors per FAT. Means how long is File allocation table in sectors (FAT12/16 only) |
| 0x18 | 2 | Numbers of sectors per Track |
| 0x1A | 2 | Numbers of heads/sides of storage media |
| 0x1C | 4 | Numbers of hidden sectors |
| 0x20 | 4 | Large sector count |

**Extendet boot Record FAT12/16:**

| Offset | Size (in bytes) |Descritpion |
| ------ | ------ | ------ |
| 0x24 | 1 | Drive number 0x00 = Floppy; 0x80 = HDD|
| 0x25 | 1 | Windows NT flags / Resvered just keep them at 0 |
| 0x26 | 1 | Siganture 0x28 or 0x29 |
| 0x27 | 4 | VolumeID/ Serial Number of Volume |
| 0x2B | 11| Volume lable string |
| 0x36 | 8 | System ID string. Name for this drive |
| 0x3E |448| Boot code/or empty |
| 0x1FE| 2| Bootable partition signature 0xAA55 or empty |

**Extendet boot Record FAT32**

| Offset | Size (in bytes) |Descritpion |
| ------ | ------ | ------ |
| 0x24 | 4 | Sectors per Fat. The size of FAT in sectors. |
| 0x28 | 2 | Flags |
| 0x2A | 2 | FAT version number |
| 0x2C | 4 | Cluster number of root directory |
| 0x30 | 2 | Sector number of FSInfo structure |
| 0x32 | 2 | Sector number of the backup boot sector |
| 0x34 | 12| Reserved|
| 0x40 | 1 | Drive number |
| 0x41 | 1 | Flags in WindowsNT |
| 0x42 | 1 | Signature 0x28 or 0x29  |
| 0x43 | 4 | VolumeID/ Serial Number of Volume |
| 0x47 | 11| Volume lable string |
| 0x52 | 8 | System ID string. Name for this drive |
| 0x5A |420| Boot code/or empty |
| 0x1FE| 2| Bootable partition signature 0xAA55 or empty |

### File Allocation Tables
Here are stored the File Allocation Table structs. This section is always placed right after the Boot Sector section. It stores the cluster chain of the next cluster. More to that later...

On Fat12 one entry is 12 bits
On Fat16 one entry is 16 bits
On Fat32 one entry is 32 bits

The size of this region in sectors can be calculated by multiplying the Numbers of File Allocation Tales (0x10) with the Numers of sectors per FAT (0x16).

#### How are data structured in a FAT12?
FAT is little endian.
Fat12 is a little special, because it has 12 bytes. In FAT16 and FAT32 the data structure is better and dont need any explenation.

So entry 0x123, 0x456 would be placed like this in a FAT:
```
0x123   0001 0010 0011
0x456   0100 0101 0110

0x23         0x61         0x45
0010 0011    0110 0001    0100 0101
```

#### How to get a entry in FAT12?
```
FAT:
0:    0xFF0    1111 1111 0000
1:    0xFFF    1111 1111 1111
2:    0x005    0000 0000 0101
3:    0x009    0000 0000 1001
4:    0x00B    0000 0000 1011
5:    0x00C    0000 0000 1100

Actuall on disk:
0xF0         0xFF         0xFF         0x05         0x90         0x00         0xB          0xC0         0x00
1111 0000    1111 1111    1111 1111    0000 0101    1001 0000    0000 0000    0000 1011    1100 0000    0000 0000
```
We want to read index 2.
2 x 1.5 = 3(because 8 x 1.5 is 12 and FAT is 12 bits long)

We check if 2 % 2 == 0
We take 16 bit from index 3 = 0000 0101 1001 0000 = 0x05 0x90
From little endian to dec = 0x9005
Then we take the low 12 bit and got the index: 0x9005 & 0xFFF = 0x5

We want to read index 4.
If index % 2 != 0:
We take 16 bit from index 4 = 1001 0000 0000 0000 = 0x90 0x00
From little endian to dex = 0x0090
Then we shift right 4 bits: 0x0090 >> 4 = 0x9.

Tada. Thats how we read the 12 bit FAT table.
The 16 and 32 bit are self explenatory.

### Root Directory
On Fat12,16 the Root Directory is placed right after the FAT directory. On Fat32 its possition is given by the Extendet boot Record (0x2C).
In the Root Directory are stored all the files. To be more exact it stores more than that. It stores file names, Attributes, Creation date, starting cluster etc. 
On entry is 32 bits long.

Calculate the size of the root directory: Number of Root Directory entrys (0x11) * 32bytes + Bytes per sector (0x0B)

Struct of a root directory entry
| Offset | Size (in bytes) |Descritpion |
| ------ | ------ | ------ |
| 0x00 | 11 | Filename with extension |
| 0xB | 1 | Attributes of File: READ_ONLY=0x01;HIDDEN=0x02;SYSTEM=0x04;VOLUME_ID=0x08;DIRECTORY=0x10;ARCHIVE=0x20 |
| 0xC | 1 | Reserved. Used by Windows NT |
| 0xD | 1 | Creation time in tens of a second |
| 0xE | 2 | Creating Time HOUR=5bits; MINUTES=6bits; SECONDS=5bits |
| 0x10 | 2 | Creating Date YEAR=7bits; MONTHS=4bits; DAY=5bits |
| 0x12 | 2 | Last access date. Same format as Creation date |
| 0x14 | 2 | High 16 bits of the Start cluster number |
| 0x16 | 2 | Last modifiet time. Same format as Creation time |
| 0x18 | 2 | Last modifiet date. Same format as Creation date |
| 0x1A | 2 | Low 16 bit of Start cluster number |
| 0x1C | 4 | The size of the file in bytes |

*Important note: Number 0 and 1 on the starting cluster are reserved. So indexing starts at 2 and later calculate -2 to get the lba address.*

## How to get a File?

We first need the file name and the search for it in the Root Directory.
In the Root Directory we got the starting cluster. We read the content of the starting cluster and the go to the File Allocation Table.
For example if the start cluster of a file is 3 we then look at the File Allocation Table at entry 3. This entry holds the next cluster and the next entry of the FAT. For example 4. We then read all the bytes from the 4th cluster and look at the FAT at entry 4. This 4th enty points us to the next cluster etc. This is what I meant with chain. 

We know if we reacht the end of a cluster if the entry of the FAT is 0xFF8. 0xFF8 marks the end of a chain.