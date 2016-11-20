/*!
 * \file
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
#include "pvar.h"
#include <stdio.h>
#include <stdlib.h>
#include <pmalloc.h>
#include <pthread.h>

unsigned long long cl_mask = 0xffffffffffffffc0;
#define sz 32
#define count 100000
long *ptr, *ptr1;
pthread_t thr[2];

void* reader()
{
	int i = 0;
	long rd;
	while(i++ < count)
	{
		PTx { rd = *ptr1; } 	
	}
}

void* writer()
{
	int i = 0;
	long wrt = 1;
	while(i++ < count)
	{
		PTx { *ptr = wrt; } 	
	}
}

void malloc_bench()
{
	ptr = NULL;
	ptr = pmalloc(sz); /* */
	ptr1 = pmalloc(sz);
	if(ptr != NULL && ptr1 != NULL)
	{
	       printf("ptr =%p, ptr1 =%p, sz=%d\n", ptr, ptr1, sz);
	       printf("ptr & cl_mask = %p, ptr1 & cl_mask = %p\n", (void*)((unsigned long)ptr & cl_mask), (void*)((unsigned long)ptr1 & cl_mask));
	       printf("ptr1-ptr = %ld\n", labs((unsigned long)ptr1-(unsigned long)ptr));
	       pthread_create(&thr[0], NULL,
                          &writer, NULL);

	       pthread_create(&thr[1], NULL,
                          &reader, NULL);
	}

	pthread_join(thr[0], NULL);
	pthread_join(thr[1], NULL);
}

int main (int argc, char const *argv[])
{
	printf("flag: %d", PGET(flag));

	PTx {
		if (PGET(flag) == 0) {
			PSET(flag, 1);
		} else {
			PSET(flag, 0);
		}
	}
	
	printf(" --> %d\n", PGET(flag));
	printf("&flag = %p\n", PADDR(flag));

	malloc_bench();
	return 0;
}
