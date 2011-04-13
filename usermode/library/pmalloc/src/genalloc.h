#ifndef _GENERIC_ALLOC_H
#define _GENERIC_ALLOC_H

#include <stdlib.h>

#define TM_CALLABLE __attribute__((tm_callable))
#define TM_PURE     __attribute__((tm_pure))
#define TM_WAIVER   __tm_waiver

#define GENERIC_PMALLOC(x)          generic_pmalloc(x)
#define GENERIC_PFREE(x)            generic_pfree(x)
#define GENERIC_PREALLOC(x,y)       generic_prealloc(x,y)
#define GENERIC_PCALLOC(x,y)        generic_pcalloc(x,y)
#define GENERIC_PCFREE(x)           generic_pcfree(x)
#define GENERIC_PMEMALIGN(x,y)      generic_pmemalign(x,y)
#define GENERIC_PVALLOC(x)          generic_pvalloc(x)
#define GENERIC_PGET_USABLE_SIZE(x) generic_pmalloc_usable_size(x)
#define GENERIC_PMALLOC_STATS       generic_pmalloc_stats

#ifdef __cplusplus
extern "C" {
#endif

TM_CALLABLE void*  GENERIC_PMALLOC(size_t);
TM_CALLABLE void   GENERIC_PFREE(void* mem);
TM_CALLABLE void*  GENERIC_PREALLOC(void* mem, size_t bytes);
TM_CALLABLE void*  GENERIC_PCALLOC(size_t n, size_t elem_size);
TM_CALLABLE void   GENERIC_PCFREE(void *mem);
TM_CALLABLE void*  GENERIC_PMEMALIGN(size_t alignment, size_t bytes);
TM_CALLABLE void*  GENERIC_PVALLOC(size_t bytes);
TM_CALLABLE void   GENERIC_PMALLOC_STATS();
TM_CALLABLE size_t GENERIC_PGET_USABLE_SIZE(void* mem);
TM_CALLABLE size_t generic_objsize(void *ptr);

#ifdef __cplusplus
} /* end of extern "C" */
#endif 
#endif
