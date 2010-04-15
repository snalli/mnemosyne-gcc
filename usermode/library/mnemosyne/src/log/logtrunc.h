/**
 * \brief Interface to asynchronous log truncation.
 */
#ifndef _LOGTRUNC_H
#define _LOGTRUNC_H

m_result_t m_logtrunc_init(m_logmgr_t *mgr);
m_result_t m_logtrunc_truncate(pcm_storeset_t *set);

#endif /* _LOGTRUNC_H */
