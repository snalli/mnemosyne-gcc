#include "paxos.h"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include "jsl_log.h"
#include <unistd.h>
#include <stdio.h>
#include "hrtime.h"
#include "log_tornbit.h"

#define LOGDIR "/mnt/pcmfs"

//#define MEASURE_TIME
//#define MNEMOSYNE
#define STDIO_STREAM
#define FSYNC

#define LOG_INSTANCE 0x1
#define LOG_HIGH     0x2
#define LOG_PROP     0x4

// Paxos must maintain some durable state (i.e., that survives power
// failures) to run Paxos correct.  This module implements a log with
// all durable state to run Paxos.  Since the values chosen correspond
// to views, the log contains all views since the beginning of time.

log::log(acceptor *_acc, std::string _me)
  : pxs (_acc)
{
  name = "paxos-" + _me + ".log";
  logread();
  pcm_storeset = pcm_storeset_get ();

  /* Register persistent logical log type with the log manager */
  m_logmgr_register_logtype(pcm_storeset, LF_TYPE_PAXOS_TORNBIT, &log_tornbit_ops);
  /* Ask log manager to perform recovery on the new log type */
  //m_logmgr_do_recovery(pcm_storeset);
  m_logmgr_alloc_log(pcm_storeset, LF_TYPE_PAXOS_TORNBIT, LF_ASYNC_TRUNCATION, &plog_dsc);
  plog = (m_log_tornbit_t *) plog_dsc->log;


}

void
log::logread(void)
{
  std::ifstream from;
  std::string type;
  unsigned instance;

  from.open(name.c_str());
  jsl_log(JSL_DBG_4, "logread\n");
  while (from >> type) {
    if (type == "done") {
      std::string v;
      from >> instance;
      from.get();
      getline(from, v);
      pxs->values[instance] = v;
      pxs->instance_h = instance;
      jsl_log(JSL_DBG_4, "logread: instance: %d w. v = %s\n", instance, 
	      pxs->values[instance].c_str());
      pxs->v_a.clear();
      pxs->n_h.n = 0;
      pxs->n_a.n = 0;
    } else if (type == "high") {
      from >> pxs->n_h.n;
      from >> pxs->n_h.m;
      jsl_log(JSL_DBG_4, "logread: high update: %d(%s)\n", pxs->n_h.n, pxs->n_h.m.c_str());
    } else if (type == "prop") {
      std::string v;
      from >> pxs->n_a.n;
      from >> pxs->n_a.m;
      from.get();
      getline(from, v);
      pxs->v_a = v;
      jsl_log(JSL_DBG_4, "logread: prop update %d(%s) with v = %s\n", pxs->n_a.n, 
	     pxs->n_a.m.c_str(), pxs->v_a.c_str());
    } else {
      jsl_log(JSL_DBG_4, "logread: unknown log record\n");
      assert(0);
    }
  } 
  from.close();
}

std::string 
log::dump()
{
  std::ifstream from;
  std::string res;
  std::string v;
  from.open(name.c_str());
  while (getline(from, v)) {
    res = res + v + "\n";
  }
  from.close();
  return res;
}

void
log::restore(std::string s)
{
  std::ofstream f;
  jsl_log(JSL_DBG_4, "restore: %s\n", s.c_str());
  f.open(name.c_str(), std::ios::trunc);
  f << s;
  f.close();
}

// XXX should be an atomic operation
void
log::loginstance_stream(unsigned instance, std::string v)
{
 #ifndef STDIO_STREAM 
  std::ofstream f;
  f.open(name.c_str(), std::ios::app);
  f << "done";
  f << " ";
  f << instance;
  f << " ";
  f << v;
  f << "\n";
  f.close();
#else
  char path[64];
  std::stringstream oss;
  FILE *fout;
  oss << "done";
  oss << " ";
  oss << instance;
  oss << " ";
  oss << v;
  oss << "\n";
  sprintf(path, "%s/%s", LOGDIR, name.c_str());
  fout = fopen(path, "a");
  fprintf(fout, "%s", oss.str().c_str());
# ifdef FSYNC   
  fflush(fout);
  fsync(fileno(fout));
# endif  
  fclose(fout);
#endif
}

