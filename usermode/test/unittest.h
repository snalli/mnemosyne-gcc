#include <fstream>
#include <getopt.h>
#include <unittest++/UnitTest++.h>
#include <unittest++/TestReporterStdout.h>
#include <unittest++/TestRunner.h>
#include <unittest++/Test.h>
#include <unittest++/XmlTestReporter.h>

struct RunTestIfNameIs
{
	RunTestIfNameIs(char const* name_)
	: name(name_)
	{
	}

	bool operator()(const UnitTest::Test* const test) const
	{
		using namespace std;
		return (0 == strcmp(test->m_details.testName, name));
	}

	char const* name;
};


int runTests(const char *suiteName, const char *testName)
{
	int                          ret;
	UnitTest::XmlTestReporter    reporter(std::cerr);
	UnitTest::TestRunner         runner(reporter);

	if (testName) {
		RunTestIfNameIs predicate(testName);
		return runner.RunTestsIf(UnitTest::Test::GetTestList(), suiteName, predicate, 0);
	} else {
		return runner.RunTestsIf(UnitTest::Test::GetTestList(), suiteName, UnitTest::True(), 0);
	}
}


int getTest(int argc, char **argv, char **suiteName, char **testName)
{
	extern char  *optarg;
	int          c;
	char         *_suiteName = NULL;
	char         *_testName = NULL;

	while (1) {
		static struct option long_options[] = {
			{"suite",  required_argument, 0, 's'},
			{"test", required_argument, 0, 't'},
			{0, 0, 0, 0}
		};
		int option_index = 0;
     
		c = getopt_long (argc, argv, "s:t:",
		                 long_options, &option_index);
     
		/* Detect the end of the options. */
		if (c == -1)
			break;
     
		switch (c) {
			case 's':
				_suiteName = optarg;
				break;
			case 't':
				_testName = optarg;
				break;
		}
	}

	*suiteName = _suiteName;
	*testName = _testName;

	if (_suiteName && _testName) {
		return 0;
	} else {
		return 1;
	}
		
}
