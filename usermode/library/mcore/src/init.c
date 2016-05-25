/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

#include <pthread.h>
#include <stdlib.h>
#include "reincarnation_callback.h"
#include "segment.h"
#include "log/log_i.h"
#include "thrdesc.h"
#include "debug.h"
#include "config.h"


static pthread_mutex_t global_init_lock = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t  global_init_cond = PTHREAD_COND_INITIALIZER;
//static pthread_mutex_t global_fini_lock = PTHREAD_MUTEX_INITIALIZER;
//static pthread_cond_t  global_fini_cond = PTHREAD_COND_INITIALIZER;
volatile uint32_t      mnemosyne_initialized = 0;


__thread mnemosyne_thrdesc_t *_mnemosyne_thr;

static void do_global_init(void) __attribute__(( constructor ));
static void do_global_fini(void) __attribute__(( destructor ));

void
do_global_init(void)
{
	pcm_storeset_t* pcm_storeset;
#ifdef _M_STATS_BUILD
	struct timeval     start_time;
	struct timeval     stop_time;
	unsigned long long op_time;
#endif

	if (mnemosyne_initialized) {
		return;
	}
	
	pthread_spin_init(&tbuf_lock, PTHREAD_PROCESS_SHARED);
	tbuf = (char*)malloc(MAX_TBUF_SZ);
	if(!tbuf)
		M_ERROR("Failed to allocate trace buffer. Abort now.");
		

	pcm_storeset = pcm_storeset_get ();

	pthread_mutex_lock(&global_init_lock);
	if (!mnemosyne_initialized) {
		mcore_config_init();
#ifdef _M_STATS_BUILD
		gettimeofday(&start_time, NULL);
#endif
		m_segmentmgr_init();
		mnemosyne_initialized = 1;
		mnemosyne_reincarnation_callback_execute_all();
#ifdef _M_STATS_BUILD
		gettimeofday(&stop_time, NULL);
		op_time = 1000000 * (stop_time.tv_sec - start_time.tv_sec) +
		                     stop_time.tv_usec - start_time.tv_usec;
		fprintf(stderr, "reincarnation_latency = %llu (us)\n", op_time);
#endif
		m_logmgr_init(pcm_storeset);
		M_WARNING("Initialize\n");
	}	
	pthread_mutex_unlock(&global_init_lock);
}


void
do_global_fini(void)
{
	if (!mnemosyne_initialized) {
		return;
	}

	pthread_mutex_lock(&global_init_lock);
	if (mnemosyne_initialized) {
		m_logmgr_fini();
		m_segmentmgr_fini();
		mtm_fini_global();
		pthread_spin_lock(&tbuf_lock);
		if(tbuf_sz)
		{
		        tbuf_ptr = 0; 
        	        while(tbuf_ptr < tbuf_sz)       
	                {
				/* 
				 * for some reason tbuf[tbuf] doesn't work
				 * although i thot tbuf + tbuf is same as 
				 * tbuf[tbuf];
				 */
	                        tbuf_ptr = tbuf_ptr + 1 + fprintf(stderr, "%s", tbuf + tbuf_ptr); 
				// tbuf_ptr += TSTR_SZ;
                	}
			tbuf_sz = 0;
		}
		pthread_spin_unlock(&tbuf_lock);
		pthread_spin_destroy(&tbuf_lock);
		mnemosyne_initialized = 0;

		M_WARNING("Shutdown\n");
	}	
	pthread_mutex_unlock(&global_init_lock);
}


void 
mnemosyne_init_global(void)
{
	do_global_init();
}


void 
mnemosyne_fini_global(void)
{
	do_global_fini();
}


mnemosyne_thrdesc_t *
mnemosyne_init_thread(void)
{
	mnemosyne_thrdesc_t *thr = NULL; //FIXME: get thread descriptor value

	if (thr) {
		M_WARNING("mnemosyne_init_thread:1: thr = %p\n", thr);
		return thr;
	}

	mnemosyne_init_global();
	thr = (mnemosyne_thrdesc_t *) malloc(sizeof(mnemosyne_thrdesc_t));
	//TODO: initialize values
	//thr->tx = NULL;
	//thr->vtable = defaultVtables->PTM;
	M_WARNING("mnemosyne_init_thread:2: thr = %p\n", thr);
	return thr;
}


void
mnemosyne_fini_thread(void)
{
	M_WARNING("mnemosyne_fini_thread:2\n");
}
