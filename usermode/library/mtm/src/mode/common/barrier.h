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

#ifndef _BARRIER_H
#define _BARRIER_H

typedef union convert_u {
  mtm_word_t w;
  uint8_t b[sizeof(mtm_word_t)];
} convert_t;

# define DEFINE_LOAD_BYTES(NAME)                                               \
   DEFINE_TM_LOAD_BYTES(NAME)

# define DEFINE_STORE_BYTES(NAME)                                              \
   DEFINE_TM_STORE_BYTES(NAME)


#define DEFINE_TM_LOAD_BYTES(NAME)                                             \
void mtm_##NAME##_load_bytes(mtm_tx_t *tx,                                     \
                             volatile uint8_t *addr,                           \
                             uint8_t *buf,                                     \
                             size_t size)                                      \
{                                                                              \
  convert_t    val;                                                            \
  unsigned int i;                                                              \
  mtm_word_t   *a;                                                             \
                                                                               \
  if (size == 0) {                                                             \
    return;                                                                    \
  }	                                                                       \
  i = (uintptr_t)addr & (sizeof(mtm_word_t) - 1);                              \
  if (i != 0) {                                                                \
    /* First bytes */                                                          \
    a = (mtm_word_t *)((uintptr_t)addr & ~(uintptr_t)(sizeof(mtm_word_t) - 1));\
    val.w = mtm_##NAME##_load(tx, a++);                                        \
    for (; i < sizeof(mtm_word_t) && size > 0; i++, size--) {                  \
      *buf++ = val.b[i];                                                       \
    }                                                                          \
  } else {                                                                     \
    a = (mtm_word_t *)addr;                                                    \
  }	                                                                       \
  /* Full words */                                                             \
  while (size >= sizeof(mtm_word_t)) {                                         \
    *((mtm_word_t *)buf) = mtm_##NAME##_load(tx, a++);                         \
    buf += sizeof(mtm_word_t);                                                 \
    size -= sizeof(mtm_word_t);                                                \
  }                                                                            \
  if (size > 0) {                                                              \
    /* Last bytes */                                                           \
    val.w = mtm_##NAME##_load(tx, a);                                          \
    i = 0;                                                                     \
    for (i = 0; size > 0; i++, size--) {                                       \
      *buf++ = val.b[i];                                                       \
    }                                                                          \
  }                                                                            \
}


#define DEFINE_TM_STORE_BYTES(NAME)                                            \
void mtm_##NAME##_store_bytes(mtm_tx_t *tx,                                    \
                              volatile uint8_t *addr,                          \
                              uint8_t *buf,                                    \
                              size_t size)                                     \
{                                                                              \
  convert_t    val;                                                            \
  convert_t    mask;                                                           \
  unsigned int i;                                                              \
  mtm_word_t   *a;                                                             \
                                                                               \
  if (size == 0) {                                                             \
    return;                                                                    \
  }                                                                            \
  i = (uintptr_t)addr & (sizeof(mtm_word_t) - 1);                              \
  if (i != 0) {                                                                \
    /* First bytes */                                                          \
    a = (mtm_word_t *)((uintptr_t)addr & ~(uintptr_t)(sizeof(mtm_word_t) - 1));\
    val.w = mask.w = 0;                                                        \
    for (; i < sizeof(mtm_word_t) && size > 0; i++, size--) {                  \
      mask.b[i] = 0xFF;                                                        \
      val.b[i] = *buf++;                                                       \
    }                                                                          \
    mtm_##NAME##_store2(tx, a++, val.w, mask.w);                               \
  } else {                                                                     \
    a = (mtm_word_t *)addr;                                                    \
  }	                                                                       \
  /* Full words */                                                             \
  while (size >= sizeof(mtm_word_t)) {                                         \
    mtm_##NAME##_store(tx, a++, *((mtm_word_t *)buf));                         \
    buf += sizeof(mtm_word_t);                                                 \
    size -= sizeof(mtm_word_t);                                                \
  }                                                                            \
  if (size > 0) {                                                              \
    /* Last bytes */                                                           \
    val.w = mask.w = 0;                                                        \
    for (i = 0; size > 0; i++, size--) {                                       \
      mask.b[i] = 0xFF;                                                        \
      val.b[i] = *buf++;                                                       \
    }                                                                          \
    mtm_##NAME##_store2(tx, a, val.w, mask.w);                                 \
  }                                                                            \
}

#define READ_BARRIER(NAME, T, LOCK)                                            \
_ITM_TYPE_##T _ITM_CALL_CONVENTION                                             \
_ITM_##LOCK##T(        const _ITM_TYPE_##T *addr)                              \
{                                                                              \
  mtm_tx_t *tx = mtm_get_tx();						       \
  _ITM_TYPE_##T val;                                                           \
  mtm_##NAME##_load_bytes(tx,                                                  \
                          (volatile uint8_t *)addr,                            \
                          (uint8_t *)&val,                                     \
                          sizeof(_ITM_TYPE_##T));                              \
  return val;                                                                  \
}


#define WRITE_BARRIER(NAME, T, LOCK)                                           \
void _ITM_CALL_CONVENTION _ITM_##LOCK##T( const  _ITM_TYPE_##T *addr,          \
                                                 _ITM_TYPE_##T value)          \
{                                                                              \
  mtm_tx_t *tx = mtm_get_tx();						       \
  mtm_##NAME##_store_bytes(tx,                                                 \
                           (volatile uint8_t *)addr,                           \
                           (uint8_t *)&value,                                  \
                           sizeof(_ITM_TYPE_##T));                             \
}


#define DEFINE_READ_BARRIERS(name, type, encoding)      \
  READ_BARRIER(name, encoding, R)                       \
  READ_BARRIER(name, encoding, RaR)	                \
  READ_BARRIER(name, encoding, RaW)	                \
  READ_BARRIER(name, encoding, RfW)		        \


#define DEFINE_WRITE_BARRIERS(name, type, encoding)     \
  WRITE_BARRIER(name, encoding, W)                      \
  WRITE_BARRIER(name, encoding, WaR)                    \
  WRITE_BARRIER(name, encoding, WaW)

#endif
