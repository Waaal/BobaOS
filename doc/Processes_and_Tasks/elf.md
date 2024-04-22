# Executable Linkable Format
Elf is a executable file with a structure. ELF Files are object files that contain symbols that point to data or functions. 
Elf files can be dynamically linked at runtime. A modern kernel should be able to load and execute ELF files. A ELF file has a header tha contains various information like the entry point.


A ELF file contains a list of symbols with positioning information of where that symbol is located in the binary. 
>[!NOTE]
> A symbol in a assembly files are lables
> A symbol in a c file are functions and global variables


A ELF file also contains info about all the sections (text, data etc) so that we as the kernel can load and treat them differently.
With ELF files DLLs (shared libraries) are also possbile because ELF files can contain references to symbol that are not known yet. So the kernel/loader can find these unknown symbols when loading a ELF file and resolve them.

## Inside a ELF file
A ELF file is structured like this

| |
| ------ |
| ELF header |
| Program header table |
| .text |
| .rodata |
| .data |
| ... |
| Section header table |


**ELF header:** Contains the elf header, there is only one efl header at the beginning of a file


**Program header table:** Contains a array of different program headers


**Section header table:** Contains a array of different section headers


**.text:** contains code


**.data:** contains data


**.rodata:** contains read only data

### ELF header
The ELF header contains all of the important info and positions of this file and a magic number at the start to identify that this is a ELF file. 

| Index (32 bit) | Index (64 bit) | Size (32 bit) | Size (64 bit) | Description |
| ------ | ------ | ------ | ------ | ------ |
| 0 | 0 | 16 | 16 | ELF file identifier (own struct) |
| 16 | 16 | 2 | 2 | Type |
| 18 | 18 | 2 | 2 | Machine |
| 20 | 20 | 4 | 4 | ELF Version |
| 24 | 24 | 4 | 8 | Program entry address |
| 28 | 32 | 4 | 8 | Program header table offset |
| 32 | 40 | 4 | 8 | Section table offset |
| 36 | 48 | 4 | 4 | Flags |
| 40 | 52 | 2 | 2 | ELF Header size |
| 42 | 54 | 2 | 2 | Program header table  entry size |
| 44 | 56 | 2 | 2 | Number of entries in program header table |
| 46 | 58 | 2 | 2 | Section table  entry size |
| 48 | 60 | 2 | 2 | Number of entries in section table |
| 50 | 62 | 2 | 2 | ??? Something to do with the string table |


**ELF file identifier:**
| Index | Size | Description |
| ------ | ------ | ------ |
| 0 | 4 | Magic number (0x7F, 'E', 'L', 'F') |
| 4 | 1 | Class (1 = 32 bit, 2 = 64 bit) |
| 5 | 1 | Data (1 = little endian, 2 = big endian) |
| 6 | 1 | ELF header version |
| 7 | 1 | OS ABI |
| 8 | 8 | Unused/Padding |



**Type:** Identifies the taype of current ELF file
- ET_NONE = 0: No file type
- ET_REL = 1: Relocatable file
- ET_EXEC = 2: Executable file
- ET_DYN = 3: Shared object file
- ET_CORE = 4: Core file (A snapshot of the process right before it crashed or dumped)
- ET_LOPROC = 0xff00: Processor-specific
- ET_HIPROC = 0xffff: Processor-specific


**Program entry address:** Virtual address of entry point of program


**Program header table offset:** Offset into ELF file in bytes


**Section header table offset:** Offset into ELF file in bytes


### Program header

### Section header

>[!NOTE]
>ELF file specification pdf:
>https://refspecs.linuxfoundation.org/elf/elf.pdf