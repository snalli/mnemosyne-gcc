#include "mtm_i.h"
#include <vtable.h>
#include <vtable_macros.h>
#include "local.h"
#include "barrier.h"
#include "beginend.h"
#include "memcpy.h"
#include "memset.h"

#undef DEFINE_VTABLE_MEMBER
#define DEFINE_VTABLE_MEMBER(result, function, args, ARG)   \
    ARG##function,

#define INSERT_MODIFIER(str1, str2) str1##str2

txninterface_t INSERT_MODIFIER (mtm_wbetl, _vtable) =
{
	FOREACH_OUTER_FUNCTION (DEFINE_VTABLE_MEMBER, mtm_wbetl_)
	_GEN_READ_BARRIERS_LIST (mtm_wbetl_R, mtm_wbetl_R, mtm_wbetl_R, mtm_wbetl_RfW)
	_GEN_WRITE_BARRIERS_LIST (mtm_wbetl_W, mtm_wbetl_W, mtm_wbetl_W)
	_ITM_FOREACH_MEMCPY (DEFINE_VTABLE_MEMBER, INSERT_MODIFIER (mtm_wbetl, _))
	_ITM_FOREACH_LOG_TRANSFER (DEFINE_VTABLE_MEMBER, mtm_local_)
	_ITM_FOREACH_MEMSET (DEFINE_VTABLE_MEMBER, INSERT_MODIFIER (mtm_wbetl, _))
	_ITM_FOREACH_MEMMOVE (DEFINE_VTABLE_MEMBER, INSERT_MODIFIER (mtm_wbetl, _))
};


#define ACTION(mode) &mode##_vtable,
txninterfacegroup_t perfVtables = { FOREACH_MODE (ACTION) };

#undef ACTION


txninterfacegroup_t *defaultVtables = NULL;
