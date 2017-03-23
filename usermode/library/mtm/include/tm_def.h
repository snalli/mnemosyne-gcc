#ifndef __TM_DEF_H__
#define __TM_DEF_H__

#include <features.h>

#define TM_SAFE                         __attribute__((transaction_safe))
#define TM_CALL                         __attribute__((transaction_callable))
#define TM_PURE                         __attribute__((transaction_pure))
#define TM_ATTR                         TM_SAFE
#define TM_ATOMIC       		__transaction_atomic
#define TM_RELAXED      		__transaction_relaxed
#define PTx				TM_RELAXED
#define __persist__			__attribute__ ((section("PERSISTENT")))


/* To prevent GCC from barfing on libc calls */
TM_PURE 
void __assert_fail (const char *__assertion, const char *__file,
                    unsigned int __line, const char *__function)
     __THROW __attribute__ ((__noreturn__));

TM_PURE void *realloc(void *ptr, size_t size);
TM_PURE long int strtol(const char *nptr, char **endptr, int base);
TM_PURE unsigned long long int strtoull(const char *nptr, char **endptr, int base);


#if (__GLIBC__ == 2 && __GLIBC_MINOR__ > 19)
TM_PURE int fprintf (FILE *__restrict __stream, const char *__restrict __fmt, ...);
TM_PURE int sprintf (char *__restrict __s, const char *__restrict __format, ...) __THROWNL;
TM_PURE int snprintf (char *__restrict __s, size_t __maxlen, const char *__restrict __format, ...)
	__THROWNL __attribute__ ((__format__ (__printf__, 3, 4)));
TM_PURE int vsprintf (char *__restrict __s, const char *__restrict __fmt, _G_va_list __ap);
TM_PURE int vsnprintf (char *__restrict __s, size_t __n, const char *__restrict __fmt, _G_va_list __ap);
#endif

TM_PURE size_t __builtin_object_size (void * ptr, int type);
TM_PURE int __fprintf_chk (FILE *__restrict __stream, int __flag,
                          const char *__restrict __format, ...);
TM_PURE int __builtin___sprintf_chk (char *s, int flag, size_t os, const char *fmt, ...);
TM_PURE int __builtin___snprintf_chk (char *s, size_t maxlen, int flag, size_t os, const char *fmt, ...);
TM_PURE int __builtin___vsprintf_chk (char *s, int flag, size_t os, const char *fmt, va_list ap);
TM_PURE int __builtin___vsnprintf_chk (char *s, size_t maxlen, int flag, size_t os, const char *fmt, va_list ap);




/*
 * The choice of relaxed over atomic transactions
 * is to enable the programmer to call un-safe 
 * routines like system calls, print routines etc.
 * that can potentially transaction state to the outside
 * world. This can be useful while debugging to inspect
 * transaction state. 
 * The choice of TM_SAFE over TM_CALL attribute for routines
 * is purely due to GCC semantic. For some reason, I cannot 
 * figure out as to why a routine marked with TM_CALL does
 * not get instrumented.
 *
 */
#endif
