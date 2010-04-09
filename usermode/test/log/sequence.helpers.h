#ifndef _SEQUENCE_H
#define _SEQUENCE_H

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <assert.h>
#include <phlog.h>

#define SEQUENCE_END 0xDEADBEEFDEADBEEF

class Sequence {
public:
	Sequence(int len, int seed);
	Sequence(int len);
	int write2log(m_phlog_t *log);
	int readFromLog(m_phlog_t *log);
	bool operator==(Sequence &other) const;
	bool operator!=(Sequence &other) const;
	int size();
	void print();
private:
	int                      _len;
	std::vector<scm_word_t>  _words;
	unsigned int             _seed;
};


Sequence::Sequence(int len, int seed): _len(len)
{
	int        i;
	scm_word_t value;

	_seed = seed;

	for (i=0; i<len; i++) {
retry:
		value = ((scm_word_t) rand_r(&_seed) | ( ((scm_word_t) rand_r(&_seed)) << 32));
		if (value == SEQUENCE_END) {
			goto retry;
		}
		_words.push_back(value);
	}
}

Sequence::Sequence(int len): _len(len)
{
	
}


int Sequence::write2log(m_phlog_t *log)
{
	int i;

	
	for (i=0; i<_words.size(); i++) {
		if (m_phlog_write(log, _words[i]) != M_R_SUCCESS) {
			assert(0);
		}
	}
	m_phlog_write(log, SEQUENCE_END);
	m_phlog_flush(log);
}

int Sequence::readFromLog(m_phlog_t *log)
{
	int        i;
	scm_word_t value;

	_words.clear();
	while(1) {
		if (m_phlog_read(log, &value) != M_R_SUCCESS) {
			assert(0);
		}
		if (value == SEQUENCE_END) {
			m_phlog_next_chunk(log);
			break;
		} else {
			_words.push_back(value);
		}
	}
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
