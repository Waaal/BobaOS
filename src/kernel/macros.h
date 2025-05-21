#ifndef MACROS_H
#define MACROS_H

#include <stddef.h>

#define RETNULL(c) if ((c) == NULL) return NULL;
#define RETERROR(c) if ((c) < 0) return c;

#endif
