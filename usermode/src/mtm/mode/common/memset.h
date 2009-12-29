#ifndef _MEMSET_H
#define _MEMSET_H

#define BUFSIZE (sizeof(mtm_word_t)*16)

/* FIXME: need more efficient implementation */

#define MEMSET_DEFINITION(PREFIX, VARIANT)                                     \
void _ITM_CALL_CONVENTION mtm_##PREFIX##_memset##VARIANT(mtm_tx_t *tx,         \
                                                        void *dst,             \
                                                        int c,                 \
                                                        size_t size)           \
{                                                                              \
  volatile uint8_t *daddr=dst;                                                 \
  uint8_t          buf[BUFSIZE];                                               \
  int              i;                                                          \
                                                                               \
  if (size == 0) {                                                             \
    return;                                                                    \
  }                                                                            \
  for (i=0; i<BUFSIZE; i++) {                                                  \
    buf[i] = c;                                                                \
  }                                                                            \
  while (size>BUFSIZE) {                                                       \
    mtm_##PREFIX##_store_bytes(tx, daddr, buf, BUFSIZE);                       \
    daddr += BUFSIZE;                                                          \
    size -= BUFSIZE;                                                           \
  }	                                                                           \
  if (size > 0) {                                                              \
    mtm_##PREFIX##_store_bytes(tx, daddr, buf, size);                          \
  }                                                                            \
}



#define MEMSET_DECLARATION(PREFIX, VARIANT)                                    \
void _ITM_CALL_CONVENTION mtm_##PREFIX##_memset##VARIANT(mtm_tx_t * tx,        \
                                                        void *dst,             \
                                                        int c, size_t size);

#define FORALL_MEMSET_VARIANTS(ACTION, PREFIX)                                 \
ACTION(PREFIX, W)                                                              \
ACTION(PREFIX, WaR)                                                            \
ACTION(PREFIX, WaW)

FORALL_MEMSET_VARIANTS(MEMSET_DECLARATION, wbetl)

#endif
