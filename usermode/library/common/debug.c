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

#include <execinfo.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include "debug.h"

#define PSEGMENT_RESERVED_REGION_START   0x0000100000000000
#define PSEGMENT_RESERVED_REGION_SIZE    0x0000010000000000 /* 1 TB */
#define PSEGMENT_RESERVED_REGION_END     (PSEGMENT_RESERVED_REGION_START +    \
                                          PSEGMENT_RESERVED_REGION_SIZE)

/* Tracing infrastructure */
__thread struct timeval mtm_time;
__thread int mtm_tid = -1;

__thread char tstr[TSTR_SZ];
__thread int tsz = 0;
__thread unsigned long long tbuf_ptr = 0;
__thread int reg_write = 0;
__thread unsigned long long n_epoch = 0;

/* Can we make these thread local ? */
char *tbuf;
pthread_spinlock_t tbuf_lock;
pthread_spinlock_t tot_epoch_lock;
unsigned long long tbuf_sz;
int mtm_enable_trace = 0;
int mtm_debug_buffer = 1;
int trace_marker = -1, tracing_on = -1;
struct timeval glb_time;
unsigned long long start_buf_drain = 0, end_buf_drain = 0, buf_drain_period = 0;
unsigned long long glb_tv_sec = 0, glb_tv_usec = 0, glb_start_time = 0;
unsigned long long tot_epoch = 0;

/*
 * Count the number of epochs on a thread. Tracing must be turned
 * off for this feature, as it slows down execution thus reducing
 * the rate of epochs. The strcmp compares at most 4 bytes as the
 * longest marker is that long, hence adding minimal overhead.
 *
 * If a write is encountered, we register it by setting a flag.
 * If a fence is encountered, we reset the flag and increment
 * the number of epochs on the thread. These operations are
 * thread specific and operate on thread-local storage. We do
 * not instrument reads to PM.
 *
 */
#include <pm_instr.h>
void __pm_trace_print(char* format, ...)
{
	va_list __va_list;
	va_start(__va_list, format);
	va_arg(__va_list, int); /* ignore first arg */
        char* marker = va_arg(__va_list, char*);
	unsigned long long addr = 0;

        if(!strcmp(marker, PM_FENCE_MARKER) ||
		!strcmp(marker, PM_TX_END)) {
		/*
		 * Applications are notorious for issuing
		 * fences, even when they didn't write to 
		 * PM. For eg., a fence for making a store
		 * to local, volatile variable visible.
		 */
		if(reg_write) {
        	        n_epoch += 1;

			pthread_spin_lock(&tot_epoch_lock);
			tot_epoch += 1;
			pthread_spin_unlock(&tot_epoch_lock);
		}
	        reg_write = 0;

        } else if(!strcmp(marker, PM_WRT_MARKER) ||
                !strcmp(marker, PM_DWRT_MARKER) ||
                !strcmp(marker, PM_DI_MARKER) ||
                !strcmp(marker, PM_NTI)) {
		addr = va_arg(__va_list, unsigned long long);

		if((PSEGMENT_RESERVED_REGION_START < addr &&
			addr < PSEGMENT_RESERVED_REGION_END))
                	reg_write = 1;
        } else;
	va_end(__va_list);
}

unsigned long long get_epoch_count(void)
{
	return n_epoch;
}

unsigned long long get_tot_epoch_count(void)
{
	return tot_epoch;
}

void
m_debug_print(char *file, int line, int fatal, const char *prefix,
              const char *strformat, ...) 
{
	char    buf[512];
	va_list ap;
	int     len;
	if (prefix) {
		len = sprintf(buf, "%s ", prefix); 
	}
	if (file) {
		len += sprintf(&buf[len], "%s(%d)", file, line); 
	}
	if (file || prefix) {
		len += sprintf(&buf[len], ": "); 
	}
	va_start (ap, strformat);
	vsnprintf(&buf[len], sizeof (buf) - 1 - len, strformat, ap);
	va_end (ap);
	fprintf(M_DEBUG_OUT, "%s", buf);
	if (fatal) {
		exit(1);
	}
}


void 
m_debug_print_L(int debug_flag, const char *strformat, ...) 
{
	char           msg[512];
	int            len; 
	va_list        ap;
	struct timeval curtime;
	int            xact_state;
	unsigned int   tid;
	unsigned int   tid_pthread;
	
	if (!debug_flag) {
		return;
	} 

	//TODO: initialize with the right values
	//xact_state = mnemosyne_tx_get_xactstate(mnemosyne_l_txd);
	//tid = mnemosyne_thrdesc_get_tid(mnemosyne_l_txd); 
	//tid_pthread = mnemosyne_tx_get_tid_pthread(mnemosyne_l_txd); 
	xact_state = 0;
	tid = 0;
	tid_pthread = 0;
	gettimeofday(&curtime, NULL); 
	len = sprintf(msg, "[M_DEBUG: T-%02u (%u) TS=%04u%06u TX=%d PC=%p] ", 
	              tid, 
	              tid_pthread, 
	              (unsigned int) curtime.tv_sec, (unsigned int) curtime.tv_usec,
	              xact_state,
	              __builtin_return_address(0)); 
	va_start (ap, strformat);
	vsnprintf(&msg[len], sizeof (msg) - 1 - len, strformat, ap);
	va_end (ap);

	len = strlen(msg);
	msg[len] = '\0';

	fprintf(M_DEBUG_OUT, "%s", msg);
}


/* Obtain a backtrace and print it to stdout. */
void 
m_print_trace (void)
{
       void *array[10];
       int size;
       char **strings;
       size_t i;
     
       size = backtrace (array, 10);
       strings = backtrace_symbols (array, size);
     
       printf ("Obtained %d stack frames.\n", size);
     
       for (i = 0; i < size; i++)
          printf ("%s\n", strings[i]);
     
       free (strings);
}
