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

#define DECLARE_READ_BARRIER(NAME, T, LOCK)                                    \
_ITM_TYPE_##T _ITM_CALL_CONVENTION                                             \
mtm_##NAME##_##LOCK##T(mtm_tx_t *tx,                                           \
                       const _ITM_TYPE_##T *ptr);

#define DECLARE_WRITE_BARRIER(NAME, T, LOCK)                                   \
void _ITM_CALL_CONVENTION mtm_##NAME##_##LOCK##T(mtm_tx_t *tx,                 \
                                                _ITM_TYPE_##T *addr,           \
                                                _ITM_TYPE_##T value);


#define DECLARE_READ_BARRIERS(name, type, encoding)                            \
  DECLARE_READ_BARRIER(name, encoding, R)                                      \
  DECLARE_READ_BARRIER(name, encoding, RaR)                                    \
  DECLARE_READ_BARRIER(name, encoding, RaW)                                    \
  DECLARE_READ_BARRIER(name, encoding, RfW)                                    \


#define DECLARE_WRITE_BARRIERS(name, type, encoding)                           \
  DECLARE_WRITE_BARRIER(name, encoding, W)                                     \
  DECLARE_WRITE_BARRIER(name, encoding, WaR)	                               \
  DECLARE_WRITE_BARRIER(name, encoding, WaW)

typedef union convert_u {
  mtm_word_t w;
  uint8_t b[sizeof(mtm_word_t)];
} convert_t;

#if 0
# define DEFINE_LOAD_BYTES(NAME)                                               \
   DEFINE_IDENTITY_LOAD_BYTES(NAME)

# define DEFINE_STORE_BYTES(NAME)                                              \
   DEFINE_IDENTITY_STORE_BYTES(NAME)
#else
# define DEFINE_LOAD_BYTES(NAME)                                               \
   DEFINE_TM_LOAD_BYTES(NAME)

# define DEFINE_STORE_BYTES(NAME)                                              \
   DEFINE_TM_STORE_BYTES(NAME)

#endif


#define DEFINE_IDENTITY_LOAD_BYTES(NAME)                                       \
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
  }                                                                            \
  for (i=0; i < size; i++) {                                                   \
    buf[i] = addr[i];                                                          \
  }                                                                            \
}

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
  }	                                                                           \
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
  }	                                                                           \
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


