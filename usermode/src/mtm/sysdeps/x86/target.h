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

#ifdef __x86_64__
# define CACHELINE_SIZE 64
#else
# define CACHELINE_SIZE 32
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
