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

// Wrap architecture-specific functions.

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)

// Windows

#ifndef WIN32
#define WIN32 1
#endif

// Set maximal inlining.
#pragma inline_depth(255)
#define inline __forceinline

#include <windows.h>
#include <process.h>
//typedef CRITICAL_SECTION	hoardLockType;
typedef long hoardLockType;
typedef HANDLE 	hoardThreadType;

#elif USE_SPROC

// SGI's SPROC library

#include <sys/types.h>
#include <sys/prctl.h>

#else

// Generic UNIX

#if defined(__SVR4) // Solaris
#include <thread.h>
#endif

#include <pthread.h>
#include <unistd.h>

#endif


#ifndef WIN32
#if USER_LOCKS && (defined(i386) || defined(sparc) || defined(__sgi))
typedef unsigned long	hoardLockType;
#else
typedef pthread_mutex_t	hoardLockType;
#endif

#if USE_SPROC
typedef pid_t			hoardThreadType;
#else
typedef pthread_t		hoardThreadType;
#endif

#endif

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

#if !defined(WIN32) && !USER_LOCKS

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

#else
  void	hoardLockInit (hoardLockType& lock);
  void	hoardLock (hoardLockType& lock);
  void	hoardUnlock (hoardLockType& lock);
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

