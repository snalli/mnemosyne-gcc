#include  <stdio.h>
#include  <stdlib.h>
#include  <stddef.h>
#include  <string.h>
#include  <ctype.h>
#include  <time.h>

#ifdef __WIN32__
#include  <conio.h>
#include  <crtdbg.h>
#include  <process.h>
#include  <windows.h>
#else
typedef void * LPVOID;
typedef long long LONGLONG;
typedef long DWORD;
typedef long LONG;
typedef union _LARGE_INTEGER {
  struct {
    DWORD LowPart;
    LONG  HighPart;
  };
  LONGLONG QuadPart;    // In Visual C++, a typedef to _ _int64} LARGE_INTEGER;
} LARGE_INTEGER;
typedef long long _int64;
enum { TRUE = 1, FALSE = 0 };
#include <assert.h>
#define _ASSERTE(x) assert(x)
#define pt_malloc_init(x)
void * pt_malloc (int x)
{
  return malloc(x);
}

#define pt_free(x) free(x)
void Sleep (long x) 
{
  printf ("sleeping for %d seconds.\n", x/1000);
  sleep(x/1000);
}

#define QueryPerformanceCounter(x) times(0)
#define QueryPerformanceFrequency(x) CLK_TCK

#define _REENTRANT 1
#include <pthread.h>
typedef void * VoidFunction (void *);
void _beginthread (VoidFunction x, int y, void * z)
{
  pthread_t pt;
  printf ("creating a thread.\n");
  pthread_create(&pt, NULL, x, z);
}
#endif

#include  "perfcounters.h"
/* #include  "ptmalloc.h" */


/* Test driver for memory allocators           */
/* Author: Paul Larson, palarson@microsoft.com */

#define MAX_THREADS     100
#define MAX_BLOCKS  1000000
#define MAX_IIV           1

#define TC_CONTINUE          0
#define TC_PAUSE             1
#define TC_TERMINATE         2

struct lran2_st {
  long x, y, v[97];
};

typedef struct thr_data {

  int    threadno ;
  int    NumBlocks ;
  int    seed ;

  int    min_size ;
  int    max_size ;

  LPVOID *array ;
  int     asize ;

  int    cAllocs ;
  int    cFrees ;
  int    cThreads ;

  PerfCounters  cntrs ;

  int    waitflag ;       
  int    finished ;
  struct lran2_st rgen ;

} thread_data;


static void warmup(LPVOID *blkp, int num_chunks, thread_data *pdea );
static void * exercise_heap( void *pinput) ;
static void lran2_init(struct lran2_st* d, long seed) ;
static long lran2(struct lran2_st* d) ;
 
LPVOID        blkp[MAX_BLOCKS] ;
long          seqlock=0 ;
int           min_size, max_size ;
struct lran2_st rgen ;

extern  int   cAllocedChunks ;
extern  int   cAllocedSpace ;
extern  int   cUsedSpace ;
extern  int   cFreeChunks ;
extern  int   cFreeSpace ;

