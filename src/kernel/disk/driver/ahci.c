#include "ahci.h"

#include <stddef.h>

struct ahciPrivate {

};

static struct diskDriver driver = {
    .type = DISK_DRIVER_TYPE_AHCI,
    .getInfo = NULL,
    .read = NULL,
    .write = NULL
};

struct diskDriver* registerAHCI()
{
    return &driver;
}