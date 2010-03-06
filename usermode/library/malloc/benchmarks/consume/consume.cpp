/*

  consume:

  One thread (the producer) generates n blocks of
  allocated memory.

  Each consumer thread then gets one of the blocks
  and frees it.

  This is a stress test for memory consumption for
  private heaps allocators.

*/

#include <stdlib.h>
#include <iostream.h>
#include <stdio.h>
#include <pthread.h>

#if __SVR4 // Solaris
#include <synch.h>
typedef sema_t sem_t;
#define sem_init(a,b,c) sema_init(a,b,c,NULL)
#define sem_wait(a) sema_wait(a)
#define sem_post(a) sema_post(a)
#else
#include <semaphore.h>
#endif


#include <timer.h>

typedef struct {
  int iterations;
  int self;
} parameters;

typedef int bufObjType;

bufObjType *** bufObjs;	// The array of pointers to allocated objects.
sem_t bufferSemaphore[64];
sem_t consumed[64];

int nthreads;
int niterations = 16800;
int bufferSize;

void * consumer (void * arg)
{
  parameters * parms = (parameters *) arg;

  for (int i = 0; i < niterations; i++) {
    sem_wait (&bufferSemaphore[parms->self]);
    for (int j = 0; j < bufferSize; j++) {
      delete bufObjs[parms->self][j];
    }
    sem_post (&consumed[parms->self]);
  }
  return NULL;
}


void * producer (void * arg)
{
  for (int i = 0; i < niterations; i++) {
    for (int j = 0; j < nthreads; j++) {
      for (int k = 0; k < bufferSize; k++) {
	bufObjs[j][k] = new bufObjType;
	*bufObjs[j][k] = 12;
      }
      sem_post (&bufferSemaphore[j]);
    }
    for (int j = 0; j < nthreads; j++) {
      sem_wait (&consumed[j]);
    }
  }
  return NULL;
}


int main (int argc, char * argv[])
{
  if (argc > 3) {
    nthreads = atoi(argv[1]);
    niterations = atoi(argv[2]);
    bufferSize = atoi(argv[3]);
  } else {
    cerr << "Usage: " << argv[0] << " nthreads niterations bufferSize" << endl;
    exit (1);
  }

  if (nthreads > 64) {
    printf ("Error: The number of threads must be no more than 64.\n");
    exit(1);
  }

  for (int i = 0; i < nthreads; i++) {
    sem_init (&bufferSemaphore[i], 0, 0);
    sem_init (&consumed[i], 0, 0);
  }

  pthread_t consume[nthreads];
  pthread_t produce;
  
  pthread_attr_t consume_attr[nthreads];
  pthread_attr_t produce_attr;

  bufObjs = new bufObjType ** [nthreads];

  for (int i = 0; i < nthreads; i++) {
    bufObjs[i] = new bufObjType * [bufferSize];
    pthread_attr_init (&consume_attr[i]);
    pthread_attr_setscope (&consume_attr[i], PTHREAD_SCOPE_SYSTEM);
  }

  pthread_attr_init (&produce_attr);
  pthread_attr_setscope (&produce_attr, PTHREAD_SCOPE_SYSTEM);

  Timer t;
  t.start();

  for (int i = 0; i < nthreads; i++) {
    parameters * p = new parameters;
    p->self = i;
    pthread_create (&consume[i], &consume_attr[i], consumer, (void *) p);
  }

  pthread_create (&produce, &produce_attr, producer, NULL);

  void * result;

  for (int i = 0; i < nthreads; i++) {
    pthread_join (consume[i], &result);
  }
  pthread_join (produce, &result);

  t.stop();
  cout << "Time elapsed: " << (double) t << " seconds." << endl;
}
