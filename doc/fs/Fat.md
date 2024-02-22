# FAT - Filesystem
*This document explains the Fat filesystem*
## General
Fat is a extremely simple filesystem. FAT stands for File Allocation Table. The Filesystem works with a File Allocation Table, which is a struct at the start of the disk, which holds all the clusters on the disk. It also has a Root Directory which holds all the files and subdirectories that are saved on the disk. The FAT file systems stores data with clusters. One cluster holds multiple sectors. Both the size of a sector and the Numbers of sectors per clusters are changable in the BIOS Parameter Block.

There is Fat12, Fat16 and Fat32 (also exfat but who cares about exfat).

A Fat Volume is seperated into sector. Each sector is 512 bytes long on a standard fat(12,16) disk.

#### LBA
Lba stands for logical block address. It is a way to index all the sectors on a disk. 
It starts from 0 for the first sector and goes up how many sectors a disk has.

#### Cluster
One sector is normaly 512 bytes (0x0B in boot sector). One cluster can have multiple sectors (0x0D in boot sector).

## Fat Disk 
A typically FAT disk is orderd into 4 regions:
- Boot Sector
- More Reserved (optional. Value in Boot Sector 0x0E)
- File Allocation Tables
- Root Directory
- Data

### Boot Sector

The Boot sector is 1 sector long and is split into the BIOS Parameter Block and the Extended Boot Record.
The Boot sector counts as a reserved sector (0x0E index in boot sector). 
They need to contain the following:

**BIOS Parameter Block:**

| Offset | Size (in bytes) |Descritpion |
| ------ | ------ | ------ |
| 0x00 | 3 | The first 3 bytes dissassemble to JMP SHORT 3C NOP. The reason is: if this is a boot disk, then JMP SHORT would jump to your entry point of the bootloader. So if you boot from disk, chang 3C to your entry point (.start) |
| 0x03 | 8 | OEM identifier. Used by microsoft example: MSWIN4.1. Can now be any string with the length of 8 |
| 0x0B | 2 | Numbers of bytes per sector. Normaly 512 |
| 0x0D | 1 | Numbers of sectors per cluster |
| 0x0E | 2 | Numbers of reserved secors (at least 1 for boot sector) |
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
Here are stored the File Allocation Table structs. This section is always placed right after the Boot Sector section and reserved sectors. It stores all the clusters and the status, if they are taken or if they are the end of a chain.

So each entry in the FAT respresents one cluster. If the entry for a specific cluster is 0 that means this cluster is free to use. If the entry is FF or a number that means this entry is in use. So for example if we want to know if cluster 10 is taken we look at the index 10 in the FAT.

Some files/directories are larger than one cluster. So one data chunk can spread over multiple clusters. But the root directory only tells us the starting cluster and the size of a file. So for this the numbers in the FAT are important. 

For example if we look at cluster 3 and it is taken, but it is larget than one cluster then in the FAT at index 3 would stand a number. This number tells us the next cluster of the data chunk. If we look at the new cluster index in the FAT it would tell us the next cluster etc. If on entry in the FAT returns 0xFF8 or 0xFFF this means we have reached the final cluster of the data chunk.

On Fat12 one entry is 12 bits
On Fat16 one entry is 16 bits
On Fat32 one entry is 32 bits

The size of this region in sectors can be calculated by multiplying the Numbers of File Allocation Tales (0x10) with the Numers of sectors per FAT (0x16).

An entry is set to:
	- FF8 or FFF: if used and if of chain
	- FF0 or FF6: if reserved
	- FF7: if it is a bad sector
	- Number: if used and not end of chain
	- 0x00: if free

### Root Directory
On Fat12,16 the Root Directory is placed right after the FAT directory. On Fat32 its position is given by the Extendet boot Record (0x2C).
The root directoy is the root directory of all the files and subdirectories. 

The root directory and all subdirectories hold directory entries. In an entry is the file name, extension and other attributes saved. It also holds the information if this entry is a file or a directory.
On entry is 32 bits long.
Its a array out of entries.

Calculate the size of the root directory: Number of Root Directory entrys (0x11) * 32bytes

We know if we have reached the end of a directory if we have a NULL entry. Basically a entry that has all 0). 

If the Filename at position 0 has 0xE5 as first character, this means this entry is not used.

Struct of a directory entry
| Offset | Size (in bytes) |Descritpion |
| ------ | ------ | ------ |
| 0x0 | 8 | Filename (unused bytes must be padded with spaces) |
| 0x8 | 3 | Extension (unused bytes must be padded with spaces) |
| 0xB | 1 | Attributes of File (bitmask): READ_ONLY=0x01;HIDDEN=0x02;SYSTEM_FILE=0x04;VOLUME_ID=0x08;DIRECTORY=0x10;ARCHIVE=0x20;DEVICE=0x40;RESERVED=0x80 |
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

**Important note:** The start cluster number in a dir entry is in Data cluster format. Data cluster format starts from 0. The 0 is the cluster after root dir. It is important to take this into account. If we want to calculate the absolute sector of a data cluster we need to add all the sectors bevore the data cluster (reserved, FAT and root dir) to the end result.

Number 0 and 1 of the starting cluster entry are reserved. We need to add -2 for a absolute value.
For example, if the Start cluster number of a entry says 4. It its actually the 2 in the data cluster.

## How to get a File?
We first need the file name and the search for it in the Root Directory of the file.
In the Directory entry we got the starting cluster. We read the content of the starting cluster and the go to the File Allocation Table.
For example if the start cluster of a file is 3 we then look at the File Allocation Table at entry 3. This entry holds the next cluster and the next entry of the FAT. For example 4. We then read all the bytes from the 4th cluster and look at the FAT at entry 4. This 4th enty points us to the next cluster etc.

We know if we reacht the end of a cluster if the entry of the FAT is 0xFFF or 0xFF8. This marks the end of a chain.

## How to get a Directory?
We first need the directory name and search for it in the Root Directory of the directory.
In the Root Directory we got the starting cluster.

Other than in a file where we read the raw binary data of the file in a directory we read another directory entry array from the disk.

We read the content of the starting cluster and the go to the File Allocation Table.
For example if the start cluster of a directory is 3 we then look at the File Allocation Table at entry 3. This entry holds the next cluster and the next entry of the FAT. For example 4. We then read all the bytes from the 4th cluster and look at the FAT at entry 4. This 4th enty points us to the next cluster etc.

We know if we reacht the end of a cluster if the entry of the FAT is 0xFF. 0xFF marks the end of a chain.

If we have a directory path like this: 0:/dir1/dir2/dir3 and want dir3, we first need to load the dir1 entries, then from there the dir2 entries and here we find the dir3 entries.

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