/* =============================================================================
 *
 * ptm/tm.h
 *
 * Utility defines for transactional memory for sequential
 *
 * =============================================================================
 *
 * Copyright (C) Stanford University, 2006.  All Rights Reserved.
 * Authors: Chi Cao Minh and Martin Trautmann
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of ssca2, please see ssca2/COPYRIGHT
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 * 
 * ------------------------------------------------------------------------
 * 
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 * 
 * ------------------------------------------------------------------------
 * 
 * Unless otherwise noted, the following license applies to STAMP files:
 * 
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 * 
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#ifndef TM_H
#define TM_H 1

#include "../tm_common.h"
#include <ptx.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#define debug_race 0

extern struct timeval v_time;
#define __m_tid__                 syscall(SYS_gettid)
#define __m_time__                ({gettimeofday(&v_time, NULL); \
                                        (double)v_time.tv_sec +    \
                                        ((double)v_time.tv_usec / 1000000);})

#if debug_race
#define __m_debug()              fprintf(stderr, "%lf %d %s-%d \n",__m_time__,__m_tid__, __func__, __LINE__);
#define __m_fail()               fprintf(stderr, "FAIL %lf %d %s-%d \n",__m_time__,__m_tid__, __func__, __LINE__);
#define __m_print(args ...)      fprintf(stderr, args);
#define __m_print_d(s,d)         fprintf(stderr, "%lf %d %s-%d %s=%d\n",__m_time__,__m_tid__, __func__, __LINE__,s,d);
#define __m_print_p(s,p)         fprintf(stderr, "%lf %d %s-%d %s=%p\n",__m_time__,__m_tid__, __func__, __LINE__,s,p);
#define __m_print_s(s)           fprintf(stderr, "%lf %d %s-%d %s\n",__m_time__,__m_tid__, __func__, __LINE__,s);
#else
#define __m_debug()              {;}
#define __m_fail()               {;}
#define __m_print(args ...)      {;}
#define __m_print_d(s,d)      	 {;}
#define __m_print_p(s,d)      	 {;}
#define __m_print_s(s)      	 {;}
#endif


/* =============================================================================
 * Transactional Memory System Interface
 *
 * TM_ARG
 * TM_ARG_ALONE
 * TM_ARGDECL
 * TM_ARGDECL_ALONE
 *     Used to pass TM thread meta data to functions (see Examples below)
 *
 * TM_STARTUP(numThread)
 *     Startup the TM system (call before any other TM calls)
 *
 * TM_SHUTDOWN()
 *     Shutdown the TM system
 *
 * TM_THREAD_ENTER()
 *     Call when thread first enters parallel region
 *
 * TM_THREAD_EXIT()
 *     Call when thread exits last parallel region
 *
 * P_MALLOC(size)
 *     Allocate memory inside parallel region
 *
 * P_FREE(ptr)
 *     Deallocate memory inside parallel region
 *
 * TM_MALLOC(size)
 *     Allocate memory inside atomic block / transaction
 *
 * TM_FREE(ptr)
 *     Deallocate memory inside atomic block / transaction
 *
 * TM_BEGIN()
 *     Begin atomic block / transaction
 *
 * TM_BEGIN_RO()
 *     Begin atomic block / transaction that only reads shared data
 *
 * TM_END()
 *     End atomic block / transaction
 *
 * TM_RESTART()
 *     Restart atomic block / transaction
 *
 * TM_EARLY_RELEASE()
 *     Remove speculatively read line from the read set
 *
 * =============================================================================
 *
 * Example Usage:
 *
 *     MAIN(argc,argv)
 *     {
 *         TM_STARTUP(8);
 *         // create 8 threads and go parallel
 *         TM_SHUTDOWN();
 *     }
 *
 *     void parallel_region ()
 *     {
 *         TM_THREAD_ENTER();
 *         subfunction1(TM_ARG_ALONE);
 *         subfunction2(TM_ARG  1, 2, 3);
 *         TM_THREAD_EXIT();
 *     }
 *
 *     void subfunction1 (TM_ARGDECL_ALONE)
 *     {
 *         TM_BEGIN_RO()
 *         // ... do work that only reads shared data ...
 *         TM_END()
 *
 *         long* array = (long*)P_MALLOC(10 * sizeof(long));
 *         // ... do work ...
 *         P_FREE(array);
 *     }
 *
 *     void subfunction2 (TM_ARGDECL  long a, long b, long c)
 *     {
 *         TM_BEGIN();
 *         long* array = (long*)TM_MALLOC(a * b * c * sizeof(long));
 *         // ... do work that may read or write shared data ...
 *         TM_FREE(array);
 *         TM_END();
 *     }
 *
 * =============================================================================
 */


