/*!
 * \file
 *
 * \author Haris Volos <hvolos@cs.wisc.edu>
 */
#include <stdio.h>
#include <mnemosyne.h>
#include <mtm.h>

extern MNEMOSYNE_PERSISTENT int flag;

extern
__attribute__((transaction_safe))
int set_flag(int val);

extern
__attribute__((transaction_safe))
int get_flag(); 
