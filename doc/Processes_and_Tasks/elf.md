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

### ELF header (elf32/64_Ehdr)
The ELF header contains all of the important info and positions of this file and a magic number at the start to identify that this is a ELF file. 

| Index (32 bit) | Index (64 bit) | Size (32 bit) | Size (64 bit) | Description |
| ------ | ------ | ------ | ------ | ------ |
| 0 | 0 | 16 | 16 | e_ident |
| 16 | 16 | 2 | 2 | e_type |
| 18 | 18 | 2 | 2 | e_machine |
| 20 | 20 | 4 | 4 | e_version |
| 24 | 24 | 4 | 8 | e_entry |
| 28 | 32 | 4 | 8 | e_phoff |
| 32 | 40 | 4 | 8 | e_shoff |
| 36 | 48 | 4 | 4 | e_flags |
| 40 | 52 | 2 | 2 | e_ehsize |
| 42 | 54 | 2 | 2 | e_phentsize |
| 44 | 56 | 2 | 2 | e_phnum |
| 46 | 58 | 2 | 2 | e_shentsize |
| 48 | 60 | 2 | 2 | e_shnum |
| 50 | 62 | 2 | 2 | e_shstrndx |


**e_ident:**
| Index | Size | Description |
| ------ | ------ | ------ |
| 0 | 4 | Magic number (0x7F, 'E', 'L', 'F') |
| 4 | 1 | Class (1 = 32 bit, 2 = 64 bit) |
| 5 | 1 | Data (1 = little endian, 2 = big endian) |
| 6 | 1 | ELF header version |
| 7 | 1 | OS ABI |
| 8 | 8 | Unused/Padding |


**e_type:** Identifies the type of current ELF file
- ET_NONE = 0: No file type
- ET_REL = 1: Relocatable file
- ET_EXEC = 2: Executable file
- ET_DYN = 3: Shared object file
- ET_CORE = 4: Core file (A snapshot of the process right before it crashed or dumped)
- ET_LOPROC = 0xff00: Processor-specific
- ET_HIPROC = 0xffff: Processor-specific


**e_machine:** Required architecture for current file
- EM_NONE = 0: No machine
- EM_M32 = 1: AT&T WE 32100
- EM_SPARC = 2: SPARC
- EM_386 = 3: Intel
- EM_68K = 4: Motorola 68000
- EM_88K = 5: Motorola 88000
- EM_860 = 7: Intel 80860
- EM_MIPS = 8: MIPS RS3000 Big-Endian
- EM_MIPS_RS4_BE = 10: MIPS RS4000 Big-Endian
- RESERVED = 11-16: Reserved for future


**e_version:** Version of the object file. 1 = original file. Extension will create new versions with higher numbers. Value EV_CURRENT is given as 1 or above.
- EV_NONE = 0: Invalid version
- EV_NONE = 1: Current version


**e_entry:** Virtual address of entry point of program.


**e_phoff:** Offset of program header table into ELF file in bytes. If there is no Program header table than 0.


**e_shoff:** Offset of section table into ELF file in bytes.


**e_flags:** Processor specific flags associated with the file.


**e_ehsize:** Size of the ELF header in bytes.


**e_phentsize:** Size of a entry in the program header table in bytes.


**e_phnum:** Number of entries in program header table.


**e_shentsize:** Size of a entry in the section table in bytes.


**e_shnum:** Number of entries in section table.


**e_shstrndx:** ????? Has something to do with the string table 


### Program header

### Section header

>[!NOTE]
>ELF file specification pdf:
>https://refspecs.linuxfoundation.org/elf/elf.pdf