#ifndef __TM_DEF_H__
#define __TM_DEF_H__

#define TM_SAFE                         __attribute__((transaction_safe))
#define TM_CALL                         __attribute__((transaction_callable))
#define TM_PURE                         __attribute__((transaction_pure))
#define TM_ATTR                         TM_SAFE

TM_PURE extern
void __assert_fail (const char *__assertion, const char *__file,
                    unsigned int __line, const char *__function)
     __THROW __attribute__ ((__noreturn__));

#define TM_ATOMIC       __transaction_atomic
#define TM_RELAXED      __transaction_relaxed
#define PTx	TM_RELAXED

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
