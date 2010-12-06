#ifndef MNEMOSYNE_H
#define MNEMOSYNE_H

#define MNEMOSYNE_PERSISTENT __attribute__ ((section("PERSISTENT")))

#define TM_CALLABLE __attribute__((tm_callable))

void *pmalloc(size_t sz);
void pfree(void *);
void *prealloc(void *ptr, size_t size);

__attribute__((tm_wrapping(pmalloc))) void *pmallocTxn(size_t);
__attribute__((tm_wrapping(pfree))) void pfreeTxn(void *);
__attribute__((tm_wrapping(prealloc))) void *preallocTxn(size_t);


#endif
