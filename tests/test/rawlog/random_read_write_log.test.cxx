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

#include <vector>
#include <stdlib.h>
#include <UnitTest++/UnitTest++.h>
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
