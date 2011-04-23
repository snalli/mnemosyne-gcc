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

/**
 * \file genalloc.h
 * 
 * \brief Choose the generic persistent memory allocator based on configuration 
 *
 */

#ifndef _GENALLOC_H_AGH112
#define _GENALLOC_H_AGH112

#include <stdlib.h>

#define TM_CALLABLE __attribute__((tm_callable))
#define TM_PURE     __attribute__((tm_pure))
#define TM_WAIVER   __tm_waiver

#define GENALLOC_DOUGLEA    0x1
#define GENALLOC_VISTAHEAP  0x2

#if GENALLOC == GENALLOC_DOUGLEA

#define GENERIC_PMALLOC(x)          pdl_malloc(x)
#define GENERIC_PFREE(x)            pdl_free(x)
#define GENERIC_PREALLOC(x,y)       pdl_realloc(x,y)
#define GENERIC_PCALLOC(x,y)        pdl_ccalloc(x,y)
#define GENERIC_PCFREE(x)           pdl_cfree(x)
#define GENERIC_PMEMALIGN(x,y)      pdl_memalign(x,y)
#define GENERIC_PVALLOC(x)          pdl_valloc(x)
#define GENERIC_PGET_USABLE_SIZE(x) pdl_malloc_usable_size(x)
#define GENERIC_PMALLOC_STATS       pdl_malloc_stats

#define GENERIC_OBJSIZE(x)          pdl_objsize(x)

#include "pdlmalloc.h"

#endif

#if GENALLOC == GENALLOC_VISTAHEAP

#define GENERIC_PMALLOC(x)          vhalloc_pmalloc(x)
#define GENERIC_PFREE(x)            vhalloc_pfree(x)
#define GENERIC_PREALLOC(x,y)       vhalloc_prealloc(x,y)
#define GENERIC_PCALLOC(x,y)        vhalloc_pcalloc(x,y)
#define GENERIC_PCFREE(x)           vhalloc_pcfree(x)
#define GENERIC_PMEMALIGN(x,y)      vhalloc_pmemalign(x,y)
#define GENERIC_PVALLOC(x)          vhalloc_pvalloc(x)
#define GENERIC_PGET_USABLE_SIZE(x) vhalloc_pmalloc_usable_size(x)
#define GENERIC_PMALLOC_STATS       vhalloc_pmalloc_stats

#define GENERIC_OBJSIZE(x)          vhalloc_objsize(x)

#include "vhalloc.h"

#endif



#endif /* _GENALLOC_H_AGH112 */
