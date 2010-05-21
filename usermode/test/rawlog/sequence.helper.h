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
