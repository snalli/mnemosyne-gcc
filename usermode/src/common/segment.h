#ifndef _MNEMOSYNE_SPRING_H
#define _MNEMOSYNE_SPRING_H

#include <stdio.h>
#include <common/result.h>

mnemosyne_result_t mnemosyne_segment_reincarnate_address_space();
mnemosyne_result_t mnemosyne_segment_checkpoint_address_space();

void *mnemosyne_segment_create(void *start, size_t length, int prot, int flags);

#endif /* _MNEMOSYNE_SPRING_H */
