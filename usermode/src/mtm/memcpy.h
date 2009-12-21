#ifndef _MEMCPY_H
#define _MEMCPY_H

static void
do_memcpy (mtm_thread_t *td, uintptr_t dst, uintptr_t src, size_t size,
           mtm_write_lock_fn W, mtm_read_lock_fn R)
{
	uintptr_t dofs = dst & (CACHELINE_SIZE - 1);
	uintptr_t sofs = src & (CACHELINE_SIZE - 1);
	mtm_cacheline *sline;
	mtm_cacheline_mask_pair dpair;

	if (size == 0) {
		return;
	}

	dst &= -CACHELINE_SIZE;
	src &= -CACHELINE_SIZE;

	if (dofs == sofs) {
		if (sofs != 0) {
			size_t sleft = CACHELINE_SIZE - sofs;
			size_t min = (size <= sleft ? size : sleft);

			dpair = W(td, dst);
			sline = R(td, src);
			*dpair.mask |= (((mtm_cacheline_mask)1 << min) - 1) << sofs;
			memcpy (&dpair.line->b[sofs], &sline->b[sofs], min);
			dst += CACHELINE_SIZE;
			src += CACHELINE_SIZE;
			size -= min;
		}

		while (size >= CACHELINE_SIZE) {
			dpair = W(td, dst);
			sline = R(td, src);
			*dpair.mask = -1;
			mtm_cacheline_copy (dpair.line, sline);
			dst += CACHELINE_SIZE;
			src += CACHELINE_SIZE;
			size -= CACHELINE_SIZE;
		}

		if (size != 0) {
			dpair = W(td, dst);
			sline = R(td, src);
			*dpair.mask |= ((mtm_cacheline_mask)1 << size) - 1;
			memcpy (dpair.line, sline, size);
		}
	} else {
		mtm_cacheline c;
		size_t sleft = CACHELINE_SIZE - sofs;

		sline = R(td, src);
		if (dofs != 0) {
			size_t dleft = CACHELINE_SIZE - dofs;
			size_t min = (size <= dleft ? size : dleft);

			dpair = W(td, dst);
			*dpair.mask |= (((mtm_cacheline_mask)1 << min) - 1) << dofs;
			if (min <= sleft) {
				memcpy (&dpair.line->b[dofs], &sline->b[sofs], min);
				sofs += min;
			} else {
				memcpy (&c, &sline->b[sofs], sleft);
				src += CACHELINE_SIZE;
				sline = R(td, src);
				sofs = min - sleft;
				memcpy (&c.b[sleft], sline, sofs);
				memcpy (&dpair.line->b[dofs], &c, min);
			}
			sleft = CACHELINE_SIZE - sofs;

			dst += CACHELINE_SIZE;
			size -= min;
		}

		while (size >= CACHELINE_SIZE) {
			memcpy (&c, &sline->b[sofs], sleft);
			src += CACHELINE_SIZE;
			sline = R(td, src);
			memcpy (&c.b[sleft], sline, sofs);

			dpair = W(td, dst);
			*dpair.mask = -1;
			mtm_cacheline_copy (dpair.line, &c);

			dst += CACHELINE_SIZE;
			size -= CACHELINE_SIZE;
		}

		if (size != 0) {
			dpair = W(td, dst);
			*dpair.mask |= ((mtm_cacheline_mask)1 << size) - 1;
			if (size <= sleft) {
				memcpy (dpair.line, &sline->b[sofs], size);
			} else {
				memcpy (&c, &sline->b[sofs], sleft);
				src += CACHELINE_SIZE;
				sline = R(td, src);
				memcpy (&c.b[sleft], sline, size - sleft);
				memcpy (dpair.line, &c, size);
			}
		}
	}
}


