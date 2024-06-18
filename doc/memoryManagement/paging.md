# Paging
Paging is implemented by the Memory Managmen Unit (MMU).
Paging works with virual addresses that are mapped to Physical addresses. 
With paging the physical memory is divided into blocks called pages.

So normaly a program is loaded into the RAM and wants to use all of the RAM, because it assumes it is the only programm running. This is dangerous because now a program can access all of the RAM and other programs memory. So a program could now accidentally overwrite another programms memory. So with paging we can give each programm its own virtual memory. A programm cannot leave this virtual memory and thinks, it is the only program on the whole machine.

The MMU maps a virual address space to the actual physical address in the RAM through a **Page Table**. An app always thinks its memory starts from 0 and it has all of the RAM.

The size of these pages are in 64 bit mode 1gb, 2mb and 4kb. So if a app request something with a virtual address, the MMU translates it with a page translation table to the page in the physical memory. This process is called **page translation**.
The page table struct is stored in RAM.

## Problem: Page Translation on a 4kb page

Lets say we want to seperate all of our memory to 4kb pages and take a page table for this.
4KB pages means that bit 31 - 12 is used as entry in the page table. and bit 11 - 0 are used as a offset in the page.
So:
```
Virual Address: 0x00003204

31                  12 11        0
|      0x00003        |   0x204  |


Page Table:
| Virual    | Physical|
| 0x0000    |  DISK   |
| 0x0001    |  0x003  |
| 0x0002    |  0x004  |
| 0x0003    |  0x006  |
| ...       |  ...    |

So Virtual address 0x00003204 gets translated to physical address: 0x006204
```
But there is a problem. Each programm needs its own page table. So on a machine where each page is 4kb thats 1M entrys.
32bit - 12bit page offset = 20bits.
Each table entry is 4 bytes(20bits for physical address + permission bits) so 4MB. 4MB for each programm.
So if we have 100 programms that are 400MB on page tables only.

That is to much so we have multiple page tables with different levels.

## Multi level page tables and multi level page lookup

For this the Multi level page translation system got created. With this we have multiple level of page tables.
The level and numbers of page tabels differs on page size and os size (32/64 bit).

