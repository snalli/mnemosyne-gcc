#include "handle.h"
#include <stdio.h>
#include "jsl_log.h"

handle_mgr mgr;

handle::handle(std::string m) 
{
  h = mgr.get_handle(m);
}

handle::~handle() 
{
  if (h != 0) mgr.done_handle(h);
}

handle_mgr::handle_mgr()
{
  assert (pthread_mutex_init(&handle_mutex, NULL) == 0);
}

struct hinfo *
handle_mgr::get_handle(std::string m)
{
  int ret;
  assert(pthread_mutex_lock(&handle_mutex)==0);
  rpcc *cl = 0;
  struct hinfo *h = 0;
  if (hmap.find(m) == hmap.end()) {
    sockaddr_in dstsock;
    make_sockaddr(m.c_str(), &dstsock);
    cl = new rpcc(dstsock);
    jsl_log(JSL_DBG_4, "paxos::get_handle trying to bind...%s\n", m.c_str());
    ret = cl->bind(rpcc::to(1000));
    if (ret < 0) {
      jsl_log(JSL_DBG_4, "handle_mgr::get_handle bind failure! %s %d\n", m.c_str(), ret);
    } else {
      jsl_log(JSL_DBG_4, "handle_mgr::get_handle bind succeeded %s\n", m.c_str());
      hmap[m].cl = cl;
      hmap[m].refcnt = 1;
      hmap[m].del = false;
      hmap[m].m = m;
      h = &hmap[m];
    }
  } else if (!hmap[m].del) {
      hmap[m].refcnt++;
      h = &hmap[m];
  }
  assert(pthread_mutex_unlock(&handle_mutex)==0);
  return h;
}

void 
handle_mgr::done_handle(struct hinfo *h)
{
  assert(pthread_mutex_lock(&handle_mutex)==0);
  h->refcnt--;
  if (h->refcnt <= 0 && h->del)
    delete_handle_wo(h->m);
  assert(pthread_mutex_unlock(&handle_mutex)==0);
}

void
handle_mgr::delete_handle(std::string m)
{
  assert(pthread_mutex_lock(&handle_mutex)==0);
  delete_handle_wo(m);
  assert(pthread_mutex_unlock(&handle_mutex)==0);
}

// Must be called with handle_mutex locked.
void
handle_mgr::delete_handle_wo(std::string m)
{
  if (hmap.find(m) == hmap.end()) {
    jsl_log(JSL_DBG_4, "handle_mgr::delete_handle_wo: cl %s isn't in cl list\n", m.c_str());
  } else {
    jsl_log(JSL_DBG_4, "handle_mgr::delete_handle_wo: cl %s refcnt %d\n", m.c_str(),
	   hmap[m].refcnt);
    if (hmap[m].refcnt == 0) {
      hmap[m].cl->cancel();
      delete hmap[m].cl;
      hmap.erase(m);
    } else {
      hmap[m].del = true;
    }
  }
}
