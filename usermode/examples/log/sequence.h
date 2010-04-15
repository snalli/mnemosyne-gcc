#ifndef _SEQUENCE_H
#define _SEQUENCE_H

#include <iostream>
#include <vector>
#include <stdlib.h>
#include <assert.h>


class Sequence {
public:
	Sequence(int len, int seed);
	Sequence(int len);
	int write2log(m_log_dsc_t *log);
	int readFromLog(m_log_dsc_t *log);
	bool operator==(Sequence &other) const;
	bool operator!=(Sequence &other) const;
	int size();
	void print();
private:
	int                      _len;
	std::vector<scm_word_t>  _words;
	unsigned int             _seed;
	int                      _sqn;
};


Sequence::Sequence(int len, int sqn): _len(len), _seed(sqn), _sqn(sqn)
{
	int        i;
	scm_word_t value;

	for (i=0; i<len; i++) {
retry:
		value = ((scm_word_t) rand_r(&_seed) | ( ((scm_word_t) rand_r(&_seed)) << 32));
		if (value == XACT_COMMIT_MARKER) {
			goto retry;
		}
		_words.push_back(value);
	}
}

Sequence::Sequence(int len): _len(len)
{
	
}


int Sequence::write2log(m_log_dsc_t *log_dsc)
{
	int               i;
	m_tmlog_tornbit_t *tmlog;

	tmlog = (m_tmlog_tornbit_t *) log_dsc->log;
	
	for (i=0; i<_words.size(); i+=3) {
		if (m_tmlog_tornbit_write(tmlog, _words[i], _words[i+1], _words[i+2]) 
		    != M_R_SUCCESS) 
		{
			assert(0);
		}
	}
	m_tmlog_tornbit_commit(tmlog, _sqn);
	return 0;
}

int Sequence::readFromLog(m_log_dsc_t *log)
{
	int        i;
	scm_word_t value;
#if 0
	_words.clear();
	while(1) {
		if (m__read(log, &value) != M_R_SUCCESS) {
			assert(0);
		}
		if (value == SEQUENCE_END) {
			m_phlog_next_chunk(log);
			break;
		} else {
			_words.push_back(value);
		}
	}
#endif	
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
