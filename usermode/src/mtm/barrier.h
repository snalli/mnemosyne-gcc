#ifndef _BARRIER_H
#define _BARRIER_H

#define DECLARE_READ_BARRIER(NAME, T, LOCK)                                    \
_ITM_TYPE_##T _ITM_CALL_CONVENTION mtm_##NAME##LOCK##T(mtm_thread_t *td,       \
                                        const _ITM_TYPE_##T *ptr);

#define DECLARE_WRITE_BARRIER(NAME, T, LOCK)                                   \
void _ITM_CALL_CONVENTION mtm_##NAME##LOCK##T(mtm_thread_t *td,                \
                               _ITM_TYPE_##T *ptr,                             \
                               _ITM_TYPE_##T val);


#define DECLARE_READ_BARRIERS(name, type, encoding)                            \
  DECLARE_READ_BARRIER(name, encoding, R)                                      \
  DECLARE_READ_BARRIER(name, encoding, RaR)                                    \
  DECLARE_READ_BARRIER(name, encoding, RaW)                                    \
  DECLARE_READ_BARRIER(name, encoding, RfW)                                    \


#define DECLARE_WRITE_BARRIERS(name, type, encoding)                           \
  DECLARE_WRITE_BARRIER(name, encoding, W)                                     \
  DECLARE_WRITE_BARRIER(name, encoding, WaR)	                               \
  DECLARE_WRITE_BARRIER(name, encoding, WaW)



#define READ_BARRIER(NAME, T, LOCK) \
_ITM_TYPE_##T _ITM_CALL_CONVENTION mtm_##NAME##LOCK##T(mtm_thread_t *td,       \
                                                       const _ITM_TYPE_##T *ptr)\
{                                                                              \
    uintptr_t iptr = (uintptr_t) ptr;                                          \
    uintptr_t iline = iptr & -CACHELINE_SIZE;                                  \
    uintptr_t iofs = iptr & (CACHELINE_SIZE - 1);                              \
    mtm_cacheline *line = NAME##LOCK (td, iline);                              \
    _ITM_TYPE_##T ret;                                                         \
                                                                               \
    if (STRICT_ALIGNMENT                                                       \
        ? (iofs & (sizeof (ret) - 1)) == 0                                     \
        : iofs + sizeof(ret) <= CACHELINE_SIZE)                                \
    {                                                                          \
        return *(_ITM_TYPE_##T *)&line->b[iofs];                               \
    }                                                                          \
    else if (STRICT_ALIGNMENT && iofs + sizeof(ret) <= CACHELINE_SIZE)         \
    {                                                                          \
        memcpy (&ret, &line->b[iofs], sizeof (ret));                           \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        uintptr_t ileft = CACHELINE_SIZE - iofs;                               \
        memcpy (&ret, &line->b[iofs], ileft);                                  \
        line = NAME##LOCK (td, iline + CACHELINE_SIZE);                        \
        memcpy ((char *)&ret + ileft, line, sizeof(ret) - ileft);              \
    }                                                                          \
    return ret;                                                                \
}


#define WRITE_BARRIER(NAME, T, LOCK)                                           \
void _ITM_CALL_CONVENTION mtm_##NAME##LOCK##T(mtm_thread_t *td,                \
                                              _ITM_TYPE_##T *ptr,              \
                                              _ITM_TYPE_##T val)               \
{									                                           \
    uintptr_t iptr = (uintptr_t) ptr;                                          \
    uintptr_t iline = iptr & -CACHELINE_SIZE;                                  \
    uintptr_t iofs = iptr & (CACHELINE_SIZE - 1);                              \
    mtm_cacheline_mask m = ((mtm_cacheline_mask)1 << sizeof(val)) - 1;         \
    mtm_cacheline_mask_pair pair = NAME##LOCK (td, iline);                     \
                                                                               \
    if (STRICT_ALIGNMENT                                                       \
        ? (iofs & (sizeof (val) - 1)) == 0                                     \
        : iofs + sizeof(val) <= CACHELINE_SIZE)                                \
    {                                                                          \
        *(_ITM_TYPE_##T *)&pair.line->b[iofs] = val;                           \
        *pair.mask |= m << iofs;                                               \
    }                                                                          \
    else if (STRICT_ALIGNMENT && iofs + sizeof(val) <= CACHELINE_SIZE)         \
    {                                                                          \
        memcpy (&pair.line->b[iofs], &val, sizeof (val));                      \
        *pair.mask |= m << iofs;                                               \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        uintptr_t ileft = CACHELINE_SIZE - iofs;                               \
        memcpy (&pair.line->b[iofs], &val, ileft);                             \
        *pair.mask |= m << iofs;                                               \
        pair = NAME##LOCK (td, iline + CACHELINE_SIZE);                        \
        memcpy (pair.line, (char *)&val + ileft, sizeof(val) - ileft);         \
        *pair.mask |= m >> ileft;                                              \
    }                                                                          \
}


#define DEFINE_READ_BARRIERS(name, type, encoding)      \
  READ_BARRIER(name, encoding, R)                       \
  READ_BARRIER(name, encoding, RaR)	                    \
  READ_BARRIER(name, encoding, RaW)	                    \
  READ_BARRIER(name, encoding, RfW)		                \


#define DEFINE_WRITE_BARRIERS(name, type, encoding)     \
  WRITE_BARRIER(name, encoding, W)                      \
  WRITE_BARRIER(name, encoding, WaR)                    \
  WRITE_BARRIER(name, encoding, WaW)



FOR_ALL_TYPES(DECLARE_READ_BARRIERS, wbetl_)
FOR_ALL_TYPES(DECLARE_WRITE_BARRIERS, wbetl_)

#endif
