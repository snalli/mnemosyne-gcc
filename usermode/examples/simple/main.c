/*!
 * \file
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 * \author Sanketh Nalli <nalli@wisc.edu>
 */
#include "pvar.h"
#include <stdio.h>
#include <stdlib.h>
#include <pmalloc.h>
#include <pthread.h>
#include <signal.h>

unsigned long long cl_mask = 0xffffffffffffffc0;
#define sz 32
#define count 100000
long *ptr;
pthread_t thr[2];

void* reader()
{
	printf("(READER) persistent ptr =%p, sz=%d\n", ptr, sz);
	int i = 0;
	long rd;
	while(i++ < count)
	{
		PTx { rd = *ptr; } 	
	}
}

void* writer()
{
	printf("(WRITER) persistent ptr =%p, sz=%d\n", ptr, sz);
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
	PTx { ptr = pmalloc(sz); }
	if(ptr != NULL)
	{
	       printf("persistent ptr =%p, sz=%d\n", ptr, sz);
	       printf("persistent ptr & cl_mask = %p\n", (void*)((unsigned long)ptr & cl_mask));
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
	printf("persistent flag: %d", PGET(flag));

	PTx {
		if (PGET(flag) == 0) {
			PSET(flag, 1);
		} else {
			PSET(flag, 0);
		}
	}
	
	printf(" --> %d\n", PGET(flag));
	printf("&persistent flag = %p\n", PADDR(flag));

	printf("\nstarting malloc bench\n");
	malloc_bench();
	return 0;
}
