#include <sstream>
#include <iostream>
#include <stdio.h>
#include "driver.h"
#include "paxos.h"
#include "handle.h"
#include "jsl_log.h"


driver::driver(std::string _first, std::string _me) 
  : myvid (0), first (_first), me (_me)
{
  assert (pthread_mutex_init(&cfg_mutex, NULL) == 0);

  std::ostringstream ost;
  ost << me;

  acc = new acceptor(this, me == _first, me, ost.str());
  pro = new proposer(this, acc, me);

  // XXX hack; maybe should have its own port number
  pxsrpc = acc->get_rpcs();

  assert(pthread_mutex_lock(&cfg_mutex)==0);

  reconstruct();

  assert(pthread_mutex_unlock(&cfg_mutex)==0);
}


std::string
driver::value(std::vector<std::string> m)
{
  std::ostringstream ost;
  for (unsigned i = 0; i < m.size(); i++)  {
    ost << m[i];
    ost << " ";
  }
  return ost.str();
}


// caller should hold cfg_mutex
void
driver::reconstruct()
{
	std::vector<std::string> _mems;
	_mems.push_back(me);
	mems=_mems;
	jsl_log(JSL_DBG_4, "driver::reconstruct: members=(%s)\n", print_members(mems).c_str());
}


// Called by Paxos's acceptor.
void
driver::paxos_commit(unsigned instance, std::string value)
{
  myvid = instance;
  jsl_log(JSL_DBG_4, "driver::paxos_commit: instance=%d\n", instance);
/*
  std::string m;
  std::vector<std::string> newmem;
  assert(pthread_mutex_lock(&cfg_mutex)==0);

  newmem = members(value);
  jsl_log(JSL_DBG_4, "driver::paxos_commit: %d: %s\n", instance, 
	 print_members(newmem).c_str());

  for (unsigned i = 0; i < mems.size(); i++) {
    jsl_log(JSL_DBG_4, "driver::paxos_commit: is %s still a member?\n", mems[i].c_str());
    if (!isamember(mems[i], newmem) && me != mems[i]) {
      jsl_log(JSL_DBG_4, "driver::paxos_commit: delete %s\n", mems[i].c_str());
      mgr.delete_handle(mems[i]);
    }
  }

  mems = newmem;
  myvid = instance;
  if (vc) {
    assert(pthread_mutex_unlock(&cfg_mutex)==0);
    vc->commit_change();
    assert(pthread_mutex_lock(&cfg_mutex)==0);
  }
  assert(pthread_mutex_unlock(&cfg_mutex)==0);
  */
}


bool
driver::stress()
{
  std::vector<std::string> m;
  assert(pthread_mutex_lock(&cfg_mutex)==0);
  std::string v = "deadbeefdeadbeefdeadbeefdeadbeef";
  //std::string v(64, 'a');
  assert(pthread_mutex_unlock(&cfg_mutex)==0);
  bool r = pro->run(myvid+1, mems, v);
  assert(pthread_mutex_lock(&cfg_mutex)==0);
  if (r) {
    jsl_log(JSL_DBG_4, "driver::stress: proposer returned success\n");
    myvid=myvid+1;
  } else {
    printf("driver::stress: proposer returned failure\n");
    jsl_log(JSL_DBG_4, "driver::stress: proposer returned failure\n");
  }
  assert(pthread_mutex_unlock(&cfg_mutex)==0);
  return r;
}

bool
driver::add_member(std::string new_m)
{
  std::vector<std::string> m;
  assert(pthread_mutex_lock(&cfg_mutex)==0);
  mems.push_back(new_m);
  assert(pthread_mutex_unlock(&cfg_mutex)==0);
  return true;
}
