/**
 * \file vhalloc.h
 * 
 * \brief A simple persistent memory allocator based on Vistaheap
 *
 */

#ifndef _VHALLOC_H_KIA911
#define _VHALLOC_H_KIA911

#include <stdlib.h>

#define VHALLOC_PMALLOC(x)          vhalloc_pmalloc(x)
#define VHALLOC_PFREE(x)            vhalloc_pfree(x)
#define VHALLOC_PREALLOC(x,y)       vhalloc_prealloc(x,y)
#define VHALLOC_PCALLOC(x,y)        vhalloc_pcalloc(x,y)
#define VHALLOC_PCFREE(x)           vhalloc_pcfree(x)
#define VHALLOC_PMEMALIGN(x,y)      vhalloc_pmemalign(x,y)
#define VHALLOC_PVALLOC(x)          vhalloc_pvalloc(x)
#define VHALLOC_PGET_USABLE_SIZE(x) vhalloc_pmalloc_usable_size(x)
#define VHALLOC_PMALLOC_STATS       vhalloc_pmalloc_stats

#define VHALLOC_OBJSIZE(x)          vhalloc_objsize(x)

#ifdef __cplusplus
extern "C" {
#endif

TM_CALLABLE void*  VHALLOC_PMALLOC(size_t);
TM_CALLABLE void   VHALLOC_PFREE(void* mem);
TM_CALLABLE void*  VHALLOC_PREALLOC(void* mem, size_t bytes);
TM_CALLABLE void*  VHALLOC_PCALLOC(size_t n, size_t elem_size);
TM_CALLABLE void   VHALLOC_PCFREE(void *mem);
TM_CALLABLE void*  VHALLOC_PMEMALIGN(size_t alignment, size_t bytes);
TM_CALLABLE void*  VHALLOC_PVALLOC(size_t bytes);
TM_CALLABLE void   VHALLOC_PMALLOC_STATS();
TM_CALLABLE size_t VHALLOC_PGET_USABLE_SIZE(void* mem);
size_t VHALLOC_OBJSIZE(void *ptr);

#ifdef __cplusplus
} /* end of extern "C" */
#endif

#endif /* _VHALLOC_H_KIA911 */
