#include <iostream>
#include <string.h>
#include "../common/unittest.h"


int main(int argc, char **argv)
{
	extern char  *optarg;
	int          c;
	char         *suiteName;
	char         *testName;

	getTest(argc, argv, &suiteName, &testName);
	return runTests(suiteName, testName);
}
