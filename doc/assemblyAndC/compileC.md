# Compile C
C gets compiled into a object file. Later that object file gets linked with other object files and a bin file gets created.

We need to create a **freestanding** binary file. That means that this file can run on a machine with no operating system and depends on no host.

### Compile C
```
CrossCompilerName-gcc $(FLAGS) -std=gnu99 -c ./src/kernel/kernel.c -o ./build/kernel.o
```
This returns a compiled object file.

**-c** Source file

**-o** Output file location and name

### $(FLAGS)
$(FLAGS) is a variable and needs to be replaced with the following:

#### Needed flags
| Flag | Explenation |
| ------ | ------ |
| **-ffreestanding** | Tells the compiler, that there are no default librarys are not present |
| **-nostdlib** | Do not use the standard system startup files or libraries when linking. No startup files and only the libraries you specify will be passed to the linker |
| **-nodefaultlibs**| Use in combination with nostdlib. Dont use system default librarys |
| **-nostartfiles** | No commad line arguments will be passed to main and enviroment variables are not available |
| **-fno-builtin** | gcc will not try to replace library functions with builtin compiled code. |

#### Usefull flags
| Flag | Explenation |
| ------ | ------ |
| **-wall** | Enables all compiler warning while compiling |
| **-O0** | Turs off optimization. Help while debugging, so that the compiler does not do unexpected things with our code while optimizing |
| **-g** | Enable Debug information in the output file. For example it puts all functions to thy symbole table and we can use them later with the debugger |
| **-Wno-unused-functions** | Disables warning that I didnt use a function |
| **-Wno-unused-lable** | Disables warning that I didnt use a variable |
| **-Wno-unused-parameter** | Disables warning that I didnt use a variable |

#### Alignment flags
| Flag | Explenation |
| ------ | ------ |
| **-falign-jumps** | Align jumps to a power-of-two boundary |
| **-falign-functions** | Align functions to a power-of-two boundary |
| **-falign-labels** | Align lables to a power-of-two boundary |
| **-falign-loops** | Align loops to a power-of-two boundary |

#### Optimization Flags
| Flag | Explenation |
| **fomit-frame-pointer** | Removes frame pointer from small functions, that dont need one. |
| **-finline-functions** | Integrate all simple functions into their callers.  |
| **-fstrength-reduce** | Part smaller loops into hardcoded code and dont use loop  |

### Include header files
If we want to include header files and didnt want to specify the relative path in the C-File, then we need to tell the compiler where to look for this header files with the -I tag.

**Example relative path:**
``` c
#include "../kernel.h"
#include "../memory/memory.h"
#include "../io/io.h"

//Here the relative path is given, we dont need to include them.
```

**Example no relative path:**
``` c
#include "kernel.h"
#include "memory/memory.h"
#include "io/io.h"

//Here no relative path is given, we need the include flag
```

```
i686-elf-gcc -I./src/kernel/ -I./src/kernel/memory/ -I./src/kernel/io/ $(FLAGS) -std=gnu99 -c ./src/kernel/kernel.c -o ./build/kernel.o
```

Here we need to specify where to look for the header files with the -I tag.