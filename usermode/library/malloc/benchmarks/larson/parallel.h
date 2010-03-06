#ifndef _SPIN_LOCKS_H_
#define _SPIN_LOCKS_H_

/* File: parallel.h
 *
 * This package was originally used at the University of Waterloo to 
 * replace the inline "asm" macros for spin locks and barriers provided 
 * by the Sequent C compiler. The UW version relied on Gnu C and Gnu
 * assembler (gas). Subsequently modified to work with Microsoft C
 * and MASM.
 *
 * Author: Paul Larson, palarson@microsoft.com, July 1997
 *
 * 12/18/97 PAL
 * Deleted everything to do with barriers.
 * Changed to use InterlockedExchange to atomically set/test a lock 
 * which eliminated all assembler code. Had to change locks to LONG.
 * 
 */

#define	L_UNLOCKED	   0
#define	L_LOCKED	     1
#define L_MAXTESTS  4000
#define L_SLEEPTIME    0


/*
 * A spin lock allows for mutual exclusion on
 * data structures.
 */

typedef long	splock_t ;       /* spin lock */

__inline int S_LOCK( splock_t *laddr )
{ int cnt ;

  while( InterlockedExchange( laddr, L_LOCKED ) == L_LOCKED ){
    cnt = L_MAXTESTS ;
    /* check no more than L_MAXTESTS times then yield */
		while( *laddr == L_LOCKED ) { 
			cnt-- ;
			if( cnt < 0 ){ 
				Sleep(L_SLEEPTIME) ;
				cnt = L_MAXTESTS ;
			}
		}
  }
  return(0) ;
}

__inline int S_TRY_LOCK( splock_t *laddr )
{ long oldval ;

	oldval = InterlockedExchange( laddr, L_LOCKED ) ;
	if( oldval == L_UNLOCKED) return(0) ;
	else                      return(1) ;
}


__inline static void S_UNLOCK( splock_t *laddr )
{
	*laddr = L_UNLOCKED;
}

__inline static void S_INIT_LOCK( splock_t *laddr )
{
	S_UNLOCK( laddr );
}


#endif
