# Paging

## Explenation
Paging is implemented by the Memory Managmen Unit (MMU).
Paging works with virual addresses that are mapped to Physical addresses. 
With paging the physical memory is divided into blocks called pages.


So normaly a program is loaded into the RAM and wants to use all of the RAM, because it assumes it is the only programm running. This is dangerous because now a program can access all of the RAM and other programs memory. So a program could now accidentally overwrite another programms memory. So with paging we can give each programm its own virtual memory. A programm cannot leave this virtual memory and thinks, it is the only program on the whole machine.


The MMU maps a virual address space to the actual physical address in the RAM through a **Page Table** .A app always thinks its memory starts from 0 up to how big its page is. 


The size of these pages are in 64 bit mode 1gb, 2mb and 4kb. So if a app request something with a virtual address, the MMU translates it with a page translation table to the page in the physical memory. This process is called **page translation**.
The page table struct is stored in RAM.
### Page Translation on a 4kb page
On a 32 bit address the bit 31 - 12 is used as entry in the page table. and bit 11 - 0 are used as a offset in the page.
So:
```
Virual Address: 0x00003204

31					12 11		 0
|	   0x00003 	  	  |	  0x204  |


Page Table:
| Virual    | Physical|
| 0x0000 	|  DISK   |
| 0x0001 	|  0x003  |
| 0x0002 	|  0x004  |
| 0x0003 	|  0x006  |
| ...	    |  ...    |

(A page table also holds permissions bits but more to that later)

So Virtual address 0x00003204 gets translated to physical address: 0x006204
```

But there is a problem. Each programm needs its own page table. So on a machine where each page is 4kb thats 1M entrys.
32bit - 12bit page offset = 20bits.
Each table entry is 4 bytes(20bits for physical address + permission bits) so 4MB. 4MB for each programm.
So if we have 100 programms that are 400MB on page tables only.

That is to much so we have multiple page tables with different levels.
### Multi level page tables and multi level page lookup (64 bit)
On a 64 bit system our address is 64 bit long. Because we have differnet page sizes (1gb, 2mb, 4kb) we have different levels of page translation:


#### 1Gb page translation
```
Virual Address: 0x000000000010000b

63	   48	   		39		30			0
|Sign	| PLM4		 |PDP	 |Offset	|
|Sign	| 0x000		 |0x40	 |0x000b	|
(Sign is not used here)

PLM4:												    ->	At location 0x002 (PDP)
| Virual    | Physical|  							   |	| Virual    | Physical| 
|[0x0000] 	|  0x002  | ---> Table at location 0x002 -		| 0x0035	| 0x00023 |
| 0x0001 	|  0x003  |										|[0x0040] 	| 0x00045 |
| 0x0002 	|  0x004  |										| 0x0050 	| 0x00067 |
| ...	    |  ...    |										| ...	    |  ...    |




So Virtual address 0x000000000010000b gets translated to physical address: 0x000000000045000b
```

#### 2mb page translation
```
Virual Address: 0x000020000100000b

63	   48	   		39		30       21			0
|Sign	| PLM4		 |PDP	 |PD	  |Offset	|
|Sign	| 0x000		 |0x40	 |0x010   |0x00b    |
(Sign is not used here)

PLM4:										->	At location 0x002 (PDP)						->	At location 0x045 (PD)
| Virual    | Physical|  					|	| Virual    | Physical|   					|	| Virual    | Physical| 
|[0x0000] 	|  0x002  | ---> location 0x002 -	| 0x0035	| 0x00023 | 					|	| 0x005 	| 0x00063 |
| 0x0001 	|  0x003  |							|[0x0040] 	| 0x00045 |	---> location 0x045 -	|[0x010] 	| 0x00A54 |
| 0x0002 	|  0x004  |							| 0x0050 	| 0x00067 |							| 0x015		| 0x00FFF |
| ...	    |  ...    |							 ...	    |  ...    |							 ...	    |  ...    |




So Virtual address 0x000020000100000b gets translated to physical address: 0x0000000000A5400b
```

The 4kb page translation is the same, but i needs one more table level.

