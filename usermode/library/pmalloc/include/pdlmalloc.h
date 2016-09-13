/**
 * \file pdlmalloc.h
 * 
 * \brief A simple persistent memory allocator based on Doug Lea's malloc
 *
 */

#ifndef _PDLMALLOC_H_JUI112
#define _PDLMALLOC_H_JUI112

#include <stdlib.h>

#define PDL_MALLOC(x)          pdl_malloc(x)
#define PDL_FREE(x)            pdl_free(x)
#define PDL_REALLOC(x,y)       pdl_realloc(x,y)
#define PDL_CALLOC(x,y)        pdl_calloc(x,y)
#define PDL_CFREE(x)           pdl_cfree(x)
#define PDL_MEMALIGN(x,y)      pdl_memalign(x,y)
#define PDL_VALLOC(x)          pdl_valloc(x)
#define PDL_GET_USABLE_SIZE(x) pdl_malloc_usable_size(x)
#define PDL_MALLOC_STATS       pdl_malloc_stats

#define PDL_OBJSIZE(x)         pdl_objsize(x)

#ifdef __cplusplus
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
size_t PDL_OBJSIZE(void *ptr);


#ifdef __cplusplus
} /* end of extern "C" */
#endif 

#endif  /* _PDLMALLOC_H_JUI112 */
