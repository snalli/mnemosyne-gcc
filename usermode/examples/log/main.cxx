#include <sys/mman.h>
#include <iostream>
#include <vector>
#include <mnemosyne.h>
#include "../../library/mtm/src/mode/pwb/tmlog_tornbit.h"
#include "sequence.h"

void test(m_log_dsc_t *log_dsc)
{
	int          length;
	unsigned int length_seed = 0;


		std::vector<Sequence *> randomSequences;
		for (int i=0; i<122; i++) {
retry:
			length = 3 * (rand_r(&length_seed) % 25); /* length is multiple of 3 */
			if (length == 0) {
				goto retry;
			}
			std::cout << "length = " << length << std::endl;
			randomSequences.push_back(new Sequence(length, i));
			randomSequences[i]->write2log(log_dsc);
		}	
		/*
		for (int i=0; i<4; i++) {
			sr->readFromLog(log_dsc);
			CHECK (*sr == *randomSequences[i]);
			sr->print();
		}
		*/
}


int main (int argc, char const *argv[])
{
	m_log_dsc_t       *log_dsc;
	m_tmlog_tornbit_t *tmlog;
	m_logmgr_register_logtype(LF_TYPE_TM_TORNBIT, &tmlog_tornbit_ops);
	m_logmgr_do_recovery();

	m_logmgr_alloc_log(LF_TYPE_TM_TORNBIT, &log_dsc);

	tmlog = (m_tmlog_tornbit_t *) log_dsc->log;

	test(log_dsc);
}
