#ifndef _AUX_H_JALLKA
#define _AUX_H_JALLKA

#include <stdint.h>

#ifndef M_PCM_CPUFREQ
#define M_PCM_CPUFREQ         2500 /* MHz */
#endif

#define CYCLE2NS(__cycles) ((__cycles) * 1000 / M_PCM_CPUFREQ)

typedef uint64_t m_hrtime_t;

static inline void asm_cpuid() {
	asm volatile( "cpuid" :::"rax", "rbx", "rcx", "rdx");
}


static inline unsigned long long asm_rdtsc(void)
{
	unsigned hi, lo;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#define gethrtime asm_rdtsc




#endif
