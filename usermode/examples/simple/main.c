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
int i = 0;
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
	while (i*sz < 16384) {
		PTx { ptr = pmalloc(sz); }
		++i;
	}
	printf("OK: total size = %d\n", i*sz);
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

void
termination_handler (int signum)
{
	printf("FAIL : total size = %d\n", i*sz);
	exit(-1);
}

int main (int argc, char const *argv[])
{
  struct sigaction new_action, old_action;

  /* Set up the structure to specify the new action. */
  new_action.sa_handler = termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;

  sigaction (SIGSEGV, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGSEGV, &new_action, NULL);

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
