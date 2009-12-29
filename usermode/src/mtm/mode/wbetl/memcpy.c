#include "mtm_i.h"
#include "mode/wbetl/barrier.h"
#include "mode/wbetl/memcpy.h"


FORALL_MEMCOPY_VARIANTS(MEMCPY_DEFINITION, wbetl)
FORALL_MEMMOVE_VARIANTS(MEMMOVE_DEFINITION, wbetl)