int main()
{
  thread_data  de_area[MAX_THREADS] ;
  thread_data *pdea;
  int          min_threads, max_threads ;
  int          num_threads, prevthreads ;
  unsigned     seed=12345 ;
  int          sum_allocs ;
  int          sum_frees ;
  double       duration ;
  char        *allocator ;
  int          ii, i ;

  LPVOID        tmp ;
  int           cblks=0 ;
  int           victim ;
  long          blk_size ;
  int           nperthread ;
  int           chperthread ;
  int           num_chunks;
  int           num_rounds ;
  int           sum_threads ;
  /* 	struct mallinfo cur_mallinfo ; */
  LARGE_INTEGER ticks_per_sec ;
  LARGE_INTEGER start_cnt, end_cnt ;
  _int64        ticks ;
  double        rate_1=0, rate_n ;
  double        eused_space ;



  printf( "\nTest driver for memory allocators \n") ;
# ifdef WIN32_CRITICAL_SECTIONS
  allocator = "PT_MALLOC with WIN32 critical sections" ;
# else
  allocator = "PT_MALLOC with spin locks" ;
# endif

  printf("Using %s,\n\n", allocator ) ;
  printf("chunk size (min,max): ") ;
  scanf("%d %d", &min_size, &max_size ) ;
  printf("threads (min, max):   ") ; 
  scanf("%d %d", &min_threads, &max_threads) ;
  printf("chunks/thread: ") ; scanf("%d", &chperthread ) ;
  printf("no of rounds:  ") ; scanf("%d", &num_rounds ) ;
  printf("random seed: ") ;   scanf("%d", &seed) ;

  num_chunks = max_threads*chperthread ;
  if( num_chunks > MAX_BLOCKS ){
    printf("Max %d chunks - exiting\n", MAX_BLOCKS ) ;
    return(1) ;
  }

  lran2_init(&rgen, seed) ;
  pt_malloc_init(void) ;

  QueryPerformanceFrequency( &ticks_per_sec ) ;

  pdea = &de_area[0] ;
  memset(&de_area[0], 0, sizeof(thread_data)) ;

  prevthreads = 0 ;

  for(num_threads=min_threads; num_threads <= max_threads; num_threads++ )
    {

      nperthread = chperthread ;
      warmup(&blkp[prevthreads*chperthread], (num_threads-prevthreads)*chperthread, pdea );
		
      for(i=0; i< num_threads; i++){
	de_area[i].threadno    = i+1 ;
	de_area[i].NumBlocks   = num_rounds*nperthread;
	de_area[i].array       = &blkp[i*nperthread] ;
	de_area[i].asize       = nperthread ;
	de_area[i].min_size    = min_size ;
	de_area[i].max_size    = max_size ;
	de_area[i].seed        = lran2(&rgen) ; ;
	de_area[i].finished    = 0 ;
	de_area[i].cAllocs     = 0 ;
	de_area[i].cFrees      = 0 ;
	de_area[i].cThreads    = 0 ;
	de_area[i].waitflag    = TC_CONTINUE ;
	de_area[i].finished    = FALSE ;
	memset(&de_area[i].cntrs, 0, sizeof(PerfCounters)) ;
	lran2_init(&de_area[i].rgen, de_area[i].seed) ;

	_beginthread(exercise_heap, 0, &de_area[i]) ;  
      }

      for(ii=0; ii<MAX_IIV ; ii++) {
	QueryPerformanceCounter( &start_cnt) ;
	
	Sleep(30000L) ;
	printf ("TIME TO DIE!\n");
	
	for(i=0; i< num_threads; i++) de_area[i].waitflag = TC_PAUSE ;
	
	for(i=0; i< num_threads; i++){
	  while( !de_area[i].finished ){
	    //Sleep(0) ;
	  }
	}
	
	QueryPerformanceCounter( &end_cnt) ;
	
	sum_frees = sum_allocs =0  ;
	sum_threads = 0 ;
	for(i=0;i< num_threads; i++){
	  sum_allocs    += de_area[i].cAllocs ;
	  sum_frees     += de_area[i].cFrees ;
	  sum_threads   += de_area[i].cThreads ;
	  de_area[i].cAllocs = de_area[i].cFrees = 0;
	}
	ticks = end_cnt.QuadPart - start_cnt.QuadPart ;
	duration = (double)ticks/ticks_per_sec.QuadPart ;
	
	rate_n = sum_allocs/duration ;
	if( rate_1 == 0){
	  rate_1 = rate_n ;
	}
	
	//	eused_space = (0.5*(min_size+max_size)*num_chunks) ;
	eused_space = (0.5*(min_size+max_size)*num_threads*chperthread) ;
	
#if 0
	cur_mallinfo = pt_mallinfo() ;
#endif
	printf("%2d ", num_threads ) ;
	printf("%2d ", ii ) ;
	printf("%6.3f", duration  ) ;
	printf("%6.3f", rate_n/rate_1 ) ;
	printf("%8.0f", sum_allocs/duration ) ;
	
#if 0
	printf("  %6.2f", (100.0*(double)cur_mallinfo.uordblks)/eused_space ) ;
	printf(" %6.2f", (100.0*eused_space)/cur_mallinfo.arena ) ;
	//		printf(" %6.2f", (100.0*cur_mallinfo.fordblks)/cur_mallinfo.arena ) ;
	printf(" %6d",   cur_mallinfo.ordblks ) ;
	printf(" %10d",  cur_mallinfo.arena ) ;
#endif
	printf(" %5d",   sum_threads ) ;
	
	printf("\n") ;
	
	
	for(i=0; i< num_threads; i++){
	  if( ii == MAX_IIV-1 ) de_area[i].waitflag = TC_TERMINATE ;
	  else                 de_area[i].waitflag = TC_CONTINUE ;
	}

	Sleep(5000L) ; // wait 5 sec for old threads to die
      }
      prevthreads = num_threads ;

    }


#ifdef _DEBUG
  _cputs("Hit any key to exit...") ;	(void)_getch() ;
#endif

  return(0) ;

} /* main */




