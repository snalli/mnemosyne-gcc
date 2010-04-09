#include <vector>
#include <stdlib.h>
#include <unittest++/UnitTest++.h>
#include <phlog.h>
#include "sequence.helpers.h"



SUITE(RandomReadWriteLog) {

	TEST(test1) {
		int          length;
		unsigned int length_seed = 0;
		Sequence     *sr = new Sequence(0);
		m_phlog_t    log;
		m_nv_phlog_t nv_log;

		m_phlog_init_nv(&nv_log);
		m_phlog_init(&log, &nv_log);

		std::vector<Sequence *> randomSequences;
		for (int i=0; i<14; i++) {
retry:
			length = rand_r(&length_seed) % 16;
			if (length == 0) {
				goto retry;
			}
			randomSequences.push_back(new Sequence(length, i));
			randomSequences[i]->write2log(&log);
		}	
		for (int i=0; i<4; i++) {
			sr->readFromLog(&log);
			CHECK (*sr == *randomSequences[i]);
			sr->print();
		}
	}

}
