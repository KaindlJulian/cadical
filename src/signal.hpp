#ifndef _signal_hpp_INCLUDED
#define _signal_hpp_INCLUDED

namespace CaDiCaL {

// Helper class for handling signals in applications.

class Handler {
public:
  Handler () {}
  virtual ~Handler () {}
  virtual void catch_signal (int sig) = 0;
#if !defined(_WIN32) && !defined(__wasi__)
  virtual void catch_alarm ();
#endif
};

class Signal {

public:
  static void set (Handler *);
  static void reset ();
#if !defined(_WIN32) && !defined(__wasi__)
  static void alarm (int seconds);
  static void reset_alarm ();
#endif

  static const char *name (int sig);
};

} // namespace CaDiCaL

#endif
