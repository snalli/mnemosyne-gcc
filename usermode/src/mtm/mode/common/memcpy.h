#ifndef _MEMCPY_H
#define _MEMCPY_H

#define BUFSIZE (sizeof(mtm_word_t)*16)

/* FIXME: The implementations here are not the most efficient possible. */

#define MEMCPY_DEFINITION(PREFIX, VARIANT, READ, WRITE)                        \
void _ITM_CALL_CONVENTION mtm_##PREFIX##_memcpy##VARIANT(mtm_tx_t *tx,         \
                                                   void *dst,                  \
                                                   const void *src,            \
                                                   size_t size)                \
{                                                                              \
  volatile uint8_t *saddr=((volatile uint8_t *) src);                          \
  volatile uint8_t *daddr=dst;                                                 \
  uint8_t buf[BUFSIZE];                                                        \
  if (size == 0) {                                                             \
    return;                                                                    \
  }	                                                                           \
  while (size>BUFSIZE) {                                                       \
    mtm_##PREFIX##_load_bytes(tx, saddr, buf, BUFSIZE);                        \
    mtm_##PREFIX##_store_bytes(tx, daddr, buf, BUFSIZE);                       \
    saddr += BUFSIZE;                                                          \
    daddr += BUFSIZE;                                                          \
    size -= BUFSIZE;                                                           \
  }	                                                                           \
  if (size > 0) {                                                              \
    mtm_##PREFIX##_load_bytes(tx, saddr, buf, size);                           \
    mtm_##PREFIX##_store_bytes(tx, daddr, buf, size);                          \
  }                                                                            \
}

/* FIXME: a more efficient memmove should check for overlapping and avoid 
 * copying the overlapped region 
 */

#define MEMMOVE_DEFINITION(PREFIX, VARIANT, READ, WRITE)                       \
void _ITM_CALL_CONVENTION mtm_##PREFIX##_memmove##VARIANT(mtm_tx_t *tx,        \
                                                    void *dst,                 \
                                                    const void *src,           \
                                                    size_t size)               \
{                                                                              \
  volatile uint8_t *saddr=((volatile uint8_t *) src) +size;                    \
  volatile uint8_t *daddr=dst+size;                                            \
  uint8_t buf[BUFSIZE];                                                        \
  if (size == 0) {                                                             \
    return;                                                                    \
  }	                                                                           \
  /* Backward non-destructive copying */                                       \
  while (size>BUFSIZE) {                                                       \
    mtm_##PREFIX##_load_bytes(tx, saddr-BUFSIZE, buf, BUFSIZE);                \
    mtm_##PREFIX##_store_bytes(tx, daddr-BUFSIZE, buf, BUFSIZE);               \
    saddr -= BUFSIZE;                                                          \
    daddr -= BUFSIZE;                                                          \
    size -= BUFSIZE;                                                           \
  }	                                                                           \
  if (size > 0) {                                                              \
    mtm_##PREFIX##_load_bytes(tx, saddr-size, buf, size);                      \
    mtm_##PREFIX##_store_bytes(tx, daddr-size, buf, size);                     \
  }                                                                            \
}


#define MEMCPY_DECLARATION(PREFIX, VARIANT, READ, WRITE)                       \
void _ITM_CALL_CONVENTION mtm_##PREFIX##_memcpy##VARIANT(mtm_tx_t *tx,         \
                                                   void *dst,                  \
                                                   const void *src,            \
                                                   size_t size);

#define MEMMOVE_DECLARATION(PREFIX, VARIANT, READ, WRITE)                      \
void _ITM_CALL_CONVENTION mtm_##PREFIX##_memmove##VARIANT(mtm_tx_t *tx,        \
                                                    void *dst,                 \
                                                    const void *src,           \
                                                    size_t size);


#define FORALL_MEMCOPY_VARIANTS(ACTION, prefix)   \
ACTION(prefix, RnWt,     false, true)             \
ACTION(prefix, RnWtaR,   false, true)             \
ACTION(prefix, RnWtaW,   false, true)             \
ACTION(prefix, RtWn,     true,  false)            \
ACTION(prefix, RtWt,     true,  true)             \
ACTION(prefix, RtWtaR,   true,  true)             \
ACTION(prefix, RtWtaW,   true,  true)             \
ACTION(prefix, RtaRWn,   true,  false)            \
ACTION(prefix, RtaRWt,   true,  true)             \
ACTION(prefix, RtaRWtaR, true,  true)             \
ACTION(prefix, RtaRWtaW, true,  true)             \
ACTION(prefix, RtaWWn,   true,  false)            \
ACTION(prefix, RtaWWt,   true,  true)             \
ACTION(prefix, RtaWWtaR, true,  true)             \
ACTION(prefix, RtaWWtaW, true,  true)


#define FORALL_MEMMOVE_VARIANTS(ACTION, prefix)   \
ACTION(prefix, RnWt,     false, true)             \
ACTION(prefix, RnWtaR,   false, true)             \
ACTION(prefix, RnWtaW,   false, true)             \
ACTION(prefix, RtWn,     true,  false)            \
ACTION(prefix, RtWt,     true,  true)             \
ACTION(prefix, RtWtaR,   true,  true)             \
ACTION(prefix, RtWtaW,   true,  true)             \
ACTION(prefix, RtaRWn,   true,  false)            \
ACTION(prefix, RtaRWt,   true,  true)             \
ACTION(prefix, RtaRWtaR, true,  true)             \
ACTION(prefix, RtaRWtaW, true,  true)             \
ACTION(prefix, RtaWWn,   true,  false)            \
ACTION(prefix, RtaWWt,   true,  true)             \
ACTION(prefix, RtaWWtaR, true,  true)             \
ACTION(prefix, RtaWWtaW, true,  true)


#endif
