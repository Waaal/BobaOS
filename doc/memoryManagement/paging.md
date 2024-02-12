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
- [4KB pages](paging.md#32Bit4KB) (2 page tables)
- [2MB pages](paging.md#32Bit2MB) (1 page table)

**On a 64 bit operating system we have**
- 4KB pages (4 page tables)
- 2MB pages (3 page table)
- 1GB pages (2 page tables)

Each page table holds some permissions and the entry in the next level of page table or the actuall place in RAM.

Calculating with multiple pages is called a Page translation.

## 32 Bit 4KB

| Level | Number | Entries | Name |
| ------ | ------ | ------ | ------ |
| 0 | - | - | RAM |
| 1 | 1024 | 1024 | Page Table |
| 2 | 1 | 1024 | Page Directory |

### 4KB Lookup 32 bit
On a 32 bit system our addres is 32 bit long. So if we want to seperate our system in 4kb pages, we need 2 Tables. 

The First table is called the **page directory (PD)** with 1024 entries. Each entry in the page directory points to a **page table**with 1024 entries. Each page table entry is a 4096 kb block.

So we have 1 Page directory and 1024 page tables on a 4kb page translation on a 32 bit system.

1024 * 1024 * 4096 = 4GB of max addressable memory in a 32 bit system.

So a 4kb page translation would look something like this
https://www.youtube.com/watch?v=Z4kSOv49GNc
```
Virual Address: 0x00000000

 31        21 20   12 11        0
|     PD     |  PT   |Offset    |
| 0x000      |0x00   |0x000     |


PD:                                                  ->   At location 0x002 (PT)
| Virual    | Physical|                                |    | Virual    | Physical| 
|[0x0000]   |  0x002  | ---> Table at location 0x002 -      | 0x0003    | 0x00023 |
| 0x0001    |  0x003  |                                     |[0x0004]   | 0x00045 |
| 0x0002    |  0x004  |                                     | 0x0005    | 0x00067 |
| ...       |  ...    |                                     | ...       |  ...    |




So Virtual address 0x0000000100000000 gets translated to physical address: 0x0000000000450000
```

Now each Table (PD, PT) has entrys. And each entry is 32 bit but differs a little bit in its structure

### 4KB page table structure 32 bit

Because the 4kb page translation needs 2 tables, we have a **PD** and a **PT** entry struct.

#### PD Entry
```
 31                               12 11   9 8  7 6 5 4  3  2  1 0
|     Bits 31 - 12 of address       | AVL  |G|PS|0|A|D|WT|US|RW|P|
```
 - P = Present: 0 = Page is not present (page fault will occure and we need to load that memory from disk or where it is). 
1 = Page is present in memory
 - RW = Read/Write: 0 = Read only. 1 = Read/Write allowed
 - US = User/Supervisor: 0 = Only Supervisor can access (ring 0). 1 = Page can be accessed by all
 - WT = Write-Through:  0 = Is not enabled. 1 = Write-Trough caching is enabled
 - D = Cache Disabled: 0 = It will be cached. 1 = Page will not be cached.
 - A = Accessed: Set to 1 by CPU if page is accessed
 - PS = Page Size: 0 = 4KB pages. 1 = 4MB pages
 - G = Global??????
 - AVL = Available: 1 = It is available; 0 = It is not available.

#### PT Entry
```
 31                               12 11   9 8  7 6 5 4  3  2  1 0
|     Bits 31 - 12 of address       | AVL  |G|PS|0|A|D|WT|US|RW|P|
```
Bits 11 - 0 are same as in PD Entry

## 32 Bit 2MB

## 64 Bit 4KB

## 64 Bit 4KB

## 64 Bit 2MB

## 64 Bit 1GB