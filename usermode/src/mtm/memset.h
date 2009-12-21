#ifndef _MEMSET_H
#define _MEMSET_H


static void
do_memset(mtm_thread_t *td, uintptr_t dst, int c, size_t size, mtm_write_lock_fn W)
{
	uintptr_t dofs = dst & (CACHELINE_SIZE - 1);
	mtm_cacheline_mask_pair dpair;

	if (size == 0) {
		return;
	}	

	dst &= -CACHELINE_SIZE;

	if (dofs != 0) {
		size_t dleft = CACHELINE_SIZE - dofs;
		size_t min = (size <= dleft ? size : dleft);

		dpair = W(td, dst);
		*dpair.mask |= (((mtm_cacheline_mask)1 << min) - 1) << dofs;
		memset (&dpair.line->b[dofs], c, min);
		dst += CACHELINE_SIZE;
		size -= min;
	}

	while (size >= CACHELINE_SIZE) {
		dpair = W(td, dst);
		*dpair.mask = -1;
		memset (dpair.line, c, CACHELINE_SIZE);
		dst += CACHELINE_SIZE;
		size -= CACHELINE_SIZE;
	}

	if (size != 0) {
		dpair = W(td, dst);
		*dpair.mask |= ((mtm_cacheline_mask)1 << size) - 1;
		memset (dpair.line, c, size);
	}
}


#define MEMSET_DEFINITION(PREFIX, VARIANT)                                     \
void _ITM_CALL_CONVENTION mtm_##PREFIX##memset##VARIANT(mtm_thread_t *td,  \
                                                        void *dst,             \
                                                        int c,                 \
                                                        size_t size)           \
{                                                                              \
  do_memset (td, (uintptr_t)dst, c, size, PREFIX##VARIANT);	                   \
}



#define MEMSET_DECLARATION(PREFIX, VARIANT)                                    \
void _ITM_CALL_CONVENTION mtm_##PREFIX##memset##VARIANT(mtm_thread_t * td, \
                                                        void *dst,             \
                                                        int c, size_t size);

#define FORALL_MEMSET_VARIANTS(ACTION, PREFIX)                                 \
ACTION(PREFIX, W)                                                              \
ACTION(PREFIX, WaR)                                                            \
ACTION(PREFIX, WaW)

FORALL_MEMSET_VARIANTS(MEMSET_DECLARATION, wbetl_)

#endif
