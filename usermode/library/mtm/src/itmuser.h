/**
*** Copyright (C) 2007-2008 Intel Corporation.  All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
***
**/

/*!
 * \file itm_user.h
 * \brief User interface header file for the SSG STM interface.
 *
 * itm_user.h contains the parts of the interface which may be useful
 * to user code, as against the full interface used by the compiler
 * which is defined in itm.h.
 */

#if !defined(_ITMUSER_H)
# define _ITMUSER_H

# ifdef __cplusplus
extern "C"
{
# endif                          /* __cplusplus */

#include <stddef.h>

# if (defined (_WIN32))

typedef unsigned char uint8;
typedef signed   char int8;

typedef unsigned short uint16;
typedef signed   short int16;

typedef unsigned int uint32;
typedef signed   int int32;

typedef unsigned long long uint64;
typedef signed   long long int64;

#  if (!defined (_M_X64))
typedef uint32 uintptr;
typedef int32  intptr;
#  else
typedef uintptr_t uintptr;
typedef intptr_t  intptr;
#  endif

# else /* !(defined (_WIN32)) */

# include <stdint.h>

typedef uint8_t uint8;
typedef int8_t  int8;

typedef uint16_t uint16;
typedef int16_t  int16;

typedef uint32_t uint32;
typedef int32_t  int32;

typedef uint64_t uint64;
typedef int64_t  int64;

typedef uintptr_t uintptr;
typedef intptr_t  intptr;

# endif /* (defined (_WIN32)) */


#ifndef _WINDOWS
  /* supress warnings on declare __attribute__(regparm(2)) function pointers
   */
  #pragma warning( disable : 1287 )
#endif

#if (defined(_TM))
    /* We're being compiled in TM mode, so we want to add the TM Waiver declspec to our
     * declarations here.
     */
#  define TM_PURE __declspec (tm_pure)
#else
#  define TM_PURE 
#endif

/* __fastcall only makes sense on IA32. We use it on all functions for
 * consistency, though it has no effect on functions which take no
 * arguments.
 */
# if (defined (_M_X64))
#  define _ITM_CALL_CONVENTION
# elif (defined (_WIN32))
#  define _ITM_CALL_CONVENTION __fastcall
# else
    /*Assume GCC like behavior on other OSes */
#  if (defined (__x86_64__))
#   define _ITM_CALL_CONVENTION
#  else
#   define _ITM_CALL_CONVENTION __attribute__((regparm(2)))
#  endif

# endif

//struct _ITM_transactionS;
//! Opaque transaction descriptor.
//typedef struct _ITM_transactionS _ITM_transaction;

typedef void (_ITM_CALL_CONVENTION * _ITM_userUndoFunction)(void *);
typedef void (_ITM_CALL_CONVENTION * _ITM_userCommitFunction)(void *);

//! Opaque transaction tag
typedef uint32 _ITM_transactionId;

/** Results from inTransaction */
typedef enum 
{
    outsideTransaction = 0,   /**< So "if (inTransaction(td))" works */
    inRetryableTransaction,
    inIrrevocableTransaction
} _ITM_howExecuting;

/*! \return A pointer to the current transaction descriptor.
 */
TM_PURE
extern _ITM_transaction * _ITM_CALL_CONVENTION _ITM_getTransaction (void);

/*! Is the code executing inside a transaction?
 * \param __td The transaction descriptor.
 * \return 1 if inside a transaction, 0 if outside a transaction.
 */
TM_PURE
extern _ITM_howExecuting _ITM_CALL_CONVENTION _ITM_inTransaction (_ITM_transaction * __td);

/*! Get the thread number which the TM logging and statistics will be using. */
TM_PURE 
extern int _ITM_CALL_CONVENTION _ITM_getThreadnum(void) ;

/*! Add an entry to the user commit action log
 * \param __td The transaction descriptor
 * \param __commit The commit function 
 * \param resumingTransactionId The id of the transaction that must be in the undo path before a commit can proceed.
 * \param __arg The user argument to store in the log entry 
 */
TM_PURE 
extern void _ITM_CALL_CONVENTION _ITM_addUserCommitAction (_ITM_transaction * __td, 
                                                       _ITM_userCommitFunction __commit,
                                                       _ITM_transactionId resumingTransactionId,
                                                       void * __arg);

/*! Add an entry to the user undo action log
 * \param __td The transaction descriptor
 * \param __undo Pointer to the undo function
 * \param __arg The user argument to store in the log entry 
 */
TM_PURE 
extern void _ITM_CALL_CONVENTION _ITM_addUserUndoAction (_ITM_transaction * __td, 
                                                       const _ITM_userUndoFunction __undo, void * __arg);

/** A transaction Id for non-transactional code.  It is guaranteed to be less 
 * than the Id for any transaction 
 */
#define _ITM_noTransactionId (1) // Id for non-transactional code.

/*! A method to retrieve a tag that uniquely identifies a transaction 
    \param __td The transaction descriptor
 */
TM_PURE 
extern _ITM_transactionId _ITM_CALL_CONVENTION _ITM_getTransactionId(_ITM_transaction * __td);

/*! A method to remove any references the library may be storing for a block of memory.
    \param __td The transaction descriptor
    \param __start The start address of the block of memory
    \param __size The size in bytes of the block of memory
 */
TM_PURE
extern void _ITM_CALL_CONVENTION _ITM_dropReferences (_ITM_transaction * __td, const void * __start, size_t __size);

/*! A method to print error from inside the transaction and exit
    \param errString The error description
    \param exitCode  The exit code
 */
TM_PURE
extern void _ITM_CALL_CONVENTION _ITM_userError (const char *errString, int exitCode);

# undef TM_PURE
# ifdef __cplusplus
} /* extern "C" */
# endif                          /* __cplusplus */
#endif /* defined (_ITMUSER_H) */
