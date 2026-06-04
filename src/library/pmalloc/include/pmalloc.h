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

#ifndef _MNEMOSYNE_PMALLOC_H
#define _MNEMOSYNE_PMALLOC_H

#include <stdlib.h>

#if __cplusplus
extern "C" {
#endif

__attribute__((transaction_pure)) void *_ITM_pmalloc(size_t);
#define pmalloc _ITM_pmalloc
__attribute__((transaction_pure)) void _ITM_pfree(void *);
#define pfree _ITM_pfree
__attribute__((transaction_pure)) void *_ITM_pcalloc(size_t, size_t);
#define pcalloc _ITM_pcalloc
__attribute__((transaction_pure)) void *_ITM_prealloc(void *, size_t);
#define prealloc _ITM_prealloc

#if __cplusplus
}
#endif

#endif
