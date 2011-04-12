/**
 * \file
 * Defines constants and data structures matching the target architecture during compilation.
 *
 * The two architectures supported herein are x86 (32-bit) and x86 (64-bit).
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */

#ifndef TARGET_H_HBX9MKK6
#define TARGET_H_HBX9MKK6

# include <stdint.h>
# include <xmmintrin.h>

#ifdef __x86_64__
# define CACHELINE_SIZE 64
# define CACHELINE_SIZE_LOG 6
#else
# define CACHELINE_SIZE 32
# define CACHELINE_SIZE_LOG 5
#endif

typedef struct mtm_jmpbuf_s mtm_jmpbuf_t;

#ifdef __x86_64__
struct mtm_jmpbuf_s
{
	uint64_t sp;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t r12;
	uint64_t r13;
	uint64_t r14;
	uint64_t r15;
	uint64_t abendPC;
	uint32_t mxcsr;
	uint32_t txn_flags;
	uint16_t fpcsr;
};
#else
struct mtm_jmpbuf_s
{
	uint32_t sp;
	uint32_t ebx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
	uint32_t abendPC;
	uint32_t mxcsr;
	uint32_t txn_flags;
	uint16_t fpcsr;
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

/* This requires frame pointers being enabled */
static inline uintptr_t
get_stack_base()
{
	uintptr_t frame_pointer;
	uintptr_t stack_base;

	asm volatile ("movq %%rbp,%0": "=r"(frame_pointer):);
	while (frame_pointer) {
		stack_base = frame_pointer;
		if (*((uintptr_t*) frame_pointer) <= frame_pointer) {
			break;
		}
		frame_pointer = *((uintptr_t*) frame_pointer);
	}
	return stack_base;
}

#endif /* TARGET_H_HBX9MKK6 */
