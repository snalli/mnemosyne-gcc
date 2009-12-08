#ifndef _MNEMOSYNE_H

# define MNEMOSYNE_PERSISTENT __attribute__ ((section("PERSISTENT")))
# define MNEMOSYNE_ATOMIC

# ifdef __cplusplus
extern "C" {
# endif
	
void *mnemosyne_segment_create(void *start, size_t length, int prot, int flags);
int mnemosyne_segment_destroy(void *start, size_t length);

# ifdef __cplusplus
}
# endif

#endif /* _MNEMOSYNE_H */
