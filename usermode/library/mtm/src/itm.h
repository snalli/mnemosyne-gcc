/**
*** Copyright (C) 2007-2008 Intel Corporation.  All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
***
**/

/*
 * This file can be processed with doxygen to generate documentation.
 * If you do that you will get better results if you use this set of parameters
 * in your Doxyfile.
 *
 *  ENABLE_PREPROCESSING   = YES
 *  MACRO_EXPANSION        = YES
 *  EXPAND_ONLY_PREDEF     = YES
 *  PREDEFINED             = "_ITM_NORETURN(arg) = arg "\
 *                           "_ITM_CALL_CONVENTION= "   \
 *                           "_ITM_PRIVATE= "           \
 *                           "_ITM_EXPORT= "            \
 *  EXPAND_AS_DEFINED      = _ITM_FOREACH_TRANSFER
 */

/*!
 * \file itm09.h
 * \brief Header file for the Intel C/C++ STM Compiler ABI v0.9.
 *
 * itm09.h contains the full 0.9 ABI interface between the compiler and the TM runtime.
 * Functions which can be called by user code are in itmuser.h.
 */

//! Version number (the version times 100)
# ifndef _ITM_VERSION
# define _ITM_VERSION "1.0.4"
#endif
# ifndef _ITM_VERSION_NO
# define _ITM_VERSION_NO 104
#endif

