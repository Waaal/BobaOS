#ifndef MACROS_H
#define MACROS_H

#include <stddef.h>

#define RETNULL(c) if ((c) == NULL) return NULL;
#define RETNULLERROR(c, err) if ((c) == NULL) return err;
#define RETERROR(c) if ((c) < 0) return c;
#define RETERRORDIFF(c, e) if ((c) < 0) return e;
#define GOTOERROR(c, label) if ((c) < 0) goto label;

#define RETNULLSETERROR(c, errCode, errPtr) \
do { \
if ((c) == NULL) { \
if ((errPtr) != NULL) *(errPtr) = errCode \
return NULL; \
} \
} while (0)

#endif
