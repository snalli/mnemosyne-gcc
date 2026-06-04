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

#include <stdio.h>
#include <mnemosyne.h>
#include <pcm.h>

// These two must come before all other includes; they define things like w_entry_t.
#include "pwb_i.h"
#include "mode/pwb-common/barrier-bits.h"
#include <barrier.h>


/*
 * Called by the CURRENT thread to store a word-sized value.
 */
void 
mtm_pwbetl_store(mtm_tx_t *tx, volatile mtm_word_t *addr, mtm_word_t value)
{
	pwb_write_internal(tx, addr, value, ~(mtm_word_t)0, 1);
}


/* freud : use this as an entry point into  the library for RSTM 
 * Called by the CURRENT thread to store part of a word-sized value.
 */
void 
mtm_pwbetl_store2(mtm_tx_t *tx, volatile mtm_word_t *addr, mtm_word_t value, mtm_word_t mask)
{
	pwb_write_internal(tx, addr, value, mask, 1);
}

/*
 * Called by the CURRENT thread to load a word-sized value.
 */
mtm_word_t 
mtm_pwbetl_load(mtm_tx_t *tx, volatile mtm_word_t *addr)
{
	return pwb_load_internal(tx, addr, 1);
}


DEFINE_LOAD_BYTES(pwbetl)
DEFINE_STORE_BYTES(pwbetl)

FOR_ALL_TYPES(DEFINE_READ_BARRIERS, pwbetl)
FOR_ALL_TYPES(DEFINE_WRITE_BARRIERS, pwbetl)
