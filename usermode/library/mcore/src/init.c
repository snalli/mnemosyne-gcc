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
#include <sys/mman.h>
#include "reincarnation_callback.h"
#include "segment.h"
#include "log/log_i.h"
#include "thrdesc.h"
#include "debug.h"
#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
	
	#ifdef _ENABLE_TRACE
        gettimeofday(&glb_time, NULL);          
        glb_tv_sec  = glb_time.tv_sec;
        glb_tv_usec = glb_time.tv_usec;
	glb_start_time = glb_tv_sec * 1000000 + glb_tv_usec;

	pthread_spin_init(&tbuf_lock, PTHREAD_PROCESS_SHARED);
	/* tbuf = (char*)malloc(MAX_TBUF_SZ); To avoid interaction with M's hoard */
	tbuf = (char*)mmap(0, MAX_TBUF_SZ, PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	/* MAZ_TBUF_SZ influences how often we compress and hence the overall execution speed. */
	if(!tbuf)
		M_WARNING("Failed to allocate trace buffer. Abort now.");
	#elif _ENABLE_FTRACE
        int debug_fd = -1, ret = 0;
        assert(trace_marker == -1);
        assert(tracing_on == -1);

        /* Turn off tracing from previous sessions */
        debug_fd = open("/sys/kernel/debug/tracing/tracing_on", O_WRONLY);
        if(debug_fd != -1){ ret = write(debug_fd, "0", 1); }
        else{ ret = -1; goto fail; }
        close(debug_fd);

        /* Emtpy trace buffer */
        debug_fd = open("/sys/kernel/debug/tracing/current_tracer", O_WRONLY);
        if(debug_fd != -1){ ret = write(debug_fd, "nop", 3); }
        else{ ret = -2; goto fail; }
        close(debug_fd);

        /* Pick a routine that EXISTS but will never be called, VVV IMP !*/
        debug_fd = open("/sys/kernel/debug/tracing/set_ftrace_filter", O_WRONLY);
        if(debug_fd != -1){ ret = write(debug_fd, "pmfs_mount", 10); }
        else{ ret = -3; goto fail; }
        close(debug_fd);

        /* Enable function tracer */
        debug_fd = open("/sys/kernel/debug/tracing/current_tracer", O_WRONLY);
        if(debug_fd != -1){ ret = write(debug_fd, "function", 8); }
        else{ ret = -4; goto fail; }
        close(debug_fd);

        trace_marker = open("/sys/kernel/debug/tracing/trace_marker", O_WRONLY);
        if(trace_marker == -1){ ret = -5; goto fail; }

        debug_fd = open("/sys/kernel/debug/tracing/tracing_on", O_WRONLY);
        if(debug_fd != -1){ ret = write(debug_fd, "1", 1); }
        else{ ret = -5; goto fail; }
        close(debug_fd);

fail:
	if (ret < 0)
		M_WARNING("failed to initialize tracing. need to be root. err = %d.\n",ret);
	#else
        pthread_spin_init(&tot_epoch_lock, PTHREAD_PROCESS_SHARED);
	#endif
		

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
		#ifdef _ENABLE_TRACE
		pthread_spin_lock(&tbuf_lock);
		if(tbuf_sz && mtm_enable_trace)
		{
		        tbuf_ptr = 0; 
        	        while(tbuf_ptr < tbuf_sz)       
	                {
				/* 
				 * for some reason tbuf[tbuf] doesn't work
				 * although i thot tbuf + tbuf is same as 
				 * tbuf[tbuf];
				 */
	                        tbuf_ptr = tbuf_ptr + 1 + fprintf(m_out, "%s", tbuf + tbuf_ptr); 
				/* tbuf_ptr += TSTR_SZ; */
                	}
			tbuf_sz = 0;
		}
		pthread_spin_unlock(&tbuf_lock);
		pthread_spin_destroy(&tbuf_lock);
		#elif _ENABLE_FTRACE
		#else
		pthread_spin_destroy(&tot_epoch_lock);
		#endif
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
