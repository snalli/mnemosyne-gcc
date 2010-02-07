#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdint.h>
#include "libpaxos_priv.h"
#include "acceptor_stable_storage.h"
#include "util.h"

#define HASH_OBJECT_MAX_SIZE 10000

static util_hash_t *acceptor_keystore;
static util_pool_t *pool_acceptor_record;

typedef struct {
	size_t size;
	char *data;
} hash_object_t;

//Buffer to read/write current record
static char record_buf[MAX_UDP_MSG_SIZE];
static acceptor_record * record_buffer = (acceptor_record*)record_buf;

//Set to 1 if init should do a recovery
static int do_recovery = 0;

//Invoked before stablestorage_init, sets recovery mode on
// the acceptor will try to recover a DB rather than creating a new one
void stablestorage_do_recovery() {
    printf("Acceptor in recovery mode\n");
    do_recovery = 1;
}


int acceptor_keystore_sanity_check(uint64_t key, void *value)
{
	acceptor_record *rec = (acceptor_record *) value;
	if (key != rec->iid) {
		return 1;
	} 
	return 0;
}

void perform_acceptor_keystore_sanity_check(char *file, int line) 
{
	PRINT_DEBUG("Sanity check: %s:%d \n", file, line);
	util_hash_sanity_check(acceptor_keystore, acceptor_keystore_sanity_check);
	PRINT_DEBUG("Sanity check: %s:%d PASSED\n", file, line);
}

void keystore_print()
{
	util_hash_print(acceptor_keystore);
}

//Initializes the underlying stable storage
int stablestorage_init(int acceptor_id) {
    int ret = 0;
 	   
	if (util_pool_create(&pool_acceptor_record,
	                     HASH_OBJECT_MAX_SIZE,
	                     200000,NULL) 
	    != UTIL_R_SUCCESS) 
	{
		printf("Failed to create acceptor keystore: pool\n");
		abort();
		return -1;
	}

	if (util_hash_create(&acceptor_keystore, 200000, 0) != UTIL_R_SUCCESS) {
		printf("Failed to create acceptor keystore: hash table\n");
		abort();
	}	
    return 0;
}

//Safely closes the underlying stable storage
int stablestorage_shutdown() {
    int result = 0;
    
	util_hash_destroy(&acceptor_keystore);

    LOG(VRB, ("Stable storage shutdown completed\n"));  
    return result;
}



//Retrieves an instance record from stable storage
// returns null if the instance does not exist yet
acceptor_record * 
stablestorage_get_record(iid_t iid) {
    int flags, result;
	acceptor_record *rec;
    
	PRINT_DEBUG("stablestorage_get_record: iid=%llu\n", iid);
    //Read the record
    result = util_hash_lookup(acceptor_keystore, iid, &rec);
    if (result == UTIL_R_NOTEXISTS) {
        //Record does not exist
        LOG(DBG, ("The record for iid:%lu does not exist\n", iid));
		return NULL;
	} else if (result != 0) {
        //Read error!
        printf("Error while reading record for iid:%lu : %s\n", iid);
        return NULL;
    }
    
    //Record found
	PRINT_DEBUG("stablestorage_get_record: rec=%p rec->iid=%llu\n", rec, rec->iid);
    assert(iid == rec->iid);
    return rec;
}

//Save a valid accept request, the instance may be new (no record)
// or old with a smaller ballot, in both cases it creates a new record
acceptor_record * 
stablestorage_save_accept(accept_req * ar) {
    int flags, result;
	acceptor_record *rec;

	PRINT_DEBUG("stablestorage_save_accept: ar=%p, ar->iid=%llu\n", ar, ar->iid);
    //Store as acceptor_record (== accept_ack)
    result = util_hash_lookup(acceptor_keystore, ar->iid, &rec);
    if (result == UTIL_R_NOTEXISTS) {
		if ((rec = util_pool_object_alloc(pool_acceptor_record))
		    == NULL)
		{
			printf("stablestorage_save_accept: failed to allocate object from pool\n");
			abort();
		}
		if (util_hash_add(acceptor_keystore, ar->iid, rec) != UTIL_R_SUCCESS) {
			abort();
		}	
	}	
    rec->iid = ar->iid;
    rec->ballot = ar->ballot;
    rec->value_ballot = ar->ballot;
    rec->is_final = 0;
    rec->value_size = ar->value_size;
    memcpy(rec->value, ar->value, ar->value_size);
    //Store permanently
	PRINT_DEBUG("stablestorage_save_accept: STORE (rec=%p, rec->iid=%llu, ar->iid=%llu)\n", rec, rec->iid, ar->iid);

    return rec;
}


//Save a valid prepare request, the instance may be new (no record)
// or old with a smaller ballot
acceptor_record * 
stablestorage_save_prepare(prepare_req * pr, acceptor_record * rec) {
    int flags, result;

	PRINT_DEBUG("stablestorage_save_prepare: pr=%p, pr->iid=%llu\n", pr, pr->iid);
    //No previous record, create a new one
    if (rec == NULL) {
        //Record does not exist yet
		if ((rec = util_pool_object_alloc(pool_acceptor_record))
		    == NULL)
		{
			printf("stablestorage_save_prepare: failed to allocate object from pool\n");
			abort();
		}
        rec->iid = pr->iid;
        rec->ballot = pr->ballot;
	    rec->value_ballot = 0;
        rec->is_final = 0;
        rec->value_size = 0;
		PRINT_DEBUG("stablestorage_save_prepare: STORE (rec=%p, rec->iid=%llu, pr->iid=%llu)\n", rec, rec->iid, pr->iid);
        util_hash_add(acceptor_keystore, pr->iid, (void *) rec);
    } else {
    //Record exists, just update the ballot
    	rec->ballot = pr->ballot;
    }

	PRINT_DEBUG("stablestorage_save_prepare: DONE (pr=%p, pr->iid=%llu)\n", pr, pr->iid);
    
    return rec;
}

//Save the final value delivered by the underlying learner. 
// The instance may be new or previously seen, in both cases 
// this creates a new record
acceptor_record * 
stablestorage_save_final_value(char * value, size_t size, iid_t iid, ballot_t ballot) {
    int flags, result;
	acceptor_record *rec;
    
	PRINT_DEBUG("stablestorage_save_final_value: iid=%llu\n", iid);
    //Store as acceptor_record (== accept_ack)
    result = util_hash_lookup(acceptor_keystore, iid, &rec);
    if (result == UTIL_R_NOTEXISTS) {
		if ((rec = util_pool_object_alloc(pool_acceptor_record))
		    == NULL)
		{
			printf("stablestorage_save_accept: failed to allocate object from pool\n");
			abort();
		}
		if (util_hash_add(acceptor_keystore, iid, rec) != UTIL_R_SUCCESS) {
			abort();
		}	
	}	

    rec->iid = iid;
    rec->ballot = ballot;
    rec->value_ballot = ballot;
    rec->is_final = 1;
    rec->value_size = size;
    memcpy(rec->value, value, size);
    //Store permanently
    PRINT_DEBUG("stablestorage_save_prepare: STORE (rec=%p, rec->iid=%llu, iid=%llu)\n", rec, rec->iid, iid);

    assert(result == 0);    
    return rec;
}
