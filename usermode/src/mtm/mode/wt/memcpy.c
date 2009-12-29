#include "mtm_i.h"
#include "mode/wt/barrier.h"
#include "mode/wt/memcpy.h"


FORALL_MEMCOPY_VARIANTS(MEMCPY_DEFINITION, wt)
FORALL_MEMMOVE_VARIANTS(MEMMOVE_DEFINITION, wt)
