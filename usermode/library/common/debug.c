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
#include "debug.h"

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
