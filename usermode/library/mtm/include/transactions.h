/*!
 * \file
 * Defines a collection of helpers for guiding a transactional compiler according with the Intel STM ABI.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef TRANSACTIONS_H_MED8GE8A
#define TRANSACTIONS_H_MED8GE8A

#ifndef TM_CALLABLE
#define TM_CALLABLE __attribute__((tm_callable))
#endif

#ifndef TM_WAIVER
#define TM_WAIVER __attribute__((tm_pure))
#endif

#ifndef TM_WRAPPING
#define TM_WRAPPING(function) __attribute__((tm_wrapping(function)))
#endif

#endif /* end of include guard: TRANSACTIONS_H_MED8GE8A */
