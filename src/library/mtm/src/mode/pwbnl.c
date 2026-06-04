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
mtm_pwbnl_store(mtm_tx_t *tx, volatile mtm_word_t *addr, mtm_word_t value)
{
	pwb_write_internal(tx, addr, value, ~(mtm_word_t)0, 0);
}


/*
 * Called by the CURRENT thread to store part of a word-sized value.
 */
void 
mtm_pwbnl_store2(mtm_tx_t *tx, volatile mtm_word_t *addr, mtm_word_t value, mtm_word_t mask)
{
	pwb_write_internal(tx, addr, value, mask, 0);
}

/*
 * Called by the CURRENT thread to load a word-sized value.
 */
mtm_word_t 
mtm_pwbnl_load(mtm_tx_t *tx, volatile mtm_word_t *addr)
{
	return pwb_load_internal(tx, addr, 0);
}

DEFINE_LOAD_BYTES(pwbnl)
DEFINE_STORE_BYTES(pwbnl)


void _ITM_nl_load_bytes(const void *src, void *dest, size_t size)
{
    mtm_tx_t *tx = mtm_get_tx();
    volatile uint8_t *saddr=((volatile uint8_t *) src);
    mtm_pwbnl_load_bytes(tx, saddr, dest, size);
}

void _ITM_nl_store_bytes(const void *src, void *dest, size_t size)
{
    mtm_tx_t *tx = mtm_get_tx();
    volatile uint8_t *daddr=((volatile uint8_t *) dest);
    mtm_pwbnl_store_bytes(tx, daddr, (uint8_t*) src, size);
}
