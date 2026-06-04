/*!
 * \file
 *
 * \author Sanketh Nalli <nalli@wisc.edu> 
 */

#ifndef __PVAR_H__
#define __PVAR_H__

#ifdef __PVAR_C__

#include <memcached.h>

#define PVAR(type, var, init)	\
	MNEMOSYNE_PERSISTENT type var = init;	\
	TM_ATTR type pset_##var(type __##var) 		\
			{ return (var = __##var); }	\
	TM_ATTR type pget_##var() { return var; }	\
	TM_ATTR void* paddr_##var() { return (void*)&var; }	
	
#else /* !__PVAR_C__ */

#define PVAR(type, var, init)	\
	extern MNEMOSYNE_PERSISTENT type var;	\
	extern TM_ATTR type pset_##var(type);	\
	extern TM_ATTR type pget_##var();	\
	extern TM_ATTR void* paddr_##var();	

#endif /* __PVAR_C__ */

#define PSET(var, val)	\
	pset_##var(val)
#define PGET(var)	\
	pget_##var()
#define PADDR(var)	\
	paddr_##var()

/* 
 * MENTION PERSISTENT VARIABLES HERE 
 * PVAR(type, variable_name)
 */
PVAR(void*, p_primary_hashtbl, NULL);


#endif /* __PVAR_H__ */
