/*!
 * \file
 *
 * \author Sanketh Nalli <nalli@wisc.edu> 
 */

#ifndef __PVAR_H__
#define __PVAR_H__

#include "tm.h"


#ifdef __PVAR_C__

#define PVAR(type, var)	\
	MNEMOSYNE_PERSISTENT type var;	\
	TM_SAFE type pset_##var(type __##var) { return (var = __##var); }	\
	TM_SAFE type pget_##var() { return var; }	\
	TM_SAFE unsigned long paddr_##var() { return &var; }	
	
#else /* !__PVAR_C__ */

#define PVAR(type, var)	\
	extern TM_SAFE type pset_##var(type);	\
	extern TM_SAFE type pget_##var();	

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
PVAR(void*, glb_car_table_ptr);
PVAR(void*, glb_room_table_ptr);
PVAR(void*, glb_flight_table_ptr);
PVAR(void*, glb_customer_table_ptr);
PVAR(int, glb_mgr_initialized);


#endif /* __PVAR_H__ */
