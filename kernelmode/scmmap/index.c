/**
 * \file index.c
 *
 * \brief The index that keeps mappings of SCM page frames to file offsets.
 *
 */

#include <linux/module.h> 
#include <linux/kernel.h>

#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>
#include <linux/exportfs.h>

#include <asm/uaccess.h>

#include "scmmap.h"
#include "scmmap-userkernel.h"
#include "index.h"

static struct kmem_cache      *scmpage_mapping_cachep;
struct scmpage_mapping_index  *scmpage_mapping_index;



int scmmap_index_create(int mappings_max_n)
{
	int rv = 0;
	int i;

	scmpage_mapping_cachep = KMEM_CACHE(scmpage_mapping, 0);
	if (scmpage_mapping_cachep == NULL) {
		rv = -ENOMEM;
		goto out;
	}
	scmpage_mapping_index = vmalloc(sizeof(struct scmpage_mapping_index));
	if (scmpage_mapping_index == NULL) {
		rv = -ENOMEM;
		goto no_index;
	}
	scmpage_mapping_index->array = vmalloc(sizeof(struct scmpage_mapping)*mappings_max_n);
	if (scmpage_mapping_index->array == NULL) {
		rv = -ENOMEM;
		goto no_index_array;
	}
	scmpage_mapping_index->mappings_count = 0;
	scmpage_mapping_index->mappings_max_n = mappings_max_n;
	for (i=0; i<mappings_max_n; i++) {
		scmpage_mapping_index->array[i] = NULL;
	}

	printk(KERN_INFO "%s initialized cache\n", MODULE_NAME);
	goto out;

no_index_array:
	vfree(scmpage_mapping_index);
no_index:
	kmem_cache_destroy(scmpage_mapping_cachep);
out:
	return rv;
}


int scmmap_index_destroy(void)
{
	int                    rv = 0;
	int                    i;
	struct scmpage_mapping *mapping;

	for (i=0; i<scmpage_mapping_index->mappings_max_n; i++) {
		if ((mapping = scmpage_mapping_index->array[i])) {
			kmem_cache_free(scmpage_mapping_cachep, mapping);
		}
	}
	kmem_cache_destroy(scmpage_mapping_cachep);
	vfree(scmpage_mapping_index->array);
	vfree(scmpage_mapping_index);

	return rv;
}


int scmmap_index_alloc_mapping(struct scmpage_mapping **mappingp)	
{
	unsigned long          i;
	struct scmpage_mapping *mapping;

	if (scmpage_mapping_index->mappings_count >= scmpage_mapping_index->mappings_max_n) {
		*mappingp = NULL;
		return -ENOMEM;
	}
	mapping = (struct scmpage_mapping *) kmem_cache_alloc(scmpage_mapping_cachep, GFP_KERNEL);
	if (mapping) {
		i = scmpage_mapping_index->mappings_count++;
		scmpage_mapping_index->array[i] = mapping;
		*mappingp = mapping;
		return 0;
	} else {
		*mappingp = NULL;
		return -ENOMEM;
	}
	/* unreachable -- but keep the return 0 so that the compiler does not complain */
	return 0;
}


int scmmap_index_get_mapping(unsigned long index, struct scmpage_mapping **mapping)
{
	if (index >= scmpage_mapping_index->mappings_max_n) {
		*mapping = NULL;
		return -EFAULT;
	}
	*mapping = scmpage_mapping_index->array[index];
	return 0;
}