static void
do_memmove (mtm_thread_t *td, uintptr_t dst, uintptr_t src, size_t size,
            mtm_write_lock_fn W, mtm_read_lock_fn R,
            mtm_write_lock_fn WaW, mtm_read_lock_fn RfW)
{
	uintptr_t dleft, sleft, sofs, dofs;
	mtm_cacheline *sline;
	mtm_cacheline_mask_pair dpair;
  
	if (size == 0) {
		return;
	}

	/* The co-aligned memmove below doesn't work for DST == SRC, so filter
	 * that out.  It's tempting to just return here, as this is a no-op move.
	 * However, our caller has the right to expect the locks to be acquired
	 * as advertized.  */
	if (__builtin_expect (dst == src, 0))
	{
		/* If the write lock is already acquired, nothing to do.  */
		if (W == WaW) {
			return;
		}
		/* If the destination is protected, acquire a write lock.  */
		if (W != mtm_null_write_lock) {
			R = RfW;
		}
		/* Notice serial mode, where we don't acquire locks at all.  */
		if (R == mtm_null_read_lock) {
			return;
		}

		dst = src + size;
		for (src &= -CACHELINE_SIZE; src < dst; src += CACHELINE_SIZE) {
			R(td, src);
		}
		return;
	}

	/* Fall back to memcpy if the implementation above can handle it.  */
	if (dst < src || src + size <= dst) {
		do_memcpy (td, dst, src, size, W, R);
		return;
	}

	/* What remains requires a backward copy from the end of the blocks.  */
	dst += size;
	src += size;
	dofs = dst & (CACHELINE_SIZE - 1);
	sofs = src & (CACHELINE_SIZE - 1);
	dleft = CACHELINE_SIZE - dofs;
	sleft = CACHELINE_SIZE - sofs;
	dst &= -CACHELINE_SIZE;
	src &= -CACHELINE_SIZE;
	if (dofs == 0) {
		dst -= CACHELINE_SIZE;
	}
	if (sofs == 0) {
		src -= CACHELINE_SIZE;
	}

	if (dofs == sofs) {
		/* Since DST and SRC are co-aligned, and we didn't use the memcpy
		 * optimization above, that implies that SIZE > CACHELINE_SIZE.  */
		if (sofs != 0) {
			dpair = W(td, dst);
			sline = R(td, src);
			*dpair.mask |= ((mtm_cacheline_mask)1 << sleft) - 1;
			memcpy (dpair.line, sline, sleft);
			dst -= CACHELINE_SIZE;
			src -= CACHELINE_SIZE;
			size -= sleft;
		}

		while (size >= CACHELINE_SIZE) {
			dpair = W(td, dst);
			sline = R(td, src);
			*dpair.mask = -1;
			mtm_cacheline_copy (dpair.line, sline);
			dst -= CACHELINE_SIZE;
			src -= CACHELINE_SIZE;
			size -= CACHELINE_SIZE;
		}

		if (size != 0) {
			size_t ofs = CACHELINE_SIZE - size;
			dpair = W(td, dst);
			sline = R(td, src);
			*dpair.mask |= (((mtm_cacheline_mask)1 << size) - 1) << ofs;
			memcpy (&dpair.line->b[ofs], &sline->b[ofs], size);
		}
	} else {
		mtm_cacheline c;

		sline = R(td, src);
		if (dofs != 0) {
			size_t min = (size <= dofs ? size : dofs);

			if (min <= sofs) {
				sofs -= min;
				memcpy (&c, &sline->b[sofs], min);
			} else {
				size_t min_ofs = min - sofs;
				memcpy (&c.b[min_ofs], sline, sofs);
				src -= CACHELINE_SIZE;
				sline = R(td, src);
				sofs = CACHELINE_SIZE - min_ofs;
				memcpy (&c, &sline->b[sofs], min_ofs);
			}

			dofs = dleft - min;
			dpair = W(td, dst);
			*dpair.mask |= (((mtm_cacheline_mask)1 << min) - 1) << dofs;
			memcpy (&dpair.line->b[dofs], &c, min);

			sleft = CACHELINE_SIZE - sofs;
			dst -= CACHELINE_SIZE;
			size -= min;
		}

		while (size >= CACHELINE_SIZE) {
			memcpy (&c.b[sleft], sline, sofs);
			src -= CACHELINE_SIZE;
			sline = R(td, src);
			memcpy (&c, &sline->b[sofs], sleft);

			dpair = W(td, dst);
			*dpair.mask = -1;
			mtm_cacheline_copy (dpair.line, &c);

			dst -= CACHELINE_SIZE;
			size -= CACHELINE_SIZE;
		}

		if (size != 0) {
			dofs = CACHELINE_SIZE - size;

			memcpy (&c.b[sleft], sline, sofs);
			if (sleft > dofs) {
				src -= CACHELINE_SIZE;
				sline = R(td, src);
				memcpy (&c, &sline->b[sofs], sleft);
			}

			dpair = W(td, dst);
			*dpair.mask |= (mtm_cacheline_mask)-1 << dofs;
			memcpy (&dpair.line->b[dofs], &c.b[dofs], size);
		}
	}
}


#define MEMCPY_DEFINITION(PREFIX, VARIANT, READ, WRITE)                        \
void _ITM_CALL_CONVENTION mtm_##PREFIX##memcpy##VARIANT(mtm_thread_t *td,  \
                                                   void *dst,                  \
                                                   const void *src,            \
                                                   size_t size)                \
{									                                           \
  do_memcpy (td, (uintptr_t)dst, (uintptr_t)src, size,                         \
             WRITE, READ);                                                     \
}


