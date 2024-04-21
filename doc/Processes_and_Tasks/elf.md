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

| Index | Size | Description |
| ------ | ------ | ------ |
| 0 | 16 | ELF file identifier (own struct) |
| 16 | 2 | Type |
| 18 | 2 | Machine |
| - | - | TODO Finish this table |


### Program header

### Section header

>[!NOTE]
>ELF file specification pdf:
>https://refspecs.linuxfoundation.org/elf/elf.pdf