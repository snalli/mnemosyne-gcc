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
#include <mnemosyne.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <UnitTest++/UnitTest++.h>
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
