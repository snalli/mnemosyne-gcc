/**
 * \file mode.c
 * \brief Transaction execution modes
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 *
 */

#include "mtm_i.h"
#include "mode/dtable.h"
#include "mode/dtablegen.h"

extern mtm_dtable_t mtm_pwbnl_dtable;
extern mtm_dtable_t mtm_pwbetl_dtable;

#define ACTION(mode) &mtm_##mode##_dtable,
mtm_dtable_group_t normal_dtable_group = { FOREACH_MODE (ACTION) };

#undef ACTION

#define ACTION(mode) #mode,
char *mtm_mode_str[] = {
  FOREACH_MODE(ACTION)
};
#undef ACTION

mtm_dtable_group_t *default_dtable_group = NULL;


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


char * 
mtm_mode2str(mtm_mode_t mode)
{
	if (mode > MTM_MODE_none && mode < (int) MTM_NUM_MODES) {
		return mtm_mode_str[mode];
	}
	return NULL;
}
