#include <vector>
#include <stdlib.h>
#include <unittest++/UnitTest++.h>
#include <mnemosyne.h>
#include <pcm.h>
#include "rawlog_tornbit.helper.h"
#include "sequence.helper.h"


struct fixtureLog {
	fixtureLog() 
	{
		pcm_storeset = pcm_storeset_get();
		m_logmgr_alloc_log(pcm_storeset, LF_TYPE_TM_TORNBIT, 0, &log_dsc);
		rawlog_tornbit = (m_rawlog_tornbit_t *) log_dsc->log;
	}

	~fixtureLog() 
	{
		pcm_storeset_put();
	}

	pcm_storeset_t     *pcm_storeset;
	m_log_dsc_t        *log_dsc;
	m_rawlog_tornbit_t *rawlog_tornbit;
};


SUITE(RandomReadWriteLog) {

	TEST_FIXTURE(fixtureLog, test1) {
		int          length;
		unsigned int length_seed = 0;
		Sequence     *sr = new Sequence(0);


		std::vector<Sequence *> randomSequences;
		for (int i=0; i<14; i++) {
retry:
			length = rand_r(&length_seed) % 16;
			if (length == 0) {
				goto retry;
			}
			randomSequences.push_back(new Sequence(length, i));
			randomSequences[i]->write2log(pcm_storeset, rawlog_tornbit);
		}	
		for (int i=0; i<4; i++) {
			sr->readFromLog(pcm_storeset, rawlog_tornbit);
			CHECK (*sr == *randomSequences[i]);
			sr->print();
		}
	}
}
