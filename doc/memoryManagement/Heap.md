# Heap
The Heap is another memory managment system besides paging. Because with paging we can only return a full page but we often dont need a whole page. For example a program needs 30 byte space for a array. It would be a waste to reserve a whole page for this 30 bytes.

For this the heap comes into place. The heap can temporary give a amount of bytes to a process if the process asks for it for example with malloc. When the process frees this memory or closes the heap then takes this back and can give it to something else.

The Heap keeps track of what memory and how much he has given away. That ensures that the Heap never gives away memory that is currently occupide.

Normally each process has its own heap. So for example if a process starts, the MMU could allocate a extra 4kb page to this process for its heap. 

*Note: Some kernel heap implementations will still use block sized heap allocation in 4096 byte blocks, because it is extremly fast and kernel needs normaly bigger sized memory blocks, so giving 4096 blocks to the kernel is pretty standard*