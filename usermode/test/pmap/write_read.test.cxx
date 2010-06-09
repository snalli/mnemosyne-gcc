#include <iostream>
#include <fstream>
#include <mnemosyne.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unittest++/UnitTest++.h>
#include "../common/memrandom.h"

#define PREGION_BASE          0xd00000000
#define PREGION_SIZE          (16*1024*1024)

MNEMOSYNE_PERSISTENT void *pregion;

SUITE(SuiteWriteRead)
{
	
	TEST(Test1)
	{
		pregion = (void *) m_pmap((void *) PREGION_BASE, PREGION_SIZE, PROT_READ|PROT_WRITE, 0);
		writeMemRandom(pregion, PREGION_SIZE, 0x1);
		CHECK(pregion != NULL);
	}

	TEST(Test2)
	{
		CHECK(pregion != NULL);
		assertMemRandom(pregion, PREGION_SIZE, 0x1);
	}
}
