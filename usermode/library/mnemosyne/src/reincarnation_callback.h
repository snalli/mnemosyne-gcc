/*!
 * \file
 * Specifies the interfaces by which reincarnation callbacks may be initiated from
 * within initialization routines.
 *
 * The concept here is that callback registration is initiated by 
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef REINCARNATION_CALLBACK_H_WTK12KU8
#define REINCARNATION_CALLBACK_H_WTK12KU8

/*! \see mnemosyne.h */
void mnemosyne_reincarnation_callback_register(void(*initializer)());

/*!
 * Runs all currently-registered callbacks given by clients of the mnemosyne library.
 * These callbacks will execute under the assumption that persistent memory has been
 * mapped for all global objects and that dynamically-allocated persistent segments
 * have been re-mapped.
 */
void mnemosyne_reincarnation_callback_execute_all();

#endif /* end of include guard: REINCARNATION_CALLBACK_H_WTK12KU8 */