static void * exercise_heap( void *pinput)
{
  thread_data  *pdea;
  int           cblks=0 ;
  int           victim ;
  long          blk_size ;
  int           range ;

  //S_LOCK(&seqlock) ;

  pdea = (thread_data *)pinput ;
BEGIN:
  pdea->finished = FALSE ;
  pdea->cThreads++ ;
  range = pdea->max_size - pdea->min_size ;

  /* allocate NumBlocks chunks of random size */
  for( cblks=0; cblks<pdea->NumBlocks; cblks++){
    victim = lran2(&pdea->rgen)%pdea->asize ;
    pt_free(pdea->array[victim]) ;
    pdea->cFrees++ ;

    blk_size = pdea->min_size+lran2(&pdea->rgen)%range ;
    pdea->array[victim] = pt_malloc(blk_size) ;
    _ASSERTE(pdea->array[victim] != NULL) ;
    pdea->cAllocs++ ;
  }

  pdea->finished = TRUE ;
  while( pdea->waitflag == TC_PAUSE){
    Sleep(0) ;
  }
  if( pdea->waitflag == TC_CONTINUE){
    _beginthread(exercise_heap, 0, pdea) ;
  }
  //	S_UNLOCK(&seqlock) ;

  printf("Thread %u terminating: %d allocs, %d frees\n",
	 pdea->threadno, pdea->cAllocs, pdea->cFrees) ;

  return (void *) 0;

}

static void warmup(LPVOID *blkp, int num_chunks, thread_data *pdea )
{
  int     cblks ;
  int     victim ;
  int     blk_size ;
  LPVOID  tmp ;


  for( cblks=0; cblks<num_chunks; cblks++){
    blk_size = min_size+lran2(&rgen)%(max_size-min_size) ;
    blkp[cblks] = pt_malloc(blk_size) ;
    _ASSERTE(blkp[cblks] != NULL) ;
  }

  /* generate a random permutation of the chunks */
  for( cblks=num_chunks; cblks > 0 ; cblks--){
    victim = lran2(&rgen)%cblks ;
    tmp = blkp[victim] ;
    blkp[victim]  = blkp[cblks-1] ;
    blkp[cblks-1] = tmp ;
  }

  for( cblks=0; cblks<4*num_chunks; cblks++){
    victim = lran2(&rgen)%num_chunks ;
    pt_free(blkp[victim]) ;

    blk_size = min_size+lran2(&rgen)%(max_size - min_size) ;
    blkp[victim] = pt_malloc(blk_size) ;
    _ASSERTE(blkp[victim] != NULL) ;
  }
}

// =======================================================

/* lran2.h
 * by Wolfram Gloger 1996.
 *
 * A small, portable pseudo-random number generator.
 */

#ifndef _LRAN2_H
#define _LRAN2_H

#define LRAN2_MAX 714025l /* constants for portable */
#define IA	  1366l	  /* random number generator */
#define IC	  150889l /* (see e.g. `Numerical Recipes') */

//struct lran2_st {
//    long x, y, v[97];
//};

static void
lran2_init(struct lran2_st* d, long seed)
{
  long x;
  int j;

  x = (IC - seed) % LRAN2_MAX;
  if(x < 0) x = -x;
  for(j=0; j<97; j++) {
    x = (IA*x + IC) % LRAN2_MAX;
    d->v[j] = x;
  }
  d->x = (IA*x + IC) % LRAN2_MAX;
  d->y = d->x;
}


static long
lran2(struct lran2_st* d)
{
  int j = (d->y % 97);

  d->y = d->v[j];
  d->x = (IA*d->x + IC) % LRAN2_MAX;
  d->v[j] = d->x;
  return d->y;
}

#undef IA
#undef IC

#endif


