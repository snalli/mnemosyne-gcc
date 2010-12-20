#ifndef driver_h
#define driver_h

#include <string>
#include <vector>
#include "paxos.h"

class driver : public paxos_change {
 private:
  acceptor *acc;
  proposer *pro;
  rpcs *pxsrpc;
  unsigned myvid;
  std::string first;
  std::string me;
  std::vector<std::string> mems;
  pthread_mutex_t cfg_mutex;
  std::string value(std::vector<std::string> mems);
  void reconstruct();
 public:
  driver(std::string _first, std::string _me);
  unsigned vid() { return myvid; }
  std::string myaddr() { return me; };
  std::string dump() { return acc->dump(); };
  bool stress();
  bool add_member(std::string);
  void paxos_commit(unsigned instance, std::string v);
  rpcs *get_rpcs() { return acc->get_rpcs(); }
  void breakpoint(int b) { pro->breakpoint(b); }
};

#endif
