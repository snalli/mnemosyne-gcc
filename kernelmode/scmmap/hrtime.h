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

/*
 * \file
 *
 * \brief Interface to x86/64's high resolution time counter.
 *
 */

#ifndef _HRTIME_H_121AJ1
#define _HRTIME_H_121AJ1

#ifndef _HRTIME_CPUFREQ
# define _HRTIME_CPUFREQ 2500 /* GHz */
#endif

#define HRTIME_NS2CYCLE(__ns) ((__ns) * _HRTIME_CPUFREQ / 1000)
#define HRTIME_CYCLE2NS(__cycles) ((__cycles) * 1000 / _HRTIME_CPUFREQ)


typedef unsigned long long hrtime_t;

static inline void hrtime_barrier() {
	asm volatile( "cpuid" :::"rax", "rbx", "rcx", "rdx");
}

#if defined(__i386__)

static inline unsigned long long hrtime_cycles(void)
{
	unsigned long long int x;
	__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
	return x;
}

#elif defined(__x86_64__)

static inline unsigned long long hrtime_cycles(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#else
#error "What architecture is this???"
#endif


#endif /* _HRTIME_H_121AJ1 */
