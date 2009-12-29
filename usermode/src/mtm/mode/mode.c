#include "mtm_i.h"
#include "mode/vtable.h"
#include "mode/vtable_macros.h"
#if 0
#include "barrier.h"
#include "beginend.h"
#include "memcpy.h"
#include "memset.h"
#endif

extern mtm_vtable_t STR2(mtm_wbetl, _vtable);

#define ACTION(mode) &mtm_##mode##_vtable,
mtm_vtable_group_t perfVtables = { FOREACH_MODE (ACTION) };

#undef ACTION


mtm_vtable_group_t *defaultVtables = NULL;
