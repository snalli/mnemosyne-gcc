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

#ifndef _MEMCPY_H
#define _MEMCPY_H

#define BUFSIZE (sizeof(mtm_word_t)*16)

/* FIXME: The implementations here are not the most efficient possible. */

#define MEMCPY_DEFINITION(PREFIX, VARIANT, READ, WRITE)                        \
void _ITM_CALL_CONVENTION _ITM_memcpy##VARIANT(    void *dst,                  \
                                                   const void *src,            \
                                                   size_t size)                \
{                                                                              \
  mtm_tx_t *tx = mtm_get_tx();						       \
  volatile uint8_t *saddr=((volatile uint8_t *) src);                          \
  volatile uint8_t *daddr=((volatile uint8_t *) dst);                          \
  uint8_t buf[BUFSIZE];                                                        \
                                                                               \
  if (size == 0) {                                                             \
    return;                                                                    \
  }	                                                                       \
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


#define MEMMOVE_DEFINITION(PREFIX, VARIANT, READ, WRITE)                       \
void _ITM_CALL_CONVENTION _ITM_memmove##VARIANT(    void *dst,                 \
                                                    const void *src,           \
                                                    size_t size)               \
{                                                                              \
  mtm_tx_t *tx = mtm_get_tx();						       \
  volatile uint8_t *saddr=((volatile uint8_t *) src);                          \
  volatile uint8_t *daddr=((volatile uint8_t *) dst);                          \
  uint8_t buf[BUFSIZE];                                                        \
                                                                               \
  if (size == 0) {                                                             \
    return;                                                                    \
  }	                                                                       \
  if (saddr < daddr && daddr < saddr + size) {                                 \
    /* Destructive overlap...have to copy backwards */                         \
    saddr=((volatile uint8_t *) src) +size;                                    \
    daddr=((volatile uint8_t *) dst) +size;                                    \
    while (size>BUFSIZE) {                                                     \
      mtm_##PREFIX##_load_bytes(tx, saddr-BUFSIZE, buf, BUFSIZE);              \
      mtm_##PREFIX##_store_bytes(tx, daddr-BUFSIZE, buf, BUFSIZE);             \
      saddr -= BUFSIZE;                                                        \
      daddr -= BUFSIZE;                                                        \
      size -= BUFSIZE;                                                         \
    }	                                                                       \
    if (size > 0) {                                                            \
      mtm_##PREFIX##_load_bytes(tx, saddr-size, buf, size);                    \
      mtm_##PREFIX##_store_bytes(tx, daddr-size, buf, size);                   \
    }                                                                          \
  } else {                                                                     \
    saddr=((volatile uint8_t *) src);                                          \
    daddr=((volatile uint8_t *) dst);                                          \
    while (size>BUFSIZE) {                                                     \
      mtm_##PREFIX##_load_bytes(tx, saddr, buf, BUFSIZE);                      \
      mtm_##PREFIX##_store_bytes(tx, daddr, buf, BUFSIZE);                     \
      saddr += BUFSIZE;                                                        \
      daddr += BUFSIZE;                                                        \
      size -= BUFSIZE;                                                         \
    }	                                                                       \
    if (size > 0) {                                                            \
      mtm_##PREFIX##_load_bytes(tx, saddr, buf, size);                         \
      mtm_##PREFIX##_store_bytes(tx, daddr, buf, size);                        \
    }                                                                          \
  }                                                                            \
}


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
