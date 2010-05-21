#include <iostream>
#include <string.h>
#include <pcm.h>
#include <log.h>
#include "../unittest.h"
#include "rawlog_tornbit.helper.h"

int init()
{
	pcm_storeset_t  *pcm_storeset;
    pcm_storeset = pcm_storeset_get ();
    m_rawlog_tornbit_register(pcm_storeset);

	return 0;
}


int main(int argc, char **argv)
{
	extern char  *optarg;
	int          c;
	char         *suiteName;
	char         *testName;

	init();

	getTest(argc, argv, &suiteName, &testName);
	return runTests(suiteName, testName);
}
