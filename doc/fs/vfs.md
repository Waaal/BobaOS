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

## Implementation
Here is a example on how this layers structure could look like
``` c
#define BOBAOS_DISK_TYPE_REAL 0

//Disk represents a disk on a system
struct disk
{
    BOBAOS_DISK_TYPE type;
    int sector_size;
    int id;

    //Holds the filesystem bound to the disk
    struct filesystem* filesystem;

    //The private data is used by the filesystem, to store data about 
    //this specific disk. 
    //So data and filesystem blueprint is not connected
    void* fs_private;
};


//Open function is implemented by the filesystem, VFS calls this if it should open a file
typedef void*(*FS_OPEN_FUNCTION)(struct disk* disk, struct path_part* path, FILE_MODE mode);

//Resolve function finds out, if this file system can manage the disk
typedef int (*FS_RESOVLE_FUNCTION)(struct disk* disk);

//struct which each filesystem can use
struct filesystem
{
    // Filesystem should return 0 from resolve if the provided disk is using its file system
    FS_RESOVLE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    //write, read, close

    //Name of the filesystem
    char name[20];
};


//Now if a disk gets insertet into the system the kernel can go trough all filesystems it has, call the resolve function 
//and if this filesystem can resolve the disk, the filesystem is bound to the disk

//now if we call the open function on a disk, the VFS can do the following:

//disk->filesystem->open();
```