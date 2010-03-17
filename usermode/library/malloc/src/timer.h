///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// The Hoard Multiprocessor Memory Allocator
// www.hoard.org
//
// Author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2001, The University of Texas at Austin.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation, http://www.fsf.org.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
//////////////////////////////////////////////////////////////////////////////


#ifndef _TIMER_H_
#define _TIMER_H_

#ifdef __SVR4 // Solaris
#include <sys/time.h>
#include <strstream.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/procfs.h>
#include <stdio.h>
#endif // __SVR4

#include <time.h>

#if defined(unix) || defined(__linux)
#include <sys/time.h>
#include <unistd.h>
#endif


#ifdef __sgi
#include <sys/types.h>
#include <sys/times.h>
#include <limits.h>
#endif


#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#ifndef WIN32
#define WIN32 1
#endif
#include <windows.h>
#endif

/*** USAGE: ***

   Timer t;

   t.start();
   // do hairy computation 1.
   t.stop();

   cout << "The first hairy computation took " << (double) t << " seconds." << endl;

   t.reset();

   t.start();
   // do hairy computation 2.
   t.stop();

   cout << "The second hairy computation took " << (double) t << " seconds." << endl;

*/

class Timer {

public:

  Timer (void)
    : _starttime (0),
      _elapsedtime (0)
    {}

  // Start the timer.
  void start (void) { _starttime = _time(); }

  // Stop the timer.
  void stop (void) { _elapsedtime += _time() - _starttime; }

  // Reset the timer.
  void reset (void) { _starttime = _elapsedtime = 0; }

  // Set the timer.
  void set (double secs) { _starttime = 0; _elapsedtime = _sectotime (secs);}

  // Return the number of seconds elapsed.
  operator double (void) { return _timetosec (_elapsedtime); }


private:

  // The _timer variable will be different depending on the OS.
  // We try to use the best timer available.

#ifdef __sgi
#define TIMER_FOUND

  long _starttime, _elapsedtime;

  long _time (void) {
    struct tms t;
    long ticks = times (&t);
    return ticks;
  }

  double _timetosec (long t) {
    return ((double) (t) / CLK_TCK);
  }

  long _sectotime (double sec) {
    return (long) sec * CLK_TCK;
  }
#endif

#ifdef __SVR4 // Solaris
#define TIMER_FOUND
  hrtime_t	_starttime, _elapsedtime;

  virtual hrtime_t _time (void) {
    return gethrtime();
  }

  hrtime_t _sectotime (double sec) { return (hrtime_t) (sec * 1.0e9); }

  double _timetosec (hrtime_t& t) {
    return ((double) (t) / 1.0e9);
  }
#endif // __SVR4

#if defined(MAC) || defined(macintosh)
#define TIMER_FOUND
  double		_starttime, _elapsedtime;

  double _time (void) {
    return get_Mac_microseconds();
  }

  double _timetosec (hrtime_t& t) {
    return t;
  }
#endif // MAC

#ifdef WIN32
#define TIMER_FOUND
  DWORD _starttime, _elapsedtime;

  DWORD _time (void) {
    return GetTickCount();
  }

  double _timetosec (DWORD& t) {
    return (double) t / 1000.0;
  }

  unsigned long _sectotime (double sec) {
		return (unsigned long)(sec * 1000);
	}

#endif // WIN32


#ifndef TIMER_FOUND

  long _starttime, _elapsedtime;

  long _time (void) {
    struct timeval t;
    struct timezone tz;
    gettimeofday (&t, &tz);
    return t.tv_sec * 1000000 + t.tv_usec;
  }

  double _timetosec (long t) {
    return ((double) (t) / 1000000);
  }

  long _sectotime (double sec) {
    return (long) sec * 1000000;
  }

#endif // TIMER_FOUND

#undef TIMER_FOUND

};


#ifdef __SVR4 // Solaris
class VirtualTimer : public Timer {
public:
  hrtime_t _time (void) {
    return gethrvtime();
  }
};  
#endif

#endif