#define DEFINE_IDENTITY_STORE_BYTES(NAME)                                      \
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
  for (i=0; i < size; i++) {                                                   \
    addr[i] = buf[i];                                                          \
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
  }	                                                                           \
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
mtm_##NAME##_##LOCK##T(mtm_tx_t *tx,                                           \
                       const _ITM_TYPE_##T *addr)                              \
{                                                                              \
  _ITM_TYPE_##T val;                                                           \
  mtm_##NAME##_load_bytes(tx,                                                  \
                          (volatile uint8_t *)addr,                            \
                          (uint8_t *)&val,                                     \
                          sizeof(_ITM_TYPE_##T));                              \
  return val;                                                                  \
}


#define WRITE_BARRIER(NAME, T, LOCK)                                           \
void _ITM_CALL_CONVENTION mtm_##NAME##_##LOCK##T(mtm_tx_t *tx,                 \
                                                 _ITM_TYPE_##T *addr,          \
                                                 _ITM_TYPE_##T value)          \
{                                                                              \
  mtm_##NAME##_store_bytes(tx,                                                 \
                           (volatile uint8_t *)addr,                           \
                           (uint8_t *)&value,                                  \
                           sizeof(_ITM_TYPE_##T));                             \
}


#define DEFINE_READ_BARRIERS(name, type, encoding)      \
  READ_BARRIER(name, encoding, R)                       \
  READ_BARRIER(name, encoding, RaR)	                    \
  READ_BARRIER(name, encoding, RaW)	                    \
  READ_BARRIER(name, encoding, RfW)		                \


#define DEFINE_WRITE_BARRIERS(name, type, encoding)     \
  WRITE_BARRIER(name, encoding, W)                      \
  WRITE_BARRIER(name, encoding, WaR)                    \
  WRITE_BARRIER(name, encoding, WaW)


/*
 * The following is an effort of implementing special wrappers for the case
 * when the address is word aligned.
 */
#if 0

typedef union convert_U32_u convert_CE_t;
typedef union convert_U16_u convert_E_t;
typedef union convert_U16_u convert_M128_t;
typedef union convert_U16_u convert_CD_t;
typedef union convert_U8_u  convert_U8_t;
typedef union convert_U8_u  convert_D_t;
typedef union convert_U8_u  convert_M64_t;
typedef union convert_U8_u  convert_CF_t;
typedef union convert_U4_u  convert_U4_t;
typedef union convert_U4_u  convert_F_t;
typedef union convert_U2_u  convert_U2_t;
typedef union convert_U1_u  convert_U1_t;

union convert_U32_u {
	_ITM_TYPE_U8   U8[2];
	_ITM_TYPE_U4   U4[4];
	_ITM_TYPE_U2   U2[8];
	_ITM_TYPE_U1   U1[16];
	_ITM_TYPE_CE   CE;
};

union convert_U16_u {
	_ITM_TYPE_U8   U8[2];
	_ITM_TYPE_U4   U4[4];
	_ITM_TYPE_U2   U2[8];
	_ITM_TYPE_U1   U1[16];
	_ITM_TYPE_E    E;
	_ITM_TYPE_M128 M128;
	_ITM_TYPE_CD   CD;
};

union convert_U8_u {
	_ITM_TYPE_U8   U8;
	_ITM_TYPE_U4   U4[2];
	_ITM_TYPE_U2   U2[4];
	_ITM_TYPE_U1   U1[8];
	_ITM_TYPE_D    D;
	_ITM_TYPE_M64  M64;
	_ITM_TYPE_CF   CF;
};

union convert_U4_u {
	_ITM_TYPE_U4   U4;
	_ITM_TYPE_U2   U2[2];
	_ITM_TYPE_U1   U1[4];
	_ITM_TYPE_F    F;
};

union convert_U2_u {
	_ITM_TYPE_U2   U2;
	_ITM_TYPE_U1   U1[2];
};

union convert_U1_u {
	_ITM_TYPE_U1   U1;
};


#define WRITE_BARRIER(NAME, T, LOCK)                                           \
void _ITM_CALL_CONVENTION mtm_##NAME##LOCK##T(mtm_tx_t *tx,                    \
                                              volatile _ITM_TYPE_##T *addr,    \
                                              _ITM_TYPE_##T value)             \
{                                                                              \
  if (((uintptr_t)addr & (sizeof(_ITM_TYPE_##T))-1) != 0) {                    \
    stm_store_bytes(tx, \
                    (volatile uint8_t *)addr, \
                    (uint8_t *)&value, \
                    sizeof(_ITM_TYPE_##T)); \
  } else if (sizeof(mtm_word_t) == 4) { \
    if (sizeof(_ITM_TYPE_##T) >= 4) {    \
	  int i;                                         \
      convert_##T##_t val;                                                \
	  val.#T = value;                                                          \
	  for (i=0; i<sizeof(_ITM_TYPE_##T)/4; i++) {                             \
        stm_store(tx, (volatile mtm_word_t *)addr+i, (mtm_word_t)val.U4[i]);\
	  }                                                                  \
	} else {                                                               \
      convert_U4_t val, mask;                                                      \
      val.#T[((uintptr_t)addr & 0x03) >> (sizeof(_ITM_TYPE_##T)-1)] = value;       \
      mask.U4 = 0;                                                               \
      mask.#T[((uintptr_t)addr & 0x03) >> (sizeof(_ITM_TYPE_##T)-1)] = ~(_ITM_TYPE_##T)0;     \
      stm_store2(tx,                                                           \
                 (volatile mtm_word_t *)((uintptr_t)addr & ~(uintptr_t)0x03),  \
                 (mtm_word_t)val.U4, (mtm_word_t)mask.U4);                     \
	}                                                                          \
  } else {               \
  }                          \
}
#endif



#endif
