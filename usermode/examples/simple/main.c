/*!
 * \file
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
#include <stdio.h>
#include <mnemosyne.h>
#include <mtm.h>

MNEMOSYNE_PERSISTENT int flag = 0;


int main (int argc, char const *argv[])
{
	printf("flag: %d", flag);
	MNEMOSYNE_ATOMIC {
		if (flag == 0) {
			flag = 1;
		} else {
			flag = 0;
		}
	}
	
	printf(" --> %d\n&flag = %p\n", flag, &flag);
	return 0;
}
