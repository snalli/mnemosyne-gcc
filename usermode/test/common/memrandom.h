#ifndef _MEMRANDOM_H
#define _MEMRANDOM_H

static inline void writeMemRandom(void *ptr, size_t sz, unsigned int seed)
{
	size_t       i;
	uintptr_t    *uptr;
	uintptr_t   val;
	unsigned int _seed = seed;

	for (i=0; i<sz; i+=sizeof(uintptr_t)) {
		val = rand_r(&_seed);
		uptr = (uintptr_t *) ((size_t) ptr + i);
		*uptr = val;
	}
}

static inline void assertMemRandom(void *ptr, size_t sz, unsigned int seed)
{
	size_t       i;
	uintptr_t    *uptr;
	uintptr_t   correct_val;
	uintptr_t   read_val;
	unsigned int _seed = seed;

	for (i=0; i<sz; i+=sizeof(uintptr_t)) {
		correct_val = rand_r(&_seed);
		uptr = (uintptr_t *) ((size_t) ptr + i);
		read_val = *uptr;
		CHECK(read_val == correct_val);
	}
}

#endif /* _MEMRANDOM_H */