### Page Table entry
A entry in a Page Table is 64bit in long mode.
It holds either the address to the next level of table, or the address of the Page in the Physical memory. But it also holds some Attributes for protection and other stuff.
```
 63 62      59 58        52 51     M M-1            				 13 12   11    9 8  7 6 5   4   3  2  1 0
|XD|    PK    |    AVL     |RESERVED|    Bits of address (M)		   | PAT|  AVL  |G|PS|D|A|PCD|PWT|US|RW|P|
```
 - P = Present: 1 = Page is actually in physical memory at the moment. 0 = Is not (Page fault will occure on call)
 - RW = Read/Write: 1 = Read/Write allowed; 0 = Read only
 - US = User/Supervisor: 1 = Page can be accessed by all; 0 = Only Supervisor can access
 - PWT = Write-Through: 1 = Write-Trough caching is enabled; 0 = Is not enabled
 - PCD = Cache Disabled: 1 = Page will not be cached; 0 = It will be cached 
 - A = Accessed: Used to discorver if this was read.
 - D = Dirty: Used to determine if page has been written to.
 - AVL = Available: 1 = It is available; 0 = It is not available.
 - PAT = Page attribute Table: 1 = PAT is supported; 0 = Pat is not supported.
 - PS = ?????
 - XD = Execute Disabled
 - PK = Protection key: 1 = ???;0 = Protection key is not used
## Setup 1gb paging
Lets setup paging and set up some entrys, to have the kernel at the virual address 0xffff800000000000.
The actual kernel sits at the physical address 0x200000.

We want to set up a 1gb pagin struct. So we need 2 tables. We decided, that we set out paging tables at 0x70000.
So first lets clear that memory area.
``` assembly
    mov edi, 0x70000
    xor eax, eax
    mov ecx, 0x10000/4
    rep stosd	; These 4 lines zeros 10000 bytes starting from 0x70000 (stosb = stores a double word (4byte) from eax into edi and increments it)
                ; Because we our table will be placed at location 0x70000
```
``` assembly
                                        ; We implement 1gb page lookup
                                        ; Each entry represent 512gb. Table needs 4kb of space = 512 entris which 8byte (64bits)
        mov dword[0x70000],0x71003      ; PLM4: Bits of address = 0x71000(Next lookup table); P = 1; RW = 1; U = 0(Only ring 0 access);

                                        ; PDP Table. Each entry represents 1gb. Has 512 entrys.
        mov dword[0x71000],10000011b    ; Bits of address = 0; PS = 1(This is 1GB page translation); P = 1; RW = 1; U = 0;

        mov eax,(0xffff800000000000>>39); Shift value 39 bits to the right, so bit 39 is now at bit index 0;
        and eax, 0x1ff                  ; 0x1ff = 9 bit. So we select the 9 bit from the value 0xffff800000000000 and take this as entry for the next table.
        mov dword[0x70000+eax*8], 0x72003; eax has the 9 bit selected from 0xffff800000000000 and we multiply with 8 because each entry has 8 bytes
        mov dword[0x72000], 10000011b   ; Bits of address = 0; PS = 1; P = 1; RW = 1; U = 0;
                                        ; So with this the address 0xffff8000000200000 gets transalted to the physical address 0x200000 where the kernel is at

```
``` 
0xffff800000000000 =                1111 1111 1111 1111 1000 0000 0000 0000
                                    0000 0000 0000 0000 0000 0000 0000 0000

eax, 0xffff800000000000 >> 39 =     0000 0000 0000 0000 0000 0000 0000 0000
                                    0000 0001 1111 1111 1111 1111 0000 0000


and eax, 0x1ff	=                   0000 0000 0000 0000 0000 0000 0000 0000
                                    0000 0001 1111 1111 1111 1111 0000 0000

                                AND
                                    0000 0000 0000 0000 0000 0000 0000 0000
                                    0000 0000 0000 0000 0000 0001 1111 1111

                                =   0000 0000 0000 0000 0000 0001 0000 0000

                                =   256. So 256 is the index for the next page table
```

We remember, that on the first lookup table bit 39 - 48 is taken as the index into the next table.
So what we are doing here is we get bit 39 - 48 by shifting right 39 and then select 9 bit from it and store it into eax.
Then we write a table entry at the index in eax (multiplied by 8, because each table length is 8 bytes). 
So now the address 0xffff8000000200000 always gets translated to 0x200000.

**Remember: each process has its own paging tables**
