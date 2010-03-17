///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// Hoard: A Fast, Scalable, and Memory-Efficient Allocator
//        for Shared-Memory Multiprocessors
// Contact author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2000, The University of Texas at Austin.
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

/*
  cache-thrash.cpp
  ------------------------------------------------------------------------
  cache-thrash is a benchmark that exercises a heap's cache-locality.
  ------------------------------------------------------------------------
  @(#) $Id: cache-thrash.cpp,v 1.1 2000/04/19 16:23:30 emery Exp $
  ------------------------------------------------------------------------
  Emery Berger                    | <http://www.cs.utexas.edu/users/emery>
  Department of Computer Sciences |             <http://www.cs.utexas.edu>
  University of Texas at Austin   |                <http://www.utexas.edu>
  ========================================================================
*/

/* Try the following (on a P-processor machine):

   cache-thrash 1 1000 1 1000000
   cache-thrash P 1000 1 1000000

   cache-thrash-hoard 1 1000 1 1000000
   cache-thrash-hoard P 1000 1 1000000

   The ideal is a P-fold speedup.
*/


#include "arch-specific.h"
#include "timer.h"

#include <iostream.h>
#include <stdlib.h>

// This class just holds arguments to each thread.
class workerArg {
public:
  workerArg (int objSize, int repetitions, int iterations)
    : _objSize (objSize),
      _iterations (iterations),
      _repetitions (repetitions)
  {}

  int _objSize;
  int _iterations;
  int _repetitions;
};


extern "C" void * worker (void * arg)
{
  // Repeatedly do the following:
  //   malloc a given-sized object,
  //   repeatedly write on it,
  //   then free it.
  workerArg * w = (workerArg *) arg;
  for (int i = 0; i < w->_iterations; i++) {
    // Allocate the object.
    char * obj = new char[w->_objSize];
    // Write into it a bunch of times.
    for (int j = 0; j < w->_repetitions; j++) {
      for (int k = 0; k < w->_objSize; k++) {
	obj[k] = (char) k;
	volatile char ch = obj[k];
	ch++;
      }
    }
    // Free the object.
    delete [] obj;
  }
  delete w;
  return NULL;
}


int main (int argc, char * argv[])
{
  int nthreads;
  int iterations;
  int objSize;
  int repetitions;

  if (argc > 4) {
    nthreads = atoi(argv[1]);
    iterations = atoi(argv[2]);
    objSize = atoi(argv[3]);
    repetitions = atoi(argv[4]);
  } else {
    cerr << "Usage: " << argv[0] << " nthreads iterations objSize repetitions" << endl;
    exit(1);
  }

  hoardThreadType * threads = new hoardThreadType[nthreads];
  hoardSetConcurrency (hoardGetNumProcessors());

  int i;

  Timer t;
  t.start();

  for (i = 0; i < nthreads; i++) {
    workerArg * w = new workerArg (objSize, repetitions / nthreads, iterations);
    hoardCreateThread (threads[i], worker, (void *) w);
  }
  for (i = 0; i < nthreads; i++) {
    hoardJoinThread (threads[i]);
  }
  t.stop();

  delete [] threads;

  cout << "Time elapsed = " << (double) t << " seconds." << endl;
}
