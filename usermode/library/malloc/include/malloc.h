#ifndef _MNEMOSYNE_MALLOC_H
#define _MNEMOSYNE_MALLOC_H

#include <stdlib.h>

#if __cplusplus
extern "C" {
#endif

void *pmalloc(size_t sz);
void pfree(void *);
void *prealloc(void *, size_t);

//__attribute__((tm_callable)) void * pmalloc (size_t sz);
__attribute__((tm_wrapping(pmalloc))) void *pmallocTxn(size_t);
__attribute__((tm_wrapping(pfree))) void pfreeTxn(void *);
__attribute__((tm_wrapping(prealloc))) void *preallocTxn(void *, size_t);

#if __cplusplus
}
#endif

#endif
