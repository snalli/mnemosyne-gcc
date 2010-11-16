#ifndef _KEYSET_H_9KI112
#define _KEYSET_H_9KI112

#include <stdlib.h>

typedef struct keyset_s {
	int          *array;
	int          size;
	int          count;
	unsigned int seed;
} keyset_t;


static int 
keyset_init(keyset_t *keyset, int size, unsigned int seed)
{
	int ret = 0; 

	if (!(keyset->array = calloc(size, sizeof(int)))) {
		ret = -1;
		goto err;
	}
	keyset->size = size;
	keyset->count = 0;
	keyset->seed = seed;
	return 0;
	
err:
	return ret;
}


static int
keyset_fill(keyset_t *keyset, int keys_range_min, int keys_range_max)
{
	int ret = 0;
	int i;
	int j;
	int keys_range_len = keys_range_max - keys_range_min + 1;

	if (keys_range_len + keyset->count > keyset->size) {
		ret = -1;
		goto err;
	}

	/* check for duplicates */
	for (i=0; i<keyset->count; i++) {
		if (keyset->array[i] >= keys_range_min && 
		    keyset->array[i] <= keys_range_max) 
		{
			ret = -1;
			goto err;
		}
	}

	j = keyset->count;
	for (i=keys_range_min; i<=keys_range_max; i++, j++) {
		keyset->array[j] = i;
	}
	keyset->count = j;

	return 0;

err:
	return ret;
}


static int 
keyset_put_key(keyset_t *keyset, int key)
{
	int ret = 0;

	if (keyset->count >= keyset->size) {
		ret = -1;
		goto err;
	}
	keyset->array[keyset->count++] = key;

err:
	return ret;
}


static int 
keyset_get_key_random(keyset_t *keyset, int *key)
{
	int ret = 0;
	int index;
	int key_local;

	if (keyset->count == 0) {
		ret = -1;
		goto err;
	}
	index = rand_r(&keyset->seed) % keyset->count;
	key_local = keyset->array[index];
	keyset->array[index] = keyset->array[keyset->count-1];
	keyset->count--;
	*key = key_local;

	return 0;
err:
	return ret;
}


static void 
keyset_print(keyset_t *keyset)
{
	int i;

	printf("[");
	for (i=0; i<keyset->count; i++) {
		printf("%d, ", keyset->array[i]);
	}
	printf("]\n");
}
#endif
