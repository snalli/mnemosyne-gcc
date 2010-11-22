#include "mtm_i.h"
#include "mode/vtable.h"
#include "mode/vtable_macros.h"

extern mtm_vtable_t STR2(mtm_pwbnl, _vtable);
extern mtm_vtable_t STR2(mtm_pwbetl, _vtable);

#define ACTION(mode) &mtm_##mode##_vtable,
mtm_vtable_group_t perfVtables = { FOREACH_MODE (ACTION) };

#undef ACTION

#define ACTION(mode) #mode,
char *mtm_mode_str[] = {
  FOREACH_MODE(ACTION)
};
#undef ACTION

mtm_vtable_group_t *defaultVtables = NULL;


mtm_mode_t 
mtm_str2mode(char *str)
{
	int i;

	for (i=0; i<MTM_NUM_MODES; i++) {
		if (strcasecmp(str, mtm_mode_str[i]) == 0) {
			return (mtm_mode_t) i;
		}
	}
}
