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

#include <iostream>
#include <fstream>
#include <getopt.h>
#include <string.h>
#include <UnitTest++/UnitTest++.h>
#include <UnitTest++/TestReporterStdout.h>
#include <UnitTest++/TestRunner.h>
#include <UnitTest++/Test.h>
#include <UnitTest++/XmlTestReporter.h>

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


static inline int runTests(const char *suiteName, const char *testName)
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


static inline int getTest(int argc, char **argv, char **suiteName, char **testName)
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
