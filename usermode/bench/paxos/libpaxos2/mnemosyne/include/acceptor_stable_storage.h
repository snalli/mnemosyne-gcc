#ifndef ACCEPTOR_STABLE_STORAGE_H_C2XN5QX9
#define ACCEPTOR_STABLE_STORAGE_H_C2XN5QX9

#include "tm.h"

int stablestorage_init(int acceptor_id);
void stablestorage_do_recovery();
int stablestorage_shutdown();


TM_CALLABLE acceptor_record * stablestorage_get_record(iid_t iid);

TM_CALLABLE acceptor_record * stablestorage_save_accept(accept_req * ar);
TM_CALLABLE acceptor_record * stablestorage_save_prepare(prepare_req * pr, acceptor_record * rec);

TM_CALLABLE acceptor_record * stablestorage_save_final_value(char * value, size_t size, iid_t iid, ballot_t ballot);

TM_CALLABLE void perform_acceptor_keystore_sanity_check(char *file, int line);

#ifdef DEBUG
# define PERFORM_ACCEPTOR_KEYSTORE_SANITY_CHECK perform_acceptor_keystore_sanity_check(__FILE__, __LINE__);
#else
# define PERFORM_ACCEPTOR_KEYSTORE_SANITY_CHECK 
#endif

#endif /* end of include guard: ACCEPTOR_STABLE_STORAGE_H_C2XN5QX9 */
