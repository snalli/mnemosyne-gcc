/*!
 * \file
 * Defines interfaces that allow one to determine whether the library has been initialized or not.
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef INIT_H_FY8AG1WS
#define INIT_H_FY8AG1WS

#include <stdint.h>

/*! Greater than zero if mnemosyne has been initialized (or more importantly: reincarnated) */
extern volatile uint32_t mnemosyne_initialized;

#endif /* end of include guard: INIT_H_FY8AG1WS */
