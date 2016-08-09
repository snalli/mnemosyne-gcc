#ifndef _HELPER_H_ASA121
#define _HELPER_H_ASA121

__attribute__((tm_callable)) int txc_libc_memcmp (const void * s1, const void * s2, int n);
__attribute__((tm_callable)) char *txc_libc_strcpy (char *dest, const char *src);
__attribute__((tm_callable)) char *txc_libc_memcpy (char *dest, const char *src, const int bytes);

#endif
