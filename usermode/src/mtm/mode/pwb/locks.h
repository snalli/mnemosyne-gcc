#ifndef _PWB_LOCKS_H
#define _PWB_LOCKS_H

#undef VERSION_MAX
#undef LOCK_GET_TIMESTAMP
#undef LOCK_SET_TIMESTAMP

#define VERSION_MAX                     (~(mtm_word_t)0 >> 1)

#define LOCK_GET_TIMESTAMP(l)           (l >> 1)         /* Logical shift (unsigned) */
#define LOCK_SET_TIMESTAMP(t)           (t << 1)         /* OWNED bit not set */

#endif /* _PWB_LOCKS_H */
