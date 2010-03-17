/*

  rrconsume

  the round-robin producer-consumer program described (as an example
  that induces P-fold blowup in private-heaps-with-ownership
  allocators) in Berger et al, ASPLOS 2000.

  thread (i % P) allocates memory and thread (i+1 % P) frees it.

  Copyright (C) 2001 by Emery Berger
  http://www.cs.utexas.edu/users/emery
  emery@cs.utexas.edu 

*/

#ifndef NDEBUG

#if 1
#define PRINTF(x) printf(x)
#define PRINTF1(x,y) printf(x,y)
#define PRINTF2(x,y,z) printf(x,y,z)
#define PRINTF3(x,y,z,a) printf(x,y,z,a)
#else
#include <windows.h>
#define PRINTF(x) {char szMsg[255]; wsprintf(szMsg, x); MessageBox(NULL,szMsg,"rrconsume", MB_OK);}
#define PRINTF1(x,y) {char szMsg[255]; wsprintf(szMsg, x, y); MessageBox(NULL,szMsg,"rrconsume", MB_OK);}
#define PRINTF2(x,y,z) printf(x,y,z)
#endif

#else
#define PRINTF(x)
#define PRINTF1(x,y)
#define PRINTF2(x,y,z)
#define PRINTF3(x,y,z,a)
#endif

#define WIN32 1
#undef linux
#undef unix

#include <stdlib.h>
#include <iostream.h>
#include <stdio.h>

#if defined(__SVR4) // Solaris
#define THREADMODEL 1
#include <pthread.h>
#include <synch.h>
typedef sema_t sem_t;
#define sem_init(a,b,c) sema_init(a,b,c,NULL)
#define sem_wait(a) sema_wait(a)
#define sem_post(a) sema_post(a)
#endif


#if defined(WIN32) // Windows
#define THREADMODEL 1
#include <windows.h>
#include <process.h>
typedef HANDLE sem_t;
#define sem_init(a,b,c) (*(a) = CreateSemaphore(NULL, 0, 255, NULL))
#define sem_wait(a) WaitForSingleObject(*(a), INFINITE)
#define sem_post(a) ReleaseSemaphore(*(a), 1, NULL)
typedef unsigned long pthread_attr_t;
typedef HANDLE pthread_t;
typedef DWORD WINAPI ThreadProc(
  LPVOID lpParameter   // thread data
);
#define pthread_attr_init(pa)
#define pthread_attr_setscope(pa,s)
#define pthread_create(pt,pa,fn,arg) (*(pt) = CreateThread(NULL,0,(ThreadProc *) fn,arg,0,NULL))
#define pthread_join(pt,res) WaitForSingleObject((pt), INFINITE)
//#define SBRK(a) wsbrk(a)
extern "C" void * wsbrk (long);
#endif


#ifndef THREADMODEL
#include <pthread.h>
#include <semaphore.h>
#endif


class parameters {
public:
  parameters (void)
    : iterations (0),
      self (-1)
  {}

  parameters (int it, int tid)
    : iterations (it),
      self (tid)
  {}

  int iterations;
  int self; // thread-id
};

typedef int bufObjType;

const int MAX_THREADS = 64;

bufObjType *** bufObjs;	// The array of pointers to allocated objects.
sem_t startConsumer[MAX_THREADS];
sem_t startProducer[MAX_THREADS];

int nthreads = 2;
int niterations = 1;
int bufferSize = 1; // = number of objects.

void * consumer (void * arg)
{
  parameters * p = (parameters *) arg;
  // Wait for the start signal, delete all of the objects,
  // say we're done, then stop.
  // PRINTF1 ("consumer %d ready.\n", p->self);
  sem_wait (&startConsumer[p->self]);
  for (int j = 0; j < bufferSize; j++) {
    delete bufObjs[p->self][j];
  }
  PRINTF1 ("consumer %d done.\n", p->self);
  sem_post (&startProducer[p->self]);
  return NULL;
}


void * producer (void * arg)
{
  parameters * p = (parameters *) arg;
  // Wait until the previous consumer is done,
  // allocate some objects, and wake up the next consumer.

  sem_wait (&startProducer[p->self]);
  for (int k = 0; k < bufferSize; k++) {
    bufObjs[p->self][k] = new bufObjType;
  }
  PRINTF1 ("producer %d done.\n", p->self);
  sem_post (&startConsumer[(p->self + 1) % nthreads]);
  return NULL;
}

extern "C" void malloc_stats (void);


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

  if (nthreads > MAX_THREADS) {
    printf ("Error: The number of threads must be no more than %d.\n", MAX_THREADS);
    exit(1);
  }

  // Initialize the semaphores.

  for (int i = 0; i < nthreads; i++) {
    sem_init (&startConsumer[i], 0, 0);
    sem_init (&startProducer[i], 0, 0);
  }

  pthread_t consume[nthreads];
  pthread_t produce[nthreads];
  parameters parms[nthreads];
  
  pthread_attr_t consume_attr[nthreads];
  pthread_attr_t produce_attr[nthreads];

  bufObjs = new bufObjType ** [nthreads];
  for (int bo = 0; bo < nthreads; bo++) {
    bufObjs[bo] = new bufObjType * [bufferSize];
  }


  // Set all of the threads to system-scope (i.e., multiplexed onto
  // kernel threads).

  for (int i = 0; i < nthreads; i++) {
    pthread_attr_init (&consume_attr[i]);
    pthread_attr_setscope (&consume_attr[i], PTHREAD_SCOPE_SYSTEM);
    pthread_attr_init (&produce_attr[i]);
    pthread_attr_setscope (&produce_attr[i], PTHREAD_SCOPE_SYSTEM);
  }

  malloc_stats();

#if defined(SBRK)
  int initialBreak = (int) SBRK(0);
  cout << "Current break value = " << initialBreak << endl;
#endif

  // Gentlemen, start your engines.

  for (int it = 0; it < niterations; it++) {
    for (int i = 0; i < nthreads; i++) {
      parms[i] = parameters (it, i);
      pthread_create (&produce[i], &produce_attr[i], producer, (void *) &parms[i]);
      pthread_create (&consume[i], &consume_attr[i], consumer, (void *) &parms[i]);
    }

    // Start the producer.
    sem_post (&startProducer[0]);

    // Now wait for everything to stop.
    void * result;
    for (int i = 0; i < nthreads; i++) {
      pthread_join (produce[i], &result);
      pthread_join (consume[i], &result);
    }
  }

  malloc_stats();

#if defined(SBRK)
  cout << "Memory consumed (via sbrk) = " << ((int) SBRK(0)) - initialBreak << endl;
#endif
}
