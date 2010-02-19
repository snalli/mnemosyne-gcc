/*!
 * \file
 * Defines constants and data structures matching the target architecture during compilation.
 *
 * The two architectures supported herein are x86 (32-bit) and x86 (64-bit).
 *
 * \author Andres Jaan Tack <tack@cs.wisc.edu>
 */
#ifndef TARGET_H_HBX9MKK6
#define TARGET_H_HBX9MKK6

#include <target.h>
# include <stdint.h>
# include <xmmintrin.h>

typedef struct mtm_jmpbuf_s mtm_jmpbuf_t;

#ifdef __x86_64__
struct mtm_jmpbuf_s
{
	uint64_t spSave;
	uint64_t rbxSave;
	uint64_t rbpSave;
	uint64_t r12Save;
	uint64_t r13Save;
	uint64_t r14Save;
	uint64_t r15Save;
	uint64_t abendPCSave;
	uint32_t mxcsrSave;
	uint32_t txnFlagsSave;      /*<< Transaction flags */
	uint16_t fpcsrSave;
};
#else
struct mtm_jmpbuf_s
{
	uint32_t spSave;
	uint32_t ebxSave;
	uint32_t esiSave;
	uint32_t ediSave;
	uint32_t ebpSave;
	uint32_t abendPCSave;
	uint32_t mxcsrSave;
	uint32_t txnFlagsSave;      /*<< Transaction flags */
	uint16_t fpcsrSave;
};
#endif

/* These are taken from the GCC's TM library */
/* Why not using fence for x86??? */

static inline void
cpu_relax (void)
{
  __asm volatile ("rep; nop" : : : "memory");
}

static inline void
atomic_read_barrier (void)
{
  /* x86 is a strong memory ordering machine.  */
  __asm volatile ("" : : : "memory");
}

static inline void
atomic_write_barrier (void)
{
  /* x86 is a strong memory ordering machine.  */
  __asm volatile ("" : : : "memory");
}


static inline uintptr_t *
get_stack_pointer(void)
{
    uintptr_t *result;

#ifdef __x86_64__
	__asm__ ("movq %%rsp,%0": "=r"(result):);
	return result;
#else
	__asm__ ("movl %%esp,%0": "=r"(result):);
	return result;
#endif	
}


static inline uintptr_t *
get_frame_pointer(void)
{
    uintptr_t *result;

#ifdef __x86_64__
	__asm__ ("movq %%rbp,%0": "=r"(result):);
	return result;
#else
	__asm__ ("movl %%ebp,%0": "=r"(result):);
	return result;
#endif	
}


static uintptr_t *
get_stack_base()
{
	uintptr_t *frame_pointer;
	uintptr_t *stack_base;

	asm volatile ("movq %%rbp,%0": "=r"(frame_pointer):);
	while (frame_pointer) {
		stack_base = frame_pointer;
		if ((uintptr_t*) *frame_pointer <= frame_pointer) {
			break;
		}
		frame_pointer = (uintptr_t*) *frame_pointer;
	}
	return stack_base;
}

#endif /* end of include guard: TARGET_H_HBX9MKK6 */
