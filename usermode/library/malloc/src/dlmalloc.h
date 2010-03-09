#ifndef _DLMALLOC_H
#define _DLMALLOC_H

#include <stdlib.h>

#define TM_CALLABLE __attribute__((tm_callable))
#define TM_PURE     __attribute__((tm_pure))
#define TM_WAIVER   __tm_waiver

#define PDL_MALLOC(x)          pdl_malloc(x)
#define PDL_FREE(x)            pdl_free(x)
#define PDL_REALLOC(x,y)       pdl_realloc(x,y)
#define PDL_CALLOC(x,y)        pdl_calloc(x,y)
#define PDL_CFREE(x)           pdl_cfree(x)
#define PDL_MEMALIGN(x,y)      pdl_memalign(x,y)
#define PDL_VALLOC(x)          pdl_valloc(x)
#define PDL_GET_USABLE_SIZE(x) pdl_malloc_usable_size(x)
#define PDL_MALLOC_STATS       pdl_malloc_stats

#if __cplusplus
extern "C" {
#endif

TM_CALLABLE void*  PDL_MALLOC(size_t);
TM_CALLABLE void   PDL_FREE(void* mem);
TM_CALLABLE void*  PDL_REALLOC(void* mem, size_t bytes);
TM_CALLABLE void*  PDL_CALLOC(size_t n, size_t elem_size);
TM_CALLABLE void   PDL_CFREE(void *mem);
TM_CALLABLE void*  PDL_MEMALIGN(size_t alignment, size_t bytes);
TM_CALLABLE void*  PDL_VALLOC(size_t bytes);
TM_CALLABLE void   PDL_MALLOC_STATS();
TM_CALLABLE size_t PDL_GET_USABLE_SIZE(void* mem);

#if __cplusplus
} /* end of extern "C" */
#endif 
#endif
