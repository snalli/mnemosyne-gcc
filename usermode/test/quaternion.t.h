// MyTestSuite.h
#include <cxxtest/TestSuite.h>
#include <stdlib.h>
#include <stdio.h>

class MyTestSuite : public CxxTest::TestSuite
{
public:
void testAddition( void )
{
	printf("%s\n", getenv("MTMINI"));
TS_ASSERT( 1 + 1 > 1 );
TS_ASSERT_EQUALS( 1 + 1, 2 );
}
};
