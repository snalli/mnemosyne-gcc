#ifndef _PCM_H
#define _PCM_H

#include <target.h>
#include <stdint.h>

typedef uintptr_t pcm_word_t;

typedef struct pcm_storeset_s pcm_storeset_t;

int pcm_storeset_create(pcm_storeset_t **setp);
void pcm_storeset_destroy(pcm_storeset_t *set);

void pcm_wb_store(pcm_storeset_t *set, volatile pcm_word_t *addr, pcm_word_t val);
void pcm_wb_flush(pcm_storeset_t *set, pcm_word_t *addr);

#endif