**On a 32 bit operating system we have**
- [4KB pages](paging.md#32-Bit-4KB) (2 page tables)
- [2MB pages](paging.md#32-Bit-4MB) (1 page table)

**On a 64 bit operating system we have**
- 4KB pages (4 page tables)
- 2MB pages (3 page table)
- [1GB pages](paging.md#64-Bit-1GB) (2 page tables)

Each page table holds some permissions and the entry in the next level of page table or the actuall place in RAM.

Calculating with multiple pages is called a Page translation.

But how does page translation solve our memory problem? Becuase on a 32bit system with 4GB of RAM and 4kb pages the page tables will still add ub to 4MB.
But this time we can dynamically genearte more page tabels. Because we have one PD which holds other PT we only need min one PD and one PT. This is 8kb. With this 8kb we have 0x400000bytes of virtual ram. If the program needs more, we can dynmalically create more PT`s.

## 32 Bit 4KB

| Level | Number | Entries | Name |
| ------ | ------ | ------ | ------ |
| 0 | - | - | RAM |
| 1 | 1024 | 1024 | Page Table |
| 2 | 1 | 1024 | Page Directory |

### 4KB Lookup 32 bit
On a 32 bit system our address is 32 bit long. So if we want to seperate our system in 4kb pages, we need 2 Tables. 

The First table is called the **page directory (PD)** with 1024 entries. Each entry in the page directory points to a **page table (PT)** with 1024 entries. Each page table entry is a 4096 kb block.

So we have 1 Page directory and 1024 page tables on a 4kb page translation on a 32 bit system.

1024 * 1024 * 4096 = 4GB of max addressable memory in a 32 bit system.

So a 4kb page translation would look something like this
```
Virual Address: 0x04002204 = 00000000010000000010000000000000

 31        22 21        12 11           0
|     PD     |     PT     |    Offset    |
| 0000000001 | 0000000010 | 100000000010 |
|    0x1     |    0x2     |    0x204     |


PD: 0000000001 = 0x1                                   ->   PT: 0000000010 = 0x2
| Entry     | Physical|                                |    | Entry     | Physical| 
| 0x0       |  0x002  |                                |    | 0x0001    | 0x00023 |
|[0x1]      |  0x003  |   --> PT at memory addr 0x003 --    |[0x0002]   | 0x00045 |
| 0x2       |  0x004  |                                     | 0x0003    | 0x00067 |
| ...       |  ...    |                                     | ...       |  ...    |




So Virtual address 0x04002204 gets translated to physical address: 0x00045204
```
So we devide our address in 3 parts. Bits 31 - 22 are the index in the **PD** table. Bits 21 - 12 are the index in the **PT** table. And bits 11 - 0 are the offset in the final 4kb page.

So one PT has 1024 enties. Each entry covers 4096 byte page. So one PT can cover 1024 * 4096 = 0x400000 byte (4MB).
The one PD can have 1024 PTs so 1024 * 0x400000 = 0x100000000 (4GB).

Each entry in the PT is shiftet 4096 bytes. 
Each entry in the PD is shiftet 0x400000 bytes.
```
PD[0] = virual address space        0x0         -   0x400000.
PD[1] = virtual address space       0x400000    -   0x800000
PD[1023] = virtual address space    0xFFC00000  -   0x100000000
```
Now each Table (PD, PT) has entrys. And each entry is 32 bit.

#### PD & PT Entry
```
 31                               12 11   9 8  7 6 5 4  3  2  1 0
|     Bits 31 - 12 of address       | AVL  |G|PS|0|A|D|WT|US|RW|P|
```
 - P = Present: 0 = Page is not present (page fault will occure and we need to load that memory from disk or where it is). 
1 = Page is present in memory
 - RW = Read/Write: 0 = Read only. 1 = Read/Write allowed
 - US = User/Supervisor: 0 = Only Supervisor can access (ring 0). 1 = Page can be accessed by all (ring 0,1,2 and 3)
 - WT = Write-Through:  0 = Is not enabled. 1 = Write-Trough caching is enabled
 - D = Cache Disabled: 0 = It will be cached. 1 = Page will not be cached.
 - A = Accessed: Set to 1 by CPU if page is accessed
 - PS = Page Size (only use on PD): 0 = 4KB pages. 1 = 4MB pages
 - G = Global??????
 - AVL = Available: Freely by the os to use.

If the PD & PT holds actual physical addresses in memory (PD holds the start of the PT table, PT holds start of 4KB block) why are they addresses they hold only bits 31 - 12?

Because the 12th bit counting from 0 is exactly 4096. Because one table is 4096 bytes long, we only need bits 31-12 to describe the address of the table, because the other bits would point in the table.

Example:
```
                                          12 11         0
Table 0 is at 0x0000    00000000000000000000 000000000000
Table 1 is at 0x1000    00000000000000000001 000000000000
Table 2 is at 0x2000    00000000000000000010 000000000000
``` 
**But with this, we cannot have a table at position 0x0010. The tables need to be aligned at 4096 bytes**

## 32 Bit 4MB

## 64 Bit 4KB

## 64 Bit 2MB

## 64 Bit 1GB
On a 64 bit system our address is 64 bit long. So if we want to seperate our system in 1Gb pages, we need 2 Tables. 


The First table is called the **PLM4** with 512 entries. Each entry in the page directory points to a **page directory pointer (PDP)** with 512 entries. Each page table entry is a 1 gb block.


Each entry in a PLM4 represents 512 GB. 


Each entry in a PDP represents 1GB.



So a 1gb page translation would look something like this
```
Virual Address: 0x04002204 = 0000000000000000000000000000000000000000010000000010000000000000

 63              48 47       39 38      31 30           Offset            0
| Sign extension   |    PLM4   |    PDP   |                                |
| 0000000000000000 | 000000000 | 00000000 | 000000010000000010000000000000 |
|       0x0        |    0x0    |    0x0   |          0x04002204            |


PLM4: 000000000 = 0x0                                   ->   PDP: 00000000 = 0x0
| Entry     | Physical|                                |    | Entry     | Physical| 
| [0x0]     |  0x002  |   --> PT at memory addr 0x002 --    | [0x0]     | 0x00023 |
| 0x1       |  0x003  |                                     | 0x1       | 0x00045 |
| 0x2       |  0x004  |                                     | 0x2       | 0x00067 |
| ...       |  ...    |                                     | ...       |  ...    |




So Virtual address 0x04002204 gets translated to physical address: 0x4002227
```
So we devide our address in 3 parts. Bits 47 - 39 are the index in the **PLM4** table. Bits 38 - 31 are the index in the **PDP** table. And bits 30 - 0 are the offset in the final 1GB page.


Now each Table (PLM4, PDP) has entrys. And each entry is 64 bit (8 byte).


#### PLM4
```
  63  62 52 51 49 48                            12 11   9 8  7 6 5 4  3  2  1 0
| XD | AVL | RES | Bits 48 - 12 of address of PDP | AVL  |G|PS|0|A|D|WT|US|RW|P|
```
#### PDP Entry
```
  63  62 52 51 49 48                                  30 29 12 11   9 8  7 6 5 4  3  2  1 0
| XD | AVL | RES | Bits 48 - 30 of address of 1GB block | RES | AVL  |G|PS|0|A|D|WT|US|RW|P|
```
 - P = Present: 0 = Page is not present (page fault will occure and we need to load that memory from disk or where it is). 
1 = Page is present in memory
 - RW = Read/Write: 0 = Read only. 1 = Read/Write allowed
 - US = User/Supervisor: 0 = Only Supervisor can access (ring 0). 1 = Page can be accessed by all (ring 0,1,2 and 3)
 - WT = Write-Through:  0 = Is not enabled. 1 = Write-Trough caching is enabled
 - D = Cache Disabled: 0 = It will be cached. 1 = Page will not be cached.
 - A = Accessed: Set to 1 by CPU if page is accessed
 - PS = Page Size: Leave 0 on PLM4. On PDP set to 1 for 1GB pages.
 - G = Global: 0 =  Page isnt flushed from chaches on address space switch (PGE bit of CR4 register must be set). 1 = Page is flushed.
 - AVL = Available: Free to use for the OS.
 - RES = Reserved: Always 0.
 - XD = no execute: 0 = Code can be executed. 1 = Code cannot be executed.


If the PLM4 & PDP holds actual physical addresses in memory (PLM4 holds the start of the PDP table, PDP holds start of 1GB block) why are they addresses they hold only bits 48 - 12 and bits 48 - 30.


**PLM4:**
The PLM4 entry holds the address of a PDP table. We know that the size of a PDP is 4kb.
The 12th bit counting from 0 is exactly 4096. Because one table is 4096 bytes long, we only need bits 48-12 to describe the address of the table, because the other bits would point in the table.

**So the physical PDP location addresses need to be aligned to a 4kb boundary**


**PDP:**
Each PDP entry covers 1 GB. Bit 30 is exactly 1GB. So we only need to bit 30 because everything under 30 is in the 1 GB block itself.

Example:
```
Entry 0 covers 0x00000000 - 0x40000000 (1GB) 
Entry 1 covers 0x40000000 - 0x80000000 (1GB)

                                    30                           0
If we have address 0x50000000 =     101000000000000000000000000000
We see bit 30 = 1 and bit 28 = 1. We remove bit 28 and the address is now 0x40000000. Know we now that the address 0x50000000 is in the Entry 1. Bit 28 which is 0x10000000 is now the offset in the 1GB block. 

But such a address 0x50000000 would not be possible to store in the PDP because the PDP entrys need to be aligned to a 1GB boundary.
``` 


## Enable paging
Assembly example on how to enable paging
### 32 bit
To enable paging in 32 bit mode we need to enable bit 31 in the cr0 control register.
``` assembly
    mov eax, cr0
    or eax, 0x80000000  ; Enable bit 31 in cr0 = paging
    mov cr0, eax
```
### 64 bit

## Set paging directory
To set a paging directory we need to write the address of the first PD entry in the cr3 register

Super cool video explaining this
https://www.youtube.com/watch?v=Z4kSOv49GNc