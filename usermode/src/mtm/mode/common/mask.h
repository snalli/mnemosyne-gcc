/*!
 * \file
 * Defines macros and functions related to the masking of written values at byte
 * granularity onto word-size snapshots of memory.
 *
 * \author Andres Jaan Tack
 */
#ifndef MASK_H_4NASQJFA
#define MASK_H_4NASQJFA


/*!
 * A word with all bits set (a.k.a the 1's compliment of 0x0).
 */
static const mtm_word_t whole_word_mask = ~(mtm_word_t)0;


/*!
 * Returns a word which is the result of masking one value on top of another.
 *
 * \param old_value is the value onto which the new value is masked.
 * \param new_value contains some bits which are written over old_value.
 * \param mask determines, with set (1) bits, which bits of new_value
 *  replace the corresponding bits of old_value.
 *
 * \return the masked combination of old_value and new_value (per mask).
 */
static inline
mtm_word_t masked_word(mtm_word_t old_value, mtm_word_t new_value, mtm_word_t mask)
{
	return (old_value & ~mask) | (new_value & mask);
}


/*!
 * Populates the given write-set entry by masking a new value on top of the value
 * already resident in the entry (if it exists).
 *
 * \param entry will be written with the new, combined value and mask after
 *  new_value is masked on top of the value contained herein. An entry with
 *  entry->mask == 0 indicates a clean (unwritten) entry.
 * \param written address indicates the address at which the write takes place.
 *  This is <em>not</em> written to entry.
 * \param this_value contains the bits which are being newly written.
 * \param this_mask is a bitmask indicating with set (1) bits which bits of
 *  this_value are to be written over the existing entry.
 *
 * \return the new value stored in the write-set entry.
 */
static inline
mtm_word_t mask_new_value(w_entry_t* entry, const volatile mtm_word_t* written_address, const mtm_word_t this_value, const mtm_word_t this_mask)
{
	mtm_word_t new_value = this_value;
	mtm_word_t new_mask = entry->mask | this_mask;
	
	if (this_mask != whole_word_mask) {
		if (entry->mask == 0)
			entry->value = ATOMIC_LOAD(written_address);
		
		new_value = masked_word(entry->value, this_value, this_mask);
	}
	
	entry->value = new_value;
	entry->mask  = new_mask;
	
	return entry->value;
}

#endif /* end of include guard: MASK_H_4NASQJFA */
