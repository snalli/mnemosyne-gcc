#include "mtm_i.h"
#include "mode/pwb/barrier.h"
#include "mode/pwb/memcpy.h"


FORALL_MEMCOPY_VARIANTS(MEMCPY_DEFINITION, pwb)
FORALL_MEMMOVE_VARIANTS(MEMMOVE_DEFINITION, pwb)
