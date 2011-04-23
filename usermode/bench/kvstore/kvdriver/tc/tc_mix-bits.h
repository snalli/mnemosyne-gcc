/*
    Copyright (C) 2011 Computer Sciences Department, 
    University of Wisconsin -- Madison

    ----------------------------------------------------------------------

    This file is part of Mnemosyne: Lightweight Persistent Memory, 
    originally developed at the University of Wisconsin -- Madison.

    Mnemosyne was originally developed primarily by Haris Volos
    with contributions from Andres Jaan Tack.

    ----------------------------------------------------------------------

    Mnemosyne is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation, version 2
    of the License.
 
    Mnemosyne is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, 
    Boston, MA  02110-1301, USA.

### END HEADER ###
*/

#ifndef _TC_MIX_BITS_H_JA812A
#define _TC_MIX_BITS_H_JA812A

char   *db_home_dir_prefix = "/mnt/pcmfs";


typedef struct tc_mix_global_state_s {
	TCBDB          *bdb;
} tc_mix_global_state_t;


typedef struct object_s {
	int   key;
	char  value[0];
} object_t;


static inline
int 
tc_mix_init_internal(tc_mix_global_state_t *global_state, int use_tc_locks)
{
	TCBDB                  *bdb;
	char                   db_file[128];
	int                    ecode;

	/* open the database */
	bdb = global_state->bdb = tcbdbnew();
	if (use_tc_locks) {
		if(!tcbdbsetmutex(bdb)){
			fprintf(stderr, "Error: tcbdbsetmutex\n");
			return -1;
		}
	}


	sprintf(db_file, "%s/tc.tcb", db_home_dir_prefix);
	if(!tcbdbopen(bdb, db_file, BDBOREADER | BDBOWRITER | BDBOCREAT)){
		ecode = tcbdbecode(bdb);
		fprintf(stderr, "open error: %s\n", tcbdberrmsg(ecode));
		return -1;
	}
	return 0;
}


int tc_mix_fini(void *args)
{
	fini_global();
	return 0;
}


int 
tc_mix_create_elem(int id, void **elemp)
{
	object_t *obj;

	if ((obj = (object_t*) malloc(sizeof(object_t) + vsize)) == NULL) {
		INTERNAL_ERROR("Could not allocate memory.\n")
	}
	obj->key = id;
	*elemp = (void *) obj;

	return 0;
}


int
tc_mix_thread_init(unsigned int tid)
{
	mix_global_state_t     *mix_global_state;
	tc_mix_global_state_t  *global_state;
	mix_thread_state_t     *thread_state;
	object_t               *obj;
	object_t               *iter;
	int                    i;
	int                    j;
	int                    num_keys_per_thread;
	bool                   result;
	TCBDB                  *bdb;
	mix_stat_t             *stat;

	assert(mix_thread_init(tid, NULL, tc_mix_create_elem, &thread_state)==0);

	mix_global_state = (mix_global_state_t *) ubench_desc.global_state;
	global_state = (tc_mix_global_state_t *) mix_global_state->data;
	bdb=global_state->bdb;

	/* Populate the hash table */
	for (i=0; i<thread_state->elemset_ins.count; i++) {
		obj = (object_t *) thread_state->elemset_ins.array[i];
		assert(tcbdbput(bdb, &(obj->key), sizeof(int), &(obj->value), vsize) == true);
	}

	/* sanity check */
	for (i=0; i<thread_state->elemset_ins.count; i++) {
		obj = (object_t *) thread_state->elemset_ins.array[i];
		assert(tcbdbvnum(bdb, &(obj->key), sizeof(int)) > 0);
	}
	for (i=0; i<thread_state->elemset_del.count; i++) {
		obj = (object_t *) thread_state->elemset_del.array[i];
		assert(tcbdbvnum(bdb, &(obj->key), sizeof(int)) == 0);
	}

	return 0;
}


int tc_mix_thread_fini(unsigned int tid)
{
	return 0;
}


static inline
void 
__tc_mix_print_stats(FILE *fout, int report_latency) 
{
	char                    prefix[] = "bench.stat";
	char                    separator[] = "rj,40";
	uint64_t                hashtable_footprint;
	unsigned int            hashtable_num_entries;

	mix_print_stats(fout, report_latency);
}


void tc_mix_latency_print_stats(FILE *fout) 
{
	__tc_mix_print_stats(fout, 1);
}


void tc_mix_throughput_print_stats(FILE *fout) 
{
	__tc_mix_print_stats(fout, 0);
}


#endif /* _TC_MIX_BITS_H_JA812A */
