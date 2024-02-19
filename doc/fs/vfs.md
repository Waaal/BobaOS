# Virtual File System
VFS is a layer that allows the kernel to support a lot of differnt file systems. Because the VFS abstracts out low level file system code and provides a interface for reading and writing.
With VFS file system functionality can be loaded and unloaded from the kernel (Driver).

So with VFS the interface to interact with the file system remains the same for all file systems.

*Note: Abstraction works with function pointer*

## Step by step when a disk gets inserted
- Resolving the file system
- Disk binds to file system implementation

### Resolving the file system
Kernel poll each file system ans asks if it can manage this disk. So kernel goes to every file system and calls its resolve function. The resolve function of a file system will look at the header of the disk and tell the kernel if it can manage it.

### Disk binds to file system implementation
If the kernel now try to access this disk and get a file, it will access it with the bound file system to this disk.