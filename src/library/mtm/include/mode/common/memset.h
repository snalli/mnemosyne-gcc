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

#ifndef _MEMSET_H
#define _MEMSET_H

#define BUFSIZE (sizeof(mtm_word_t)*16)

/* TODO: A more efficient implementation of memset */

#define MEMSET_DEFINITION(PREFIX, VARIANT)                                     \
void _ITM_CALL_CONVENTION _ITM_memset##VARIANT(         void *dst,             \
                                                        int c,                 \
                                                        size_t size)           \
{                                                                              \
  mtm_tx_t *tx = mtm_get_tx();						       \
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
  }	                                                                       \
  if (size > 0) {                                                              \
    mtm_##PREFIX##_store_bytes(tx, daddr, buf, size);                          \
  }                                                                            \
}


#define FORALL_MEMSET_VARIANTS(ACTION, PREFIX)                                 \
ACTION(PREFIX, W)                                                              \
ACTION(PREFIX, WaR)                                                            \
ACTION(PREFIX, WaW)

#endif
