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

#ifndef _SEQUENCE_H
#define _SEQUENCE_H

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <assert.h>
#include "rawlog_tornbit.helper.h"

#define SEQUENCE_END 0xDEADBEEFDEADBEEF

typedef m_rawlog_tornbit_t m_rawlog_t;

#define PHLOG_T      m_phlog_tornbit_t
#define PHLOG_PREFIX phlog
#define RAWLOG_WRITE m_rawlog_tornbit_write
#define RAWLOG_FLUSH m_rawlog_tornbit_flush

class Sequence {
public:
	Sequence(int len, int seed);
	Sequence(int len);
	int  write2log(pcm_storeset_t *set, m_rawlog_t *log);
	int  readFromLog(pcm_storeset_t *set, m_rawlog_t *log);
	bool operator==(Sequence &other) const;
	bool operator!=(Sequence &other) const;
	int  size();
	void print();
private:
	int                      _len;
	std::vector<pcm_word_t>  _words;
	unsigned int             _seed;
};


Sequence::Sequence(int len, int seed): _len(len)
{
	int        i;
	pcm_word_t value;

	_seed = seed;

	for (i=0; i<len; i++) {
retry:
		value = ((pcm_word_t) rand_r(&_seed) | ( ((pcm_word_t) rand_r(&_seed)) << 32));
		if (value == SEQUENCE_END) {
			goto retry;
		}
		_words.push_back(value);
	}
}

Sequence::Sequence(int len): _len(len)
{
	
}


int Sequence::write2log(pcm_storeset_t *set, m_rawlog_t *rawlog)
{
	int i;

	for (i=0; i<_words.size(); i++) {
		if (RAWLOG_WRITE(set, rawlog, _words[i]) != M_R_SUCCESS) {
			CHECK(false);
		}
	}
	RAWLOG_WRITE(set, rawlog, SEQUENCE_END);
	RAWLOG_FLUSH(set, rawlog);

	return 0;
}


int Sequence::readFromLog(pcm_storeset_t *set, m_rawlog_t *rawlog)
{
	int        i;
	pcm_word_t value;

	_words.clear();
	while(1) {
		if (m_phlog_tornbit_read(&(rawlog->phlog_tornbit), &value) != M_R_SUCCESS) {
			assert(0);
		}
		if (value == SEQUENCE_END) {
			m_phlog_tornbit_next_chunk(&rawlog->phlog_tornbit);
			break;
		} else {
			_words.push_back(value);
		}
	}

	return 0;
}


void Sequence::print()
{
	int i;

	for (i=0; i<_words.size(); i++) {
		std::cout << std::hex << _words[i] << std::endl;
	}
}

int Sequence::size()
{
	return _words.size();
}


bool Sequence::operator==(Sequence &other) const
{
	if (other.size() != _words.size()) {
		return false;
	}
	for (int i=0; i<_words.size(); i++) {
		if (_words[i] != other._words[i]) {
			return false;
		}
	}
	return true;
}

bool Sequence::operator!=(Sequence &other) const
{
	return (!(*this == other));
}

#endif /* _SEQUENCE_H */
