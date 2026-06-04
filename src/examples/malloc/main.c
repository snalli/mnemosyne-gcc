/*!
 * \file
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 * \author Sanketh Nalli <nalli@wisc.edu>
 */
#include "pvar.h"
#include <stdio.h>
#include <stdlib.h>
#include <pmalloc.h>
#include <pthread.h>

void* ptr;

int main (int argc, char const *argv[])
{

	__transaction_atomic {
        ptr = pmalloc(1024);
        //__transaction_cancel;
	}

    __transaction_atomic {
        pfree(ptr);
	}
	
	return 0;
}
