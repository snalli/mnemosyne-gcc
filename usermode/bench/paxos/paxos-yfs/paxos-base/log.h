#ifndef log_h
#define log_h

#include <string>
#include <vector>


class acceptor;

class log {
 private:
  std::string name;
  acceptor *pxs;
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
