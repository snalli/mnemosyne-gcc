///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// The Hoard Multiprocessor Memory Allocator
// www.hoard.org
//
// Author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2001, The University of Texas at Austin.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation, http://www.fsf.org.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _ARCH_SPECIFIC_H_
#define _ARCH_SPECIFIC_H_

#include "config.h"

#include <pthread.h>
#include <unistd.h>


#if USER_LOCKS && (defined(i386) || defined(sparc) || defined(__sgi))
typedef unsigned long	hoardLockType;
#else
typedef pthread_mutex_t	hoardLockType;
#endif

typedef pthread_t		hoardThreadType;


extern "C" {

  ///// Thread-related wrappers.

  void	hoardCreateThread (hoardThreadType& t,
			   void *(*function) (void *),
			   void * arg);
  void	hoardJoinThread (hoardThreadType& t);
  void  hoardSetConcurrency (int n);

  // Return a thread identifier appropriate for hashing:
  // if the system doesn't produce consecutive thread id's,
  // some hackery may be necessary.
  int	hoardGetThreadID (void);

  ///// Lock-related wrappers.

#if !USER_LOCKS

  // Define the lock operations inline to save a little overhead.

  inline void hoardLockInit (hoardLockType& lock) {
    pthread_mutex_init (&lock, NULL);
  }

  inline void hoardLock (hoardLockType& lock) {
    pthread_mutex_lock (&lock);
  }

  inline void hoardUnlock (hoardLockType& lock) {
    pthread_mutex_unlock (&lock);
  }

#endif

  inline void  hoardLockDestroy (hoardLockType&) {
  }

  ///// Memory-related wrapper.

  int	hoardGetPageSize (void);
  void *	hoardGetMemory (long size);
  void	hoardFreeMemory (void * ptr);

  ///// Other.

  void  hoardYield (void);
  int	hoardGetNumProcessors (void);
  unsigned long hoardInterlockedExchange (unsigned long * oldval,
					  unsigned long newval);
}

#endif // _ARCH_SPECIFIC_H_

