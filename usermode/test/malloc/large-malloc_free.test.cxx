#include <malloc.h>
#include <mnemosyne.h>
#include "../common/unittest.h"

MNEMOSYNE_PERSISTENT void *dummy_ptr1;

SUITE(SuiteLarge)
{
	TEST(Test1)
	{
		__tm_atomic {
			dummy_ptr1 = pmalloc();
		}
		CHECK(dummy_ptr1 != NULL);
	}

	TEST(Test2)
	{
		__tm_atomic {
			pfree(dummy_ptr1);
		}
		CHECK(true);
	}
}
