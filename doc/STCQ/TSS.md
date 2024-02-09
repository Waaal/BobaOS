# Task State Segment
*Note: This is still a early article. The information may be wrong and/or incomplete.*

A TSS is a struct that lives in memory. It holds information about a Task.

In **protected mode** the TSS is used for Hardware Task Switching.

In **long mode** the TSS is used to change the Stack Pointer after an interrupt or permission level change.

The TSS is also needed for software multitasking. Because each CPU core has its own TSS in a multitasking scenario.
The TSS is written in the GDT. 

### TSS in a GDT
The TSS also has a entry in the GDT. This entry is 128bit long and looks like this:
```
127    96 95        64 63     56 55   52 51   48 47           40 39       32 31           16 15           0
 | ///// |    Base    |   Base  | Flags | Limit | Access Bytes  |    Base   |      Base     |     Limit   |
```

## Implementation  