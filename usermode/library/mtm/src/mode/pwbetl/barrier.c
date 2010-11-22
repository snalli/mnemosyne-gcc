#include <stdio.h>
#include <mnemosyne.h>
#include <pcm.h>

// These two must come before all other includes; they define things like w_entry_t.
#include "pwb_i.h"
#include "mode/pwb-common/barrier-bits.h"


/*
 * Called by the CURRENT thread to store a word-sized value.
 */
void 
mtm_pwbetl_store(mtm_tx_t *tx, volatile mtm_word_t *addr, mtm_word_t value)
{
	pwb_write_internal(tx, addr, value, ~(mtm_word_t)0, 1);
}


/*
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