# ifdef __cplusplus
extern "C"
{
# endif                          /* __cplusplus */

# include <xmmintrin.h>
# include <stddef.h>

    /* defined(_WIN32) should be sufficient, but it looks so much like a latent bug
     * that I also test _WIN64
     */
# if (defined (_WIN32) || defined (_WIN64))
#  define  _ITM_NORETURN(funcspec) __declspec(noreturn) funcspec
/* May need something here for Windows, but I'm not fixing that just now... */
#  define  _ITM_EXPORT
#  define  _ITM_PRIVATE
# else
#  define _ITM_NORETURN(funcspec) funcspec __attribute__((noreturn))
#  define _ITM_EXPORT  __attribute__((visibility("default")))
#  define _ITM_PRIVATE __attribute__((visibility("hidden" )))
# endif

# include "itmuser.h"

/* Opaque types passed over the interface.
 * In most cases these allow us to use pointers to specific opaque
 * types, rather than just "void *" and thus get some type checking
 * without having to make the content of the type visible.  This also
 * allows the data-flow from the result of one function to an argument
 * to another to be clearer.
 */
//! Opaque memento descriptor
struct _ITM_mementoS;
typedef struct _ITM_mementoS _ITM_memento;

/*! This is the same source location structure as is used for OpenMP. */
struct _ITM_srcLocationS
{
    int32 reserved_1;
    int32 flags;
    int32 reserved_2;
    int32 reserved_3;
    const char *psource; /**< psource contains a string describing the
     * source location generated like this
     * \code
     * sprintf(loc_name,";%s;%s;%d;%d;;", path_file_name, routine_name, sline, eline); 
     * \endcode 
     * where sline is the starting line and eline the ending line.
     */
};

typedef struct _ITM_srcLocationS _ITM_srcLocation;

/*! Values to describe properties of the code generated for a
 * transaction, passed in to startTransaction.
 */ 
typedef enum 
{
   pr_instrumentedCode   = 0x0001, /**< Instrumented code is available.  */
   pr_uninstrumentedCode = 0x0002, /**< Uninstrumented code is available.  */
   pr_multiwayCode = pr_instrumentedCode | pr_uninstrumentedCode,
   pr_hasNoXMMUpdate     = 0x0004, /**< Compiler has proved that no XMM registers are updated.  */
   pr_hasNoAbort         = 0x0008, /**< Compiler has proved that no user abort can occur. */
   pr_hasNoRetry         = 0x0010, /**< Compiler has proved that no user retry can occur. */
   pr_hasNoIrrevocable   = 0x0020, /**< Compiler has proved that the transaction does not become irrevocable. */
   pr_doesGoIrrevocable  = 0x0040, /**< Compiler has proved that the transaction always becomes irrevocable. */
   pr_hasNoSimpleReads   = 0x0080, /**< Compiler has proved that the transaction has no R<type> calls. */

   /* More detailed information about properties of instrumented code generation. 
    * These don't tell us that the transaction had no need for these barriers (like hasNoSimpleReads),
    * rather that the compiler simply generated native ld/st for these operations and omitted the
    * barriers. (Useful if the library implementation didn't need them, for instance an in-place
    * update STM never needs to know about "after Write" operations).
    */
   pr_aWBarriersOmitted  = 0x0100, /**< Compiler omitted "after write" barriers  */
   pr_RaRBarriersOmitted = 0x0200, /**< Compiler omitted "read after read" barriers  */
   pr_undoLogCode        = 0x0400, /**< Compiler generated only code for undo-logging, no other barriers. */

   /* Another hint from the compiler. */
   pr_preferUninstrumented
                         = 0x0800, /**< Compiler thinks this is a transaction best run uninstrumented. */
   pr_exceptionBlock     = 0x1000,
   pr_hasElse            = 0x2000
} _ITM_codeProperties;

/*! Result from startTransaction which describes what actions to take.  */
typedef enum
{
    a_runInstrumentedCode   = 0x01, /**< Run instrumented code.  */
    a_runUninstrumentedCode = 0x02, /**< Run uninstrumented code.  */
    a_saveLiveVariables     = 0x04, /**< Save live in variables (only legal with run instrumented code).  */
    a_restoreLiveVariables  = 0x08, /**< Restore live in variables (only legal with abort or retry).  */
    a_abortTransaction      = 0x10, /**< Branch to the abort label.  */
} _ITM_actions;

/*! Values to be passed to changeTransactionMode. */
typedef enum 
{
    modeSerialIrrevocable, /**< Serial irrevocable mode. No abort, no retry. Good for legacy functions. */
    modeObstinate,         /**< Obstinate mode. Abort or retry are possible, but this transaction wins all contention fights. */
    modeOptimistic,        /**< Optimistic mode. Force transaction to run in optimistic mode. */
    modePessimistic,       /**< Pessimistic mode. Force transaction to run in pessimistic mode. */
} _ITM_transactionState;

/*! Values to be passed to _ITM_abort*Transaction */
typedef enum {
    unknown = 0,
    userAbort = 1,     /**< __tm_abort was invoked. */
    userRetry = 2,     /**< __tm_retry was invoked. */              
    TMConflict= 4,     /**< The runtime detected a memory access conflict. */
    exceptionBlockAbort = 8   /**< Propagate abort from a systhetic transaction for catch block or destructor. */
} _ITM_abortReason;


/*! Memory management functions to be used directly from user code */
_ITM_EXPORT extern void * _ITM_CALL_CONVENTION _ITM_malloc (size_t size);

_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_free (void * ptr);

/*! Version checking */
_ITM_EXPORT extern const char * _ITM_CALL_CONVENTION _ITM_libraryVersion (void);

/*! Call with the _ITM_VERSION_NO macro defined above, so that the
 * library can handle compatibility issues.
 */
_ITM_EXPORT extern int _ITM_CALL_CONVENTION _ITM_versionCompatible (int); /* Zero if not compatible */

/*! Initialization, finalization
 * Result of initialization function is zero for success, non-zero for failure, so you want
 * if (!_ITM_initializeProcess())
 * {
 *     ... error exit ...
 * }
 */
_ITM_EXPORT extern int  _ITM_CALL_CONVENTION _ITM_initializeProcess(void);      /* Idempotent */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_finalizeProcess(void);
_ITM_EXPORT extern int  _ITM_CALL_CONVENTION _ITM_initializeThread(void);       /* Idempotent */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_finalizeThread(void);

/*! Error reporting */
_ITM_NORETURN (_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_error (const _ITM_srcLocation *, int errorCode));

/* _ITM_getTransaction(void), _ITM_inTransaction(_ITM_transaction*), and
   _ITM_getTransactionId(_ITM_transaction*) are in itmuser.h */

/*! Begin a transaction.
 * This function can return more than once (cf setjump). The result always tells the compiled
 * code what needs to be executed next.
 * \param __properties A bit mask composed of values from _ITM_codeProperties or-ed together.
 * \param __src The source location.
 * \return A bit mask composed of values from _ITM_actions or-ed together.
 */
_ITM_EXPORT extern uint32 _ITM_CALL_CONVENTION _ITM_beginTransaction  (_ITM_transaction *td,
                                                                         uint32 __properties,
                                                                         const _ITM_srcLocation *__src);

/*! Commit the innermost transaction. If the innermost transaction is also the outermost,
 * validate and release all reader/writer locks.
 * If this fuction returns the transaction is committed. On a commit
 * failure the commit will not return, but will instead longjump and return from the associated
 * beginTransaction call.
 * \param __src The source location of the transaction.
 * 
 */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_commitTransaction     (_ITM_transaction *td,
                                                                         const _ITM_srcLocation *__src);

/*! Try to commit the innermost transaction. If the innermost transaction is also the outermost,
 * validate. If validation fails, return 0. Otherwise,  release all reader/writer locks and return 1.
 * \param __src The source location of the transaction.
 * 
 */
_ITM_EXPORT extern uint32 _ITM_CALL_CONVENTION _ITM_tryCommitTransaction  (_ITM_transaction *td,
                                                                             const _ITM_srcLocation *__src);

/*! Commit the to the nested transaction specifed by the first arugment.
 * \param tid The transaction id for the nested transaction that we are commit to.
 * \param __src The source location of the catch block.
 * 
 */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_commitTransactionToId (_ITM_transaction *td,
                                                                         const _ITM_transactionId tid,
                                                                         const _ITM_srcLocation *__src);

/*! Abort a transaction.
 * This function will never return, instead it will longjump and return from the associated
 * beginTransaction call (or from the beginTransaction call assocatiated with a catch block or destructor
 * entered on an exception).
 * \param __td The transaction pointer.
 * \param __reason The reason for the abort.
 * \param __src The source location of the __tm_abort (if abort is called by the compiler).
 */
_ITM_NORETURN (_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_abortTransaction(_ITM_transaction *td,
                                                                                  _ITM_abortReason __reason, 
                                                                                  const _ITM_srcLocation *__src));

/*! Rollback a transaction to the innermost nesting level.
 * This function is similar to abortTransaction, but returns.
 * It does not longjump and return from a beginTransaction.
 * \param __td The transaction pointer.
 * \param __src The source location of the __tm_abort (if abort is called by the compiler).
 */
/*! Abort a transaction which may or may not be nested. Arguments and semantics as for abortOuterTransaction. */
/* Will return if called with uplevelAbort, otherwise it longjumps */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_rollbackTransaction(_ITM_transaction *td,
                                                                      const _ITM_srcLocation *__src);

/*! Register the thrown object to avoid undoing it, in case the transaction aborts and throws an exception
 * from inside a catch handler.
 * \param __td The transaction pointer.
 * \param __obj The base address of the thrown object.
 * \param __size The size of the object in bytes.
 */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_registerThrownObject(_ITM_transaction *td, const void *__obj, size_t __size);

/*! Enter an irrevocable mode.
 * If this function returns then execution is now in the new mode, however it's possible that
 * to enter the new mode the transaction must be aborted and retried, in which case this function
 * will not return.
 *
 * \param __td The transaction descriptor.
 * \param __mode The new execution mode.
 * \param __src The source location.
 */
_ITM_EXPORT extern void _ITM_CALL_CONVENTION _ITM_changeTransactionMode (_ITM_transaction *td,
                                                                         _ITM_transactionState __mode,
                                                                         const _ITM_srcLocation * __loc);

/* Support macros to generate the multiple data transfer functions we need. 
 * We leave them defined because they are useful elsewhere, for instance when building
 * the vtables we use for mode changes.
 */

# ifndef __cplusplus
/*! Invoke a macro on each of the transfer functions, passing it the
 * result type, function name (without the _ITM_ prefix) and arguments (including parentheses).
 * This can be used to generate all of the function prototypes, part of the vtable,
 * statistics enumerations and so on.
 *
 * Anywhere where the names of all the transfer functions are needed is a good candidate
 * to use this macro.
 */

#  define _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(GENERATE, ACTION, ...)  \
GENERATE (ACTION, uint8,U1, __VA_ARGS__)                              \
GENERATE (ACTION, uint16,U2, __VA_ARGS__)                             \
GENERATE (ACTION, uint32,U4, __VA_ARGS__)                             \
GENERATE (ACTION, uint64,U8, __VA_ARGS__)                             \
GENERATE (ACTION, float,F, __VA_ARGS__)                                 \
GENERATE (ACTION, double,D, __VA_ARGS__)                                \
GENERATE (ACTION, long double,E, __VA_ARGS__)                           \
GENERATE (ACTION, __m64,M64, __VA_ARGS__)                               \
GENERATE (ACTION, __m128,M128, __VA_ARGS__)                             \
GENERATE (ACTION, float _Complex, CF, __VA_ARGS__)                      \
GENERATE (ACTION, double _Complex, CD, __VA_ARGS__)                     \
GENERATE (ACTION, long double _Complex, CE, __VA_ARGS__) 
# else
#  define _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(GENERATE, ACTION, ...)  \
GENERATE (ACTION, uint8,U1, __VA_ARGS__)                              \
GENERATE (ACTION, uint16,U2, __VA_ARGS__)                             \
GENERATE (ACTION, uint32,U4, __VA_ARGS__)                             \
GENERATE (ACTION, uint64,U8, __VA_ARGS__)                             \
GENERATE (ACTION, float,F, __VA_ARGS__)                                 \
GENERATE (ACTION, double,D, __VA_ARGS__)                                \
GENERATE (ACTION, long double,E, __VA_ARGS__)                           \
GENERATE (ACTION, __m64,M64, __VA_ARGS__)                               \
GENERATE (ACTION, __m128,M128, __VA_ARGS__)
# endif

# define _ITM_FOREACH_MEMCPY0(ACTION, NAME, ...)                                                            \
ACTION (void, NAME##RnWt, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)           \
ACTION (void, NAME##RnWtaR, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, NAME##RnWtaW, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, NAME##RtWn,   (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, NAME##RtWt,   (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, NAME##RtWtaR, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, NAME##RtWtaW, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, NAME##RtaRWn, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, NAME##RtaRWt, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, NAME##RtaRWtaR, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)       \
ACTION (void, NAME##RtaRWtaW, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)       \
ACTION (void, NAME##RtaWWn, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, NAME##RtaWWt, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)         \
ACTION (void, NAME##RtaWWtaR, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)       \
ACTION (void, NAME##RtaWWtaW, (_ITM_transaction *, void *, const void *, size_t), __VA_ARGS__)

#define _ITM_FOREACH_MEMCPY(ACTION,...) _ITM_FOREACH_MEMCPY0(ACTION,memcpy,__VA_ARGS__)

#define _ITM_FOREACH_MEMMOVE(ACTION,...) _ITM_FOREACH_MEMCPY0(ACTION,memmove,__VA_ARGS__)

# define _ITM_FOREACH_MEMSET(ACTION, ...)                                               \
ACTION (void, memsetW, (_ITM_transaction *, void *, int, size_t), __VA_ARGS__)          \
ACTION (void, memsetWaR, (_ITM_transaction *, void *, int, size_t), __VA_ARGS__)        \
ACTION (void, memsetWaW, (_ITM_transaction *, void *, int, size_t), __VA_ARGS__)

#define _ITM_FOREACH_SIMPLE_TRANSFER(ACTION,ARG)           \
    _ITM_FOREACH_SIMPLE_READ_TRANSFER(ACTION,ARG)          \
    _ITM_FOREACH_SIMPLE_WRITE_TRANSFER(ACTION,ARG)

# define _ITM_FOREACH_TRANSFER(ACTION, ARG)     \
 _ITM_FOREACH_SIMPLE_TRANSFER(ACTION, ARG)      \
 _ITM_FOREACH_MEMCPY (ACTION, ARG)              \
 _ITM_FOREACH_LOG_TRANSFER(ACTION,ARG)          \
 _ITM_FOREACH_MEMSET (ACTION,ARG)               \
 _ITM_FOREACH_MEMMOVE (ACTION,ARG)

#  define _ITM_FOREACH_SIMPLE_READ_TRANSFER(ACTION,ARG)                           \
    _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(_ITM_GENERATE_READ_FUNCTIONS, ACTION,ARG) 

#  define _ITM_FOREACH_SIMPLE_WRITE_TRANSFER(ACTION,ARG)                           \
    _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(_ITM_GENERATE_WRITE_FUNCTIONS, ACTION,ARG) 

#  define _ITM_FOREACH_SIMPLE_LOG_TRANSFER(ACTION,ARG)                           \
    _ITM_GENERATE_FOREACH_SIMPLE_TRANSFER(_ITM_GENERATE_LOG_FUNCTIONS, ACTION,ARG) 

#  define _ITM_FOREACH_LOG_TRANSFER(ACTION,ARG)                         \
    _ITM_FOREACH_SIMPLE_LOG_TRANSFER(ACTION,ARG)                        \
    ACTION(void, LB, (_ITM_transaction *, const void*, size_t), ARG)

#  define _ITM_GENERATE_READ_FUNCTIONS(ACTION, result_type, encoding, ARG ) \
    ACTION (result_type, R##encoding,   (_ITM_transaction *, const result_type *), ARG) \
    ACTION (result_type, RaR##encoding, (_ITM_transaction *, const result_type *), ARG) \
    ACTION (result_type, RaW##encoding, (_ITM_transaction *, const result_type *), ARG) \
    ACTION (result_type, RfW##encoding, (_ITM_transaction *, const result_type *), ARG)         

#  define _ITM_GENERATE_WRITE_FUNCTIONS(ACTION, result_type, encoding, ARG ) \
    ACTION (void, W##encoding,  (_ITM_transaction *, result_type *, result_type), ARG) \
    ACTION (void, WaR##encoding,(_ITM_transaction *, result_type *, result_type), ARG) \
    ACTION (void, WaW##encoding,(_ITM_transaction *, result_type *, result_type), ARG)

#  define _ITM_GENERATE_LOG_FUNCTIONS(ACTION, result_type, encoding, ARG ) \
    ACTION (void, L##encoding,   (_ITM_transaction *, const result_type *), ARG) 

# define GENERATE_PROTOTYPE(result, function, args, ARG)    \
    _ITM_EXPORT extern result _ITM_CALL_CONVENTION _ITM_##function args;

_ITM_FOREACH_TRANSFER (GENERATE_PROTOTYPE,dummy)
# undef GENERATE_PROTOTYPE

# ifdef __cplusplus
} /* extern "C" */
# endif                          /* __cplusplus */

#endif /* defined(_ITM09_H) */
