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
 * \file mode.h
 * \brief Transaction execution modes
 *
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 *
 *
 * To add a new transaction dynamic execution mode:
 * 
 * 1) mode.c
 * 
 *    - Add extern mtm_dtable_t mtm_XXX_dtable;
 *
 * 2) mode.h
 *
 *    - Under FOREACH_MODE(ACTION), add ACTION(XXX)
 *    - Under mtm_mode_t, add MTM_MODE_XXX = YYY
 *
 * 3) Look into pwbnl as a template
 *
 *    - barrier.c: at the end of file there are barrier definitions
 * 
 */

#ifndef _MODE_H_891AKK
#define _MODE_H_891AKK


# define FOREACH_MODE(ACTION)   \
    ACTION(pwbetl)               


typedef enum {
	MTM_MODE_none  = -1,
	MTM_MODE_pwbnl = 0,
	MTM_MODE_pwbetl = 1,
	MTM_NUM_MODES
} mtm_mode_t;

/* This type is private to the STM implementation.  */
struct mtm_mode_data_s;

typedef struct mtm_mode_data_s mtm_mode_data_t;

mtm_mode_t mtm_str2mode(char *str);
char *mtm_mode2str(mtm_mode_t mode);

#endif /* _MODE_H_891AKK */
