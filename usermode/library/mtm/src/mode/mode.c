/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

/**
 * \file mode.c
 * \brief Transaction execution modes
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 *
 */

#include "mtm_i.h"
#include "mode/dtable.h"

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
	// freud : fprintf(stderr, "mode = %s\n",str);
	for (i=0; i<MTM_NUM_MODES; i++) {
		if (strcasecmp(str, mtm_mode_str[i]) == 0) {
			// freud : fprintf(stderr, "mtm_mode = %d\n",i);
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
