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
