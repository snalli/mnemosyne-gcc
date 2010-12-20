#ifndef handle_h
#define handle_h

#include <string>
#include <vector>
#include "rpc.h"

struct hinfo {
  rpcc *cl;
  int refcnt;
  bool del;
  std::string m;
};

class handle {
 private:
  struct hinfo *h;
 public:
  handle(std::string m);
  ~handle();
  rpcc *get_rpcc() { return (h == 0) ? 0 : h->cl; };
};

class handle_mgr {
 private:
  pthread_mutex_t handle_mutex;
  std::map<std::string, struct hinfo> hmap;
 public:
  handle_mgr();
  struct hinfo *get_handle(std::string m);
  void done_handle(struct hinfo *h);
  void delete_handle(std::string m);
  void delete_handle_wo(std::string m);
};

extern class handle_mgr mgr;

#endif