#include <assert.h>
#include <unistd.h>
#include <sys/syscall.h>

extern int init_user;
extern unsigned long long v_mem_total;
extern unsigned long long nv_mem_total;
extern unsigned long v_free, nv_free;


#define OUT			      stderr
#define TM_ARG                        /* nothing */
#define TM_ARG_ALONE                  /* nothing */
#define TM_ARGDECL                    /* nothing */
#define TM_ARGDECL_ALONE              /* nothing */

#define TM_STARTUP(numThread)         /* nothing */
#define TM_SHUTDOWN()                 /* nothing */

#define TM_THREAD_ENTER()             fprintf(OUT, "Starting thread %li\n", syscall(SYS_gettid));
#define TM_THREAD_EXIT()              fprintf(OUT, "THREAD %lu executed %llu epochs\n", __m_tid__,\
						get_epoch_count());

#define VACATION_DEBUG 0
#define VACATION_MANAGER_DEBUG 0
#define VACATION_LIST_DEBUG 0

#define P_MALLOC(size)              		\
	({ 					\
		void *vptr = malloc(size); 	\
		if(vptr && init_user)		\
			v_mem_total += size;	\
		vptr;				\
	})
#define P_FREE(ptr)                 		\
	({					\
		void *vptr = ptr;		\
		if(vptr && init_user)		\
			++v_free;		\
		free(ptr);			\
	})
#define TM_MALLOC(size)             		\
	({					\
		void *nvptr = pmalloc(size);	\
		if(nvptr && init_user)		\
			nv_mem_total += size;	\
		nvptr;				\
	})
#define TM_FREE(ptr)                		\
	({					\
		void *nvptr = ptr;		\
		if(nvptr && init_user)		\
			++nv_free;		\
		pfree(ptr);			\
	})

#define TM_BEGIN()                    PTx {
#define TM_BEGIN_RO()                 PTx { // What is the txn spans across multiple routines ?
#define TM_END()                      }
#define TM_RESTART()                  assert(0); // ICC : __tm_retry, GCC : unsup 

#define TM_EARLY_RELEASE(var)         /* nothing */

/* =============================================================================
 * Transactional Memory System interface for shared memory accesses
 *
 * There are 3 flavors of each function:
 *
 * 1) no suffix: for accessing variables of size "long"
 * 2) _P suffix: for accessing variables of type "pointer"
 * 3) _F suffix: for accessing variables of type "float"
 * =============================================================================
 */
#define TM_SHARED_READ(var)           (var)
#define TM_SHARED_READ_P(var)         (var)
#define TM_SHARED_READ_F(var)         (var)

#define TM_SHARED_WRITE(var, val)     ({var = val; var;})
#define TM_SHARED_WRITE_P(var, val)   ({var = val; var;})
#define TM_SHARED_WRITE_F(var, val)   ({var = val; var;})

#define TM_LOCAL_WRITE(var, val)      ({var = val; var;})
#define TM_LOCAL_WRITE_P(var, val)    ({var = val; var;})
#define TM_LOCAL_WRITE_F(var, val)    ({var = val; var;})

#endif /* TM_H */


/* =============================================================================
 *
 * End of seq/tm.h
 *
 * =============================================================================
 */
