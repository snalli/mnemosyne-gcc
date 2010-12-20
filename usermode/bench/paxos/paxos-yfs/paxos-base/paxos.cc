#include "paxos.h"
#include "handle.h"
// #include <signal.h>
#include <stdio.h>
#include "jsl_log.h"

// This module implements the proposer and acceptor of the Paxos
// distributed algorithm as described by Lamport's "Paxos Made
// Simple".  To kick off an instance of Paxos, the caller supplies a
// list of nodes, a proposed value, and invokes the proposer.  If the
// majority of the nodes agree on the proposed value after running
// this instance of Paxos, the acceptor invokes the upcall
// paxos_commit to inform higher layers of the agreed value for this
// instance.

// in this implementation, a node has three roles, namely acceptor, proposer,
// and learner (learner is an implied role).

bool
operator> (const prop_t &a, const prop_t &b)
{
  return (a.n > b.n || (a.n == b.n && a.m > b.m));
}

bool
operator>= (const prop_t &a, const prop_t &b)
{
  return (a.n > b.n || (a.n == b.n && a.m >= b.m));
}

std::string
print_members(const std::vector<std::string> &nodes)
{
  std::string s;
  s.clear();
  for (unsigned i = 0; i < nodes.size(); i++) {
    s += nodes[i];
    if (i < (nodes.size()-1))
      s += ",";
  }
  return s;
}

bool isamember(std::string m, const std::vector<std::string> &nodes)
{
  for (unsigned i = 0; i < nodes.size(); i++) {
    if (nodes[i] == m) return 1;
  }
  return 0;
}

bool
proposer::isrunning()
{
  bool r;
  assert(pthread_mutex_lock(&pxs_mutex)==0);
  r = !stable;
  assert(pthread_mutex_unlock(&pxs_mutex)==0);
  return r;
}

// check if the servers in l2 contains a majority of servers in l1
bool
proposer::majority(const std::vector<std::string> &l1, 
		const std::vector<std::string> &l2)
{
  unsigned n = 0;

  for (unsigned i = 0; i < l1.size(); i++) {
    if (isamember(l1[i], l2))
      n++;
  }
  return n >= (l1.size() >> 1) + 1;
}

proposer::proposer(class paxos_change *_cfg, class acceptor *_acceptor, 
		   std::string _me)
  : cfg(_cfg), acc (_acceptor), me (_me), break1 (false), break2 (false), 
    stable (true)
{
  my_n.m = _me;
  assert (pthread_mutex_init(&pxs_mutex, NULL) == 0);

}

void
proposer::setn()
{
  my_n.n = acc->get_n_h().n + 1 > my_n.n + 1 ? acc->get_n_h().n + 1 : my_n.n + 1;
}

bool
proposer::run(int instance, std::vector<std::string> newnodes, std::string newv)
{
  std::vector<std::string> accepts;
  std::vector<std::string> nodes;
  std::vector<std::string> nodes1;
  std::string v;
  bool r = false;

  pthread_mutex_lock(&pxs_mutex);
  jsl_log(JSL_DBG_4, "start: initiate paxos for %s w. i=%d v=%s stable=%d\n",
      print_members(newnodes).c_str(), instance, newv.c_str(), stable);
  if (!stable) {  // already running proposer?
    jsl_log(JSL_DBG_4, "proposer::run: already running\n");
    pthread_mutex_unlock(&pxs_mutex);
    return false;
  }
  stable = false; // set stable flag
  setn();
  accepts.clear();
  nodes.clear();
  v.clear();
  c_nodes = newnodes; // update c_nodes with the new value told by config
  c_v = newv; // update c_v
  nodes = c_nodes;
  if (prepare(instance, accepts, nodes, v)) {

    if (majority(c_nodes, accepts)) {
      jsl_log(JSL_DBG_4, "paxos::manager: received a majority of prepare responses\n");

      if (v.size() == 0) {
        v = c_v;
      }

      breakpoint1();

      // qy: we only need to send accept() to nodes who replied to me. those
      // who didn't either are crashed or will not accept my proposal anyway
      nodes1 = accepts;
      accepts.clear();
      accept(instance, accepts, nodes1, v);

      if (majority(c_nodes, accepts)) {
        jsl_log(JSL_DBG_4, "paxos::manager: received a majority of accept responses\n");

        breakpoint2();

        // qy: we send decide message only to those who accepted our
        // proposal, because that's enough for a consensus
        decide(instance, accepts, v);
        r = true;
      } else {
        jsl_log(JSL_DBG_4, "paxos::manager: no majority of accept responses\n");
      }
    } else {
      jsl_log(JSL_DBG_4, "paxos::manager: no majority of prepare responses\n");
    }
  } else {
    jsl_log(JSL_DBG_4, "paxos::manager: prepare is rejected %d\n", stable);
  }
  stable = true;
  pthread_mutex_unlock(&pxs_mutex);
  return r;
}

