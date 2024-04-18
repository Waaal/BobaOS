# Sections

A normal programm is seperated in differente sections (if it is compiled with a compiler, if we write assembly we need to seperate our sections manually with the section directive).

These sections are **text**, **bss**, and **data**.

| Section | Descritpion |
| ------ | ------ |
| text | Here the code of a program is stored. In a more advanced process loader that can load elf files this sections should be read only, because the process should not be able to modify its own code while running. |
| bss | In this section are global and static variables placed which dont have a value at compile time |
| data | In this section are global and static variables placed which have a value at compile time |

*Remeber: Variables that are defined in the scope of a function are placed on the stack (if they are not static lel)*