#ifdef __x86_64__
# define CACHELINE_SIZE 64
# define CACHELINE_SIZE_LOG 6
#else
# define CACHELINE_SIZE 32
# define CACHELINE_SIZE_LOG 5
#endif

#define BLOCK_ADDR(addr) ( (pcm_word_t *) ((pcm_word_t) addr & ~(CACHELINE_SIZE - 1)) )
#define INDEX_ADDR(addr) ( (pcm_word_t *) ((pcm_word_t) addr & (CACHELINE_SIZE - 1)) )
