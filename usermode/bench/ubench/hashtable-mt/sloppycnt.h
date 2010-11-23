#ifndef _SLOPPYCNT_H_CC101_
#define _SLOPPYCNT_H_CC101_

#include <stdint.h>
#include <pthread.h>

#define CACHELINE_SIZE    64
#define NUM_CPUS          16
#define RELEASE_THRESHOLD 16

#define FOR_ALL_TYPES(ACTION,name)        \
ACTION (name,uint8_t,uint8)               \
ACTION (name,uint16_t,uint16)             \
ACTION (name,uint32_t,uint32)             \
ACTION (name,uint64_t,uint64)             


#define CACHELINE_CNT_T(name, type, typestr)                                 \
  typedef struct {                                                           \
    type cnt;                                                                \
	char pad0[CACHELINE_SIZE - 2*sizeof(type)];                              \
  } cacheline_cnt_##typestr##_t;

#define SLOPPYCNT_T(name, type, typestr)                                     \
  typedef struct {                                                           \
    cacheline_cnt_##typestr##_t global;                                      \
	cacheline_cnt_##typestr##_t local[NUM_CPUS];                             \
	pthread_mutex_t             mutex;                                       \
  } sloppycnt_##typestr##_t;

#define SLOPPYCNT_INIT(name, type, typestr)                                  \
  __attribute__((tm_callable))                                               \
  static inline                                                              \
  int sloppycnt_##typestr##_init(sloppycnt_##typestr##_t *sloppycnt)         \
                                                                             \
  {                                                                          \
    int i;                                                                   \
    sloppycnt->global.cnt=0;                                                 \
	__tm_waiver {                                                            \
      pthread_mutex_init(&sloppycnt->mutex, NULL);                           \
    }	                                                                     \
	for (i=0; i<NUM_CPUS; i++) {                                             \
	  sloppycnt->local[i].cnt = 0;                                           \
	}                                                                        \
    return 0;                                                                \
  }


#define SLOPPYCNT_ADD(name, type, typestr)                                   \
  __attribute__((tm_callable))                                               \
  static inline                                                              \
  void sloppycnt_##typestr##_add(int cpuid,                                  \
                                 sloppycnt_##typestr##_t *sloppycnt,         \
                                 type val,                                   \
                                 int thread_safe)                            \
                                                                             \
  {                                                                          \
    if (sloppycnt->local[cpuid].cnt > val) {                                 \
	  sloppycnt->local[cpuid].cnt -= val;                                    \
      return;                                                                \
	}                                                                        \
    if (thread_safe) {	                                                     \
	  __tm_waiver {                                                          \
        pthread_mutex_lock(&sloppycnt->mutex);                               \
      }                                                                      \
      sloppycnt->global.cnt += val;                                          \
	  __tm_waiver {                                                          \
        pthread_mutex_unlock(&sloppycnt->mutex);                             \
      }                                                                      \
    } else {                                                                 \
      sloppycnt->global.cnt += val;                                          \
    }                                                                        \
  }


#define SLOPPYCNT_SUB(name, type, typestr)                                   \
  __attribute__((tm_callable))                                               \
  static inline                                                              \
  void sloppycnt_##typestr##_sub(int cpuid,                                  \
                                 sloppycnt_##typestr##_t *sloppycnt,         \
                                 type val,                                   \
                                 int thread_safe)                            \
                                                                             \
  {                                                                          \
    sloppycnt->local[cpuid].cnt += val;                                      \
    if (sloppycnt->local[cpuid].cnt > RELEASE_THRESHOLD) {                   \
      if (thread_safe) {	                                                 \
	    __tm_waiver {                                                        \
          pthread_mutex_lock(&sloppycnt->mutex);                             \
        }                                                                    \
	    sloppycnt->local[cpuid].cnt -= val;                                  \
        sloppycnt->global.cnt -= val;                                        \
	    __tm_waiver {                                                        \
          pthread_mutex_unlock(&sloppycnt->mutex);                           \
        }                                                                    \
	  } else {                                                               \
	    sloppycnt->local[cpuid].cnt -= val;                                  \
        sloppycnt->global.cnt -= val;                                        \
      }                                                                      \
	}                                                                        \
  }


#define SLOPPYCNT_GET(name, type, typestr)                                   \
  __attribute__((tm_callable))                                               \
  static inline                                                              \
  type sloppycnt_##typestr##_get(sloppycnt_##typestr##_t *sloppycnt)         \
                                                                             \
  {                                                                          \
    /* TODO: Actually the actual value could be less. We need to coordinate*/\
	/* with ther other processors/threads and check their local counters.  */\
	/* We could do this by using biased locks!                             */\
    return sloppycnt->global.cnt;                                            \
  }




FOR_ALL_TYPES(CACHELINE_CNT_T, dummy)
FOR_ALL_TYPES(SLOPPYCNT_T, dummy)
FOR_ALL_TYPES(SLOPPYCNT_INIT, dummy)
FOR_ALL_TYPES(SLOPPYCNT_ADD, dummy)
FOR_ALL_TYPES(SLOPPYCNT_SUB, dummy)
FOR_ALL_TYPES(SLOPPYCNT_GET, dummy)

#endif
