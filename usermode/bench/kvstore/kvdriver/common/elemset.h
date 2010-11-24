#ifndef _ELEMSET_H_9KI112
#define _ELEMSET_H_9KI112

#include <stdlib.h>


typedef struct elemset_s {
	void         **array;
	int          size;
	int          count;
	unsigned int seed;
} elemset_t;


static int 
elemset_init(elemset_t *elemset, int size, unsigned int seed)
{
	int ret = 0; 

	if (!(elemset->array = calloc(size, sizeof(void *)))) {
		ret = -1;
		goto err;
	}
	elemset->size = size;
	elemset->count = 0;
	elemset->seed = seed;
	return 0;
	
err:
	return ret;
}


static int 
elemset_put(elemset_t *elemset, void *obj)
{
	int ret = 0;

	if (elemset->count >= elemset->size) {
		ret = -1;
		goto err;
	}
	elemset->array[elemset->count++] = obj;

err:
	return ret;
}


static int 
elemset_get_random(elemset_t *elemset, void **objp)
{
	int  ret = 0;
	int  index;
	void *obj;

	if (elemset->count == 0) {
		ret = -1;
		goto err;
	}

	index = rand_r(&elemset->seed) % elemset->count;
	obj = elemset->array[index];
	elemset->array[index] = elemset->array[elemset->count-1];
	elemset->count--;
	*objp = obj;

	return 0;
err:
	return ret;
}

static
void
elemset_print(elemset_t *elemset)
{
	int i;

	printf("[");
	for (i=0; i<elemset->count; i++) {
		/* TODO */
	}
	printf("]\n");
}
#endif
