#ifndef log_h
#define log_h

#include <string>
#include <vector>
#include <mnemosyne.h>
#include <log.h>
#include "log_tornbit.h"

class acceptor;

class log {
 private:
  std::string name;
  acceptor *pxs;
  pcm_storeset_t  *pcm_storeset;
  m_log_dsc_t     *plog_dsc; /**< The persistent tm log descriptor */
  m_log_tornbit_t *plog;     /**< The persistent tm log; this is to avoid dereferencing ptmlog_dsc in the fast path */
  void loginstance_mnemosyne(unsigned instance, std::string v);
  void loghigh_mnemosyne(prop_t n_h);
  void logprop_mnemosyne(prop_t n_a, std::string v);
  void loginstance_stream(unsigned instance, std::string v);
  void loghigh_stream(prop_t n_h);
  void logprop_stream(prop_t n_a, std::string v);
 public:
  log (acceptor*, std::string _me);
  std::string dump();
  void restore(std::string s);
  void logread(void);
  void loginstance(unsigned instance, std::string v);
  void loghigh(prop_t n_h);
  void logprop(prop_t n_a, std::string v);
};

#endif /* log_h */