bool
proposer::prepare(unsigned instance, std::vector<std::string> &accepts, 
         std::vector<std::string> nodes,
         std::string &v)
{
  std::vector<std::string>::iterator itr;
  prop_t highest;
  highest.m.clear();
  for (itr = nodes.begin(); itr != nodes.end(); ++itr) {
    handle h(*itr);
    if (h.get_rpcc()) {
      int r;
      paxos_protocol::preparearg arg;
      paxos_protocol::prepareres resp;
      arg.n = my_n;
      arg.instance = instance;
      assert(pthread_mutex_unlock(&pxs_mutex) == 0);
      r = h.get_rpcc()->call(paxos_protocol::preparereq, me, arg, resp,
          rpcc::to(1000));
      assert(pthread_mutex_lock(&pxs_mutex) == 0);
      if (r == paxos_protocol::OK) {
        if (resp.oldinstance == 0) {
          if (resp.accept) {
            accepts.push_back(*itr);
          }
          if (highest.m.size() == 0 || resp.n_a > highest) {
            highest = resp.n_a;
            v = resp.v_a;
          }
        } else {
          acc->commit(instance, resp.v_a);
        }
      } else {
        jsl_log(JSL_DBG_4, "failed to call preparereq\n");
      }
    }
  }
  // qy: what should we return here?
  return true;
}


void
proposer::accept(unsigned instance, std::vector<std::string> &accepts,
        std::vector<std::string> nodes, std::string v)
{
  std::vector<std::string>::iterator itr;
  for (itr = nodes.begin(); itr != nodes.end(); itr++) {
    handle h(*itr);
    if (h.get_rpcc()) {
      int sts, r;
      paxos_protocol::acceptarg arg;
      arg.instance = instance;
      arg.n = my_n;
      arg.v = v;
      assert(pthread_mutex_unlock(&pxs_mutex) == 0);
      sts = h.get_rpcc()->call(paxos_protocol::acceptreq, me, arg, r,
          rpcc::to(1000));
      assert(pthread_mutex_lock(&pxs_mutex) == 0);
      if (sts == paxos_protocol::OK) {
        if (r) {
          accepts.push_back(*itr);
        } else {
          jsl_log(JSL_DBG_4, "proposal rejected ins: %u, v: %s\n", instance, v.c_str());
        }
      }
    }
  }
}

void
proposer::decide(unsigned instance, std::vector<std::string> accepts, 
	      std::string v)
{
  std::vector<std::string>::iterator itr;
  for (itr = accepts.begin(); itr != accepts.end(); itr++) {
    handle h(*itr);
    if (h.get_rpcc()) {
      int r, sts;
      paxos_protocol::decidearg arg;
      arg.instance = instance;
      arg.v = v;
      assert(pthread_mutex_unlock(&pxs_mutex) == 0);
      sts = h.get_rpcc()->call(paxos_protocol::decidereq, me, arg, r,
          rpcc::to(1000));
      assert(pthread_mutex_lock(&pxs_mutex) == 0);
      if (sts != paxos_protocol::OK) {
        jsl_log(JSL_DBG_4, "failed to calll decidereq\n");
      }
    }
  }
}