#define MEMMOVE_DEFINITION(PREFIX, VARIANT, READ, WRITE)                       \
void _ITM_CALL_CONVENTION mtm_##PREFIX##memmove##VARIANT(mtm_thread_t *td, \
                                                    void *dst,                 \
                                                    const void *src,           \
                                                    size_t size)               \
{									                                           \
  do_memmove (td, (uintptr_t)dst, (uintptr_t)src, size,                        \
              WRITE, READ,                                                     \
              PREFIX##WaW,                                                     \
              PREFIX##RfW);                                                    \
}


#define MEMCPY_DECLARATION(PREFIX, VARIANT, READ, WRITE)                       \
void _ITM_CALL_CONVENTION mtm_##PREFIX##memcpy##VARIANT(mtm_thread_t *td,  \
                                                   void *dst,                  \
                                                   const void *src,            \
                                                   size_t size);

#define MEMMOVE_DECLARATION(PREFIX, VARIANT, READ, WRITE)                      \
void _ITM_CALL_CONVENTION mtm_##PREFIX##memmove##VARIANT(mtm_thread_t *td, \
                                                    void *dst,                 \
                                                    const void *src,           \
                                                    size_t size);


#define FORALL_MEMCOPY_VARIANTS(ACTION, prefix)               \
ACTION(prefix, RnWt,     mtm_null_read_lock, prefix##W)                   \
ACTION(prefix, RnWtaR,   mtm_null_read_lock, prefix##WaR)                 \
ACTION(prefix, RnWtaW,   mtm_null_read_lock, prefix##WaW)                 \
ACTION(prefix, RtWn,     prefix##R,              mtm_null_write_lock)     \
ACTION(prefix, RtWt,     prefix##R,              prefix##W)                   \
ACTION(prefix, RtWtaR,   prefix##R,              prefix##WaR)                 \
ACTION(prefix, RtWtaW,   prefix##R,              prefix##WaW)                 \
ACTION(prefix, RtaRWn,   prefix##RaR,            mtm_null_write_lock)     \
ACTION(prefix, RtaRWt,   prefix##RaR,            prefix##W)                   \
ACTION(prefix, RtaRWtaR, prefix##RaR,            prefix##WaR)                 \
ACTION(prefix, RtaRWtaW, prefix##RaR,            prefix##WaW)                 \
ACTION(prefix, RtaWWn,   prefix##RaW,            mtm_null_write_lock)     \
ACTION(prefix, RtaWWt,   prefix##RaW,            prefix##W)                   \
ACTION(prefix, RtaWWtaR, prefix##RaW,            prefix##WaR)                 \
ACTION(prefix, RtaWWtaW, prefix##RaW,            prefix##WaW)


#define FORALL_MEMMOVE_VARIANTS(ACTION, prefix)               \
ACTION(prefix, RnWt,     mtm_null_read_lock, prefix##W)                   \
ACTION(prefix, RnWtaR,   mtm_null_read_lock, prefix##WaR)                 \
ACTION(prefix, RnWtaW,   mtm_null_read_lock, prefix##WaW)                 \
ACTION(prefix, RtWn,     prefix##R,              mtm_null_write_lock)     \
ACTION(prefix, RtWt,     prefix##R,              prefix##W)                   \
ACTION(prefix, RtWtaR,   prefix##R,              prefix##WaR)                 \
ACTION(prefix, RtWtaW,   prefix##R,              prefix##WaW)                 \
ACTION(prefix, RtaRWn,   prefix##RaR,            mtm_null_write_lock)     \
ACTION(prefix, RtaRWt,   prefix##RaR,            prefix##W)                   \
ACTION(prefix, RtaRWtaR, prefix##RaR,            prefix##WaR)                 \
ACTION(prefix, RtaRWtaW, prefix##RaR,            prefix##WaW)                 \
ACTION(prefix, RtaWWn,   prefix##RaW,            mtm_null_write_lock)     \
ACTION(prefix, RtaWWt,   prefix##RaW,            prefix##W)                   \
ACTION(prefix, RtaWWtaR, prefix##RaW,            prefix##WaR)                 \
ACTION(prefix, RtaWWtaW, prefix##RaW,            prefix##WaW)


FORALL_MEMCOPY_VARIANTS(MEMCPY_DECLARATION, wbetl_)
FORALL_MEMMOVE_VARIANTS(MEMMOVE_DECLARATION, wbetl_)
#endif
