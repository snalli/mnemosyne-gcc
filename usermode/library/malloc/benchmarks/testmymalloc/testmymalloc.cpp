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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

class Foo {
public:
  Foo (void)
    : x (143.0)
    {}

  double x;
};


int main (int argc, char * argv[])
{
  int iterations;
  int size;

  if (argc > 2) {
    iterations = atoi(argv[1]);
    size = atoi(argv[2]);
  } else {
    fprintf (stderr, "Usage: %s iterations size\n", argv[0]);
    exit(1);
  }

  Timer t1, t2;

  long int i, j;
  Foo ** a;
  a = new Foo * [iterations];

  for (int n = 0; n < 1; n++) {
    t1.start ();
    for (i = 0; i < iterations; i++) {
      a[i] = new Foo[size];
    }
    t1.stop ();
    
    t2.start ();
    for (j = iterations - 1; j >= 0; j--) {
      delete [] a[j];
    }
    t2.stop ();
  }

  delete [] a;

  printf( "Alloc: %f\n", (double) t1);
  printf("free: %f\n", (double) t2);
  printf("total: %f\n", (double) t1 + (double) t2);

  return 0;
}
