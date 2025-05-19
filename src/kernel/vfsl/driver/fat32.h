#ifndef FAT32_H
#define FAT32_H

#include "vfsl/virtualFilesystemLayer.h"

#define FAT32_SIGNATURE_STRING = "FAT32   "

struct fileSystem* insertIntoFileSystem();

#endif
