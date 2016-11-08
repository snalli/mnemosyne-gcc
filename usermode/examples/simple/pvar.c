/*!
 * \file
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
#include <stdio.h>
#include <mnemosyne.h>
#include <mtm.h>

MNEMOSYNE_PERSISTENT int flag;


__attribute__((transaction_safe))
int set_flag(int val) 
{ 
	flag = val;
	return flag; 
}

__attribute__((transaction_safe))
int get_flag(int val) 
{ 
	return flag; 
}

void* addr_flag()
{
	return (void*)&flag;
}