acceptor::acceptor(class paxos_change *_cfg, bool _first, std::string _me, 
	     std::string _value)
  : cfg(_cfg), me (_me), instance_h(0)
{
  assert (pthread_mutex_init(&pxs_mutex, NULL) == 0);

  n_h.n = 0;
  n_h.m = me;
  n_a.n = 0;
  n_a.m = me;
  v_a.clear();

  l = new log (this, me);

  if (instance_h == 0 && _first) {
    values[1] = _value;
    l->loginstance(1, _value);
    instance_h = 1;
  }

  pxs = new rpcs(atoi(_me.c_str()));
  pxs->reg(paxos_protocol::preparereq, this, &acceptor::preparereq);
  pxs->reg(paxos_protocol::acceptreq, this, &acceptor::acceptreq);
  pxs->reg(paxos_protocol::decidereq, this, &acceptor::decidereq);
}

paxos_protocol::status
acceptor::preparereq(std::string src, paxos_protocol::preparearg a,
    paxos_protocol::prepareres &r)
{
  // handle a preparereq message from proposer
  pthread_mutex_lock(&pxs_mutex);
  if (a.instance <= instance_h) {
    r.oldinstance = 1;
    r.v_a = value(a.instance);
  } else {
    r.oldinstance = 0;
    if (a.n > n_h) {
      // i promise not to accept proposals that are <= yours
      r.accept = 1;
      n_h = a.n;
      l->loghigh(n_h);
    } else {
      // i won't accept your proposal because you have n <= n_h
      r.accept = 0;
    }
    r.n_a = n_a;
    r.v_a = v_a;
  }
  pthread_mutex_unlock(&pxs_mutex);
  return paxos_protocol::OK;
}

paxos_protocol::status
acceptor::acceptreq(std::string src, paxos_protocol::acceptarg a, int &r)
{

  // handle an acceptreq message from proposer
  pthread_mutex_lock(&pxs_mutex);
  if (a.instance > instance_h) {
    if (a.n >= n_h) {
      v_a = a.v;
      n_a = a.n;
      l->logprop(a.n, v_a);
      r = 1;
    } else {
      r = 0;
    }
    pthread_mutex_unlock(&pxs_mutex);
    return paxos_protocol::OK;
  } else {
    r = 0;
    pthread_mutex_unlock(&pxs_mutex);
    return paxos_protocol::ERR;
  }
}

paxos_protocol::status
acceptor::decidereq(std::string src, paxos_protocol::decidearg a, int &r)
{

  // handle an decide message from proposer
  commit(a.instance, a.v);
  return paxos_protocol::OK;
}

void
acceptor::commit_wo(unsigned instance, std::string value)
{
  //assume pxs_mutex is held
  jsl_log(JSL_DBG_4, "acceptor::commit: instance=%d has v= %s\n", instance, value.c_str());
  if (instance > instance_h) {
    jsl_log(JSL_DBG_4, "commit: highestaccepteinstance = %d\n", instance);
    values[instance] = value;
    l->loginstance(instance, value);
    instance_h = instance;
    n_h.n = 0;
    n_h.m = me;
    n_a.n = 0;
    n_a.m = me;
    v_a.clear();
    if (cfg) {
      pthread_mutex_unlock(&pxs_mutex);
      cfg->paxos_commit(instance, value);
      pthread_mutex_lock(&pxs_mutex);
    }
  }
}

void
acceptor::commit(unsigned instance, std::string value)
{
  pthread_mutex_lock(&pxs_mutex);
  commit_wo(instance, value);
  pthread_mutex_unlock(&pxs_mutex);
}

std::string
acceptor::dump()
{
  return l->dump();
}

void
acceptor::restore(std::string s)
{
  l->restore(s);
  l->logread();
}



// For testing purposes

// Call this from your code between phases prepare and accept of proposer
void
proposer::breakpoint1()
{
  if (break1) {
    jsl_log(JSL_DBG_4, "Dying at breakpoint 1!\n");
    exit(1);
  }
}

// Call this from your code between phases accept and decide of proposer
void
proposer::breakpoint2()
{
  if (break2) {
    jsl_log(JSL_DBG_4, "Dying at breakpoint 2!\n");
    exit(1);
  }
}

void
proposer::breakpoint(int b)
{
  if (b == 3) {
    jsl_log(JSL_DBG_4, "Proposer: breakpoint 1\n");
    break1 = true;
  } else if (b == 4) {
    jsl_log(JSL_DBG_4, "Proposer: breakpoint 2\n");
    break2 = true;
  }
}
