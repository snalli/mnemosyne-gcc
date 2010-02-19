#ifndef _WT_LOCKS_H
#define _WT_LOCKS_H

#undef INCARNATION_BITS
#undef INCARNATION_MAX
#undef INCARNATION_MASK
#undef VERSION_MAX
#undef LOCK_GET_TIMESTAMP
#undef LOCK_SET_TIMESTAMP
#undef LOCK_GET_INCARNATION
#undef LOCK_SET_INCARNATION
#undef LOCK_UPD_INCARNATION

#define INCARNATION_BITS               3                   /* 3 bits */
#define INCARNATION_MAX                ((1 << INCARNATION_BITS) - 1)
#define INCARNATION_MASK               (INCARNATION_MAX << 1)
#define VERSION_MAX                    (~(mtm_word_t)0 >> (1 + INCARNATION_BITS))

# define LOCK_GET_TIMESTAMP(l)          (l >> (1 + INCARNATION_BITS))
# define LOCK_SET_TIMESTAMP(t)          (t << (1 + INCARNATION_BITS))
# define LOCK_GET_INCARNATION(l)        ((l & INCARNATION_MASK) >> 1)
# define LOCK_SET_INCARNATION(i)        (i << 1)            /* OWNED bit not set */
# define LOCK_UPD_INCARNATION(l, i)     ((l & ~(mtm_word_t)(INCARNATION_MASK | OWNED_MASK)) | LOCK_SET_INCARNATION(i))

#endif /* _WT_LOCKS_H */
