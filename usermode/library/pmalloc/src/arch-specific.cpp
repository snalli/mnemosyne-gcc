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

#include <pm_instr.h>
#include <assert.h>
#include "arch-specific.h"

// How many iterations we spin waiting for a lock.
enum { SPIN_LIMIT = 100 };

// The values of a user-level lock.
enum { UNLOCKED = 0, LOCKED = 1 };

extern "C" {

void hoardCreateThread (hoardThreadType& t,
			void *(*function) (void *),
			void * arg)
{
  pthread_attr_t attr;
  pthread_attr_init (&attr);
  pthread_create (&t, &attr, function, arg);
}

void hoardJoinThread (hoardThreadType& t)
{
  pthread_join (t, NULL);
}

#if 0 // DISABLED!
#if defined(__linux)
// This extern declaration is required for some versions of Linux.
extern "C" void pthread_setconcurrency (int n);
#endif
#endif

void hoardSetConcurrency (int n)
{
  pthread_setconcurrency (n);
}


int hoardGetThreadID (void) {
  // Consecutive thread id's in Linux are 1024 apart.
  return (int) pthread_self() / 1024;
	// Why is this by-1024 ? If pthread_self < 1024, the return will always be 0 and all threads will map to the same heap !
	// Further, pthread_self returns pthread_t which must be considered as opaque data and not int !
}


// Here's our own lock implementation (spin then yield). This is much
// cheaper than the ordinary mutex, at least on Linux and Solaris.

#if USER_LOCKS

#include <sched.h>

#if defined(__sgi)
#include <mutex.h>
#endif


// Atomically:
//   retval = *oldval;
//   *oldval = newval;
//   return retval;

#if defined(sparc) && !defined(__GNUC__)
extern "C" unsigned long InterlockedExchange (unsigned long * oldval,
					      unsigned long newval);
#else
unsigned long InterlockedExchange (unsigned long * oldval,
						 unsigned long newval)
{
#if defined(sparc)
  asm volatile ("swap [%1],%0"
		:"=r" (newval)
		:"r" (oldval), "0" (newval)
		: "memory");

#endif
#if defined(i386)
  asm volatile ("xchgl %0, %1"
		: "=r" (newval)
		: "m" (*oldval), "0" (newval)
		: "memory");
#endif
#if defined(__sgi)
  newval = test_and_set (oldval, newval);
#endif
#if defined(ppc)
  int ret;
  asm volatile ("sync;"
		"0:    lwarx %0,0,%1 ;"
		"      xor. %0,%3,%0;"
		"      bne 1f;"
		"      stwcx. %2,0,%1;"
		"      bne- 0b;"
		"1:    sync"
	: "=&r"(ret)
	: "r"(oldval), "r"(newval), "r"(*oldval)
	: "cr0", "memory");
#endif
#if !(defined(sparc) || defined(i386) || defined(__sgi) || defined(ppc))
#error "Hoard does not include support for user-level locks for this platform."
#endif
  return newval;
}
#endif

unsigned long hoardInterlockedExchange (unsigned long * oldval,
					unsigned long newval)
{
  return InterlockedExchange (oldval, newval);
}

void hoardLockInit (hoardLockType& mutex) {
  InterlockedExchange (&mutex, UNLOCKED);
}

#include <stdio.h>
__TM_CALLABLE__
void hoardLock (hoardLockType& mutex) {
  int spincount = 0;
  while (InterlockedExchange (&mutex, LOCKED) != UNLOCKED) {
    spincount++;
    if (spincount > 100) {
      hoardYield();
      spincount = 0;
    }
  }
}

__TM_CALLABLE__
void hoardUnlock (hoardLockType& mutex) {
	mutex = UNLOCKED;
//  InterlockedExchange (&mutex, UNLOCKED);
}

#else

// use non-user-level locks. 

#endif // USER_LOCKS


void hoardYield (void)
{
  sched_yield();
}


extern "C" void *malloc (size_t);
extern "C" void free (void *);


#if USER_LOCKS
static hoardLockType getMemoryLock = 0;
#else
static hoardLockType getMemoryLock = PTHREAD_MUTEX_INITIALIZER;
#endif

#include <stdio.h>

__TM_CALLABLE__
void * hoardGetMemory (long size) {
  hoardLock (getMemoryLock);
  void * ptr = malloc (size);
  hoardUnlock (getMemoryLock);
  return ptr;
}


void hoardFreeMemory (void * ptr)
{
  hoardLock (getMemoryLock);
  free (ptr);
  hoardUnlock (getMemoryLock);
}


int hoardGetPageSize (void)
{
  return (int) sysconf(_SC_PAGESIZE);
}


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


int hoardGetNumProcessors (void)
{
	static int numProcessors = 0;

	if (numProcessors == 0) {
		// Ugly workaround.  Linux's sysconf indirectly calls malloc() (at
		// least on multiprocessors).  So we just read the info from the
		// proc file ourselves and count the occurrences of the word
		// "processor".
		
		// We only parse the first 32K of the CPU file.  By my estimates,
		// that should be more than enough for at least 64 processors.
		enum { MAX_PROCFILE_SIZE = 32768 };
		char line[MAX_PROCFILE_SIZE];
		int fd = open ("/proc/cpuinfo", O_RDONLY);
		assert (fd);
		read(fd, line, MAX_PROCFILE_SIZE);
		char * str = line;
		while (str) {
			str = strstr(str, "processor");
			if (str) {
				numProcessors++;
				str++;
			}
		}
		close (fd);
		//FIXME: Currently the uniprocessos control path is broken, so force 
		//the multiprocessor one even in the case of a single processor
		if (numProcessors == 1) {
			numProcessors = 2;
		}
		assert (numProcessors > 0);
	}
	return numProcessors;
}

} /* extern "C" */