void
log::loghigh_stream(prop_t n_h)
{
#ifndef STDIO_STREAM 
  std::ofstream f;
  f.open(name.c_str(), std::ios::app);
  f << "high";
  f << " ";
  f << n_h.n;
  f << " ";
  f << n_h.m;
  f << "\n";
  f.close();
#else
  std::stringstream oss;
  char path[64];
  FILE *fout;
  oss << "high";
  oss << " ";
  oss << n_h.n;
  oss << " ";
  oss << n_h.m;
  oss << "\n";
  sprintf(path, "%s/%s", LOGDIR, name.c_str());
  fout = fopen(path, "a");
  fprintf(fout, "%s", oss.str().c_str());
# ifdef FSYNC   
  fflush(fout);
  fsync(fileno(fout));
# endif  
  fclose(fout);
#endif
}


void
log::logprop_stream(prop_t n, std::string v)
{
#ifndef STDIO_STREAM
  std::ofstream f;
  f.open(name.c_str(), std::ios::app);
  f << "prop";
  f << " ";
  f << n.n;
  f << " ";
  f << n.m;
  f << " ";
  f << v;
  f << "\n";
  f.close();
#else
  char path[64];
  std::stringstream oss;
  FILE *fout;
  oss << "prop";
  oss << " ";
  oss << n.n;
  oss << " ";
  oss << n.m;
  oss << "\n";
  sprintf(path, "%s/%s", LOGDIR, name.c_str());
  fout = fopen(path, "a");
  fprintf(fout, "%s", oss.str().c_str());
# ifdef FSYNC   
  fflush(fout);
  fsync(fileno(fout));
# endif  
  fclose(fout);
#endif
}


void
log::loginstance_mnemosyne(unsigned instance, std::string v)
{
  m_log_tornbit_begin(pcm_storeset, plog, LOG_INSTANCE);
  m_log_tornbit_write(pcm_storeset, plog, (pcm_word_t) instance);
  m_log_tornbit_write_char(pcm_storeset, plog, v.c_str(), v.length());
  m_log_tornbit_commit(pcm_storeset, plog);
}


void
log::loghigh_mnemosyne(prop_t n_h)
{
  m_log_tornbit_begin(pcm_storeset, plog, LOG_HIGH);
  m_log_tornbit_write(pcm_storeset, plog, (pcm_word_t) n_h.n);
  m_log_tornbit_write_char(pcm_storeset, plog, n_h.m.c_str(), n_h.m.length());
  m_log_tornbit_commit(pcm_storeset, plog);
}


void
log::logprop_mnemosyne(prop_t n, std::string v)
{
  m_log_tornbit_begin(pcm_storeset, plog, LOG_PROP);
  m_log_tornbit_write(pcm_storeset, plog, (pcm_word_t) n.n);
  m_log_tornbit_write_char(pcm_storeset, plog, n.m.c_str(), n.m.length());
  m_log_tornbit_write_char(pcm_storeset, plog, v.c_str(), v.length());
  m_log_tornbit_commit(pcm_storeset, plog);
}


void
log::loginstance(unsigned instance, std::string v)
{
#ifdef MEASURE_TIME
	hrtime_t start;
	hrtime_t stop;
	start = hrtime_cycles();
#endif

#ifdef MNEMOSYNE
	loginstance_mnemosyne(instance, v);
#else
	loginstance_stream(instance, v);
#endif

#ifdef MEASURE_TIME
	stop = hrtime_cycles();
	printf("loginstance: %llu\n", HRTIME_CYCLE2NS(stop-start));
#endif	
}


void
log::logprop(prop_t n, std::string v)
{
#ifdef MEASURE_TIME
	hrtime_t start;
	hrtime_t stop;
	start = hrtime_cycles();
#endif

#ifdef MNEMOSYNE
	logprop_mnemosyne(n, v);
#else
	logprop_stream(n, v);
#endif

#ifdef MEASURE_TIME
	stop = hrtime_cycles();
	printf("logprop: %llu\n", HRTIME_CYCLE2NS(stop-start));
#endif	
}


void
log::loghigh(prop_t n_h)
{
#ifdef MEASURE_TIME
	hrtime_t start;
	hrtime_t stop;
	start = hrtime_cycles();
#endif
#ifdef MNEMOSYNE
	loghigh_mnemosyne(n_h);
#else
	loghigh_stream(n_h);
#endif

#ifdef MEASURE_TIME
	stop = hrtime_cycles();
	printf("loghigh: %llu\n", HRTIME_CYCLE2NS(stop-start));
#endif	
}
