#ifndef _MNEMOSYNE_H
#define _MNEMOSYNE_H

#ifdef USETM
# define TM_PURE __attribute__((tm_pure))
# define TM_CALLABLE __attribute__((tm_callable))
# define TM_WAIVER __tm_waiver 

# define MTM_XACT_BEGIN(tag) __tm_atomic {

# define MTM_XACT_END(tag)                                                    \
  } 
 
# define __MTM_XACT_END(tag)                                                  \
  __tm_waiver {                                                               \
    printf("TRYCOMMIT: %s:%d\n", __FILE__, __LINE__);                         \
  }                                                                           \
  } printf("COMMIT: %s:%d\n", __FILE__, __LINE__);

#else

# define TM_PURE 
# define TM_CALLABLE 
# define TM_WAIVER

# define MTM_XACT_BEGIN(tag) {

# define MTM_XACT_END(tag)                                                    \
  } 
 
#endif   

#endif /* _MNEMOSYNE_H */   
