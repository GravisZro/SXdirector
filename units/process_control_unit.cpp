// STL
#include <memory>
#include <list>
#include <string>
#include <unordered_map>

// PUT
#include <put/socket.h>
#include <put/cxxutils/posix_helpers.h>
#include <put/cxxutils/vfifo.h>
#include <put/cxxutils/vterm.h>
#include <put/cxxutils/hashing.h>
#include <put/cxxutils/configmanip.h>
#include <put/specialized/fileevent.h>
#include <put/specialized/mountpoints.h>

#include "../string_helpers.h"

#define UNIT_NAME "process_control_unit"

#define SIG_MIN SIGINT
#define SIG_MAX SIGINT

#if defined(SIGILL)
# if SIGILL > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGILL
# endif
# if SIGILL < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGILL
# endif
#endif

#if defined(SIGABRT)
# if SIGABRT > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGABRT
# endif
# if SIGABRT < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGABRT
# endif
#endif

#if defined(SIGFPE)
# if SIGFPE > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGFPE
# endif
# if SIGFPE < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGFPE
# endif
#endif

#if defined(SIGSEGV)
# if SIGSEGV > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGSEGV
# endif
# if SIGSEGV < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGSEGV
# endif
#endif

#if defined(SIGTERM)
# if SIGTERM > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGTERM
# endif
# if SIGTERM < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGTERM
# endif
#endif


/* Historical signals specified by POSIX. */
#if defined(SIGHUP)
# if SIGHUP > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGHUP
# endif
# if SIGHUP < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGHUP
# endif
#endif

#if defined(SIGQUIT)
# if SIGQUIT > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGQUIT
# endif
# if SIGQUIT < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGQUIT
# endif
#endif

#if defined(SIGTRAP)
# if SIGTRAP > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGTRAP
# endif
# if SIGTRAP < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGTRAP
# endif
#endif

#if defined(SIGKILL)
# if SIGKILL > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGKILL
# endif
# if SIGKILL < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGKILL
# endif
#endif

#if defined(SIGBUS)
# if SIGBUS > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGBUS
# endif
# if SIGBUS < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGBUS
# endif
#endif

#if defined(SIGSYS)
# if SIGSYS > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGSYS
# endif
# if SIGSYS < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGSYS
# endif
#endif

#if defined(SIGPIPE)
# if SIGPIPE > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGPIPE
# endif
# if SIGPIPE < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGPIPE
# endif
#endif

#if defined(SIGALRM)
# if SIGALRM > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGALRM
# endif
# if SIGALRM < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGALRM
# endif
#endif


/* New(er) POSIX signals (1003.1-2008, 1003.1-2013).  */
#if defined(SIGURG)
# if SIGURG > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGURG
# endif
# if SIGURG < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGURG
# endif
#endif

#if defined(SIGSTOP)
# if SIGSTOP > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGSTOP
# endif
# if SIGSTOP < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGSTOP
# endif
#endif

#if defined(SIGTSTP)
# if SIGTSTP > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGTSTP
# endif
# if SIGTSTP < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGTSTP
# endif
#endif

#if defined(SIGCONT)
# if SIGCONT > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGCONT
# endif
# if SIGCONT < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGCONT
# endif
#endif

#if defined(SIGCHLD)
# if SIGCHLD > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGCHLD
# endif
# if SIGCHLD < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGCHLD
# endif
#endif

#if defined(SIGTTIN)
# if SIGTTIN > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGTTIN
# endif
# if SIGTTIN < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGTTIN
# endif
#endif

#if defined(SIGTTOU)
# if SIGTTOU > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGTTOU
# endif
# if SIGTTOU < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGTTOU
# endif
#endif

#if defined(SIGPOLL)
# if SIGPOLL > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGPOLL
# endif
# if SIGPOLL < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGPOLL
# endif
#endif

#if defined(SIGXCPU)
# if SIGXCPU > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGXCPU
# endif
# if SIGXCPU < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGXCPU
# endif
#endif

#if defined(SIGXFSZ)
# if SIGXFSZ > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGXFSZ
# endif
# if SIGXFSZ < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGXFSZ
# endif
#endif

#if defined(SIGVTALRM)
# if SIGVTALRM > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGVTALRM
# endif
# if SIGVTALRM < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGVTALRM
# endif
#endif

#if defined(SIGPROF)
# if SIGPROF > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGPROF
# endif
# if SIGPROF < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGPROF
# endif
#endif

#if defined(SIGUSR1)
# if SIGUSR1 > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGUSR1
# endif
# if SIGUSR1 < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGUSR1
# endif
#endif

#if defined(SIGUSR2)
# if SIGUSR2 > SIG_MAX
#  undef SIG_MAX
#  define SIG_MAX SIGUSR2
# endif
# if SIGUSR2 < SIG_MIN
#  undef SIG_MIN
#  define SIG_MIN SIGUSR2
# endif
#endif


const char* signame(int signum) noexcept
{
  static char number[10] = { '\0' };
  switch(signum)
  {
    case SIGABRT: return "SIGABRT";
    case SIGALRM: return "SIGALRM";
    case SIGBUS: return "SIGBUS";
    case SIGCHLD: return "SIGCHLD";
    case SIGCONT: return "SIGCONT";
    case SIGFPE: return "SIGFPE";
    case SIGHUP: return "SIGHUP";
    case SIGILL: return "SIGILL";
    case SIGINT: return "SIGINT";
    case SIGKILL: return "SIGKILL";
    case SIGPIPE: return "SIGPIPE";
    case SIGQUIT: return "SIGQUIT";
    case SIGSEGV: return "SIGSEGV";
    case SIGSTOP: return "SIGSTOP";
    case SIGTERM: return "SIGTERM";
    case SIGTSTP: return "SIGTSTP";
    case SIGTTIN: return "SIGTTIN";
    case SIGTTOU: return "SIGTTOU";
    case SIGUSR1: return "SIGUSR1";
    case SIGUSR2: return "SIGUSR2";
    case SIGPOLL: return "SIGPOLL";
    case SIGPROF: return "SIGPROF";
    case SIGSYS: return "SIGSYS";
    case SIGTRAP: return "SIGTRAP";
    case SIGURG: return "SIGURG";
    case SIGVTALRM: return "SIGVTALRM";
    case SIGXCPU: return "SIGXCPU";
    case SIGXFSZ: return "SIGXFSZ";
  }
  posix::sprintf(number, "%i\0", signum);
  return number;
}

int main(int argc, char *argv[]) noexcept
{
  if(argc < 3)
  {
    terminal::write("%s - %s. Signal or socket name not provided as program arguments.\n", UNIT_NAME, "FAILURE");
    return EXIT_FAILURE;
  }

  posix::Signal::EId sigid = decode_signal_name(argv[1]);

  if(posix::strcmp(argv[1], signame(sigid)))
  {
    terminal::write("%s - %s. Invalid signal name.\n", UNIT_NAME, "FAILURE");
    return EXIT_FAILURE;
  }

  for(int i = SIG_MIN; i <= SIG_MAX; ++i )
  {
    if(i == sigid)
    {
      posix::signal(sigid, [](int signum) noexcept
      {
        terminal::write("%s - %s.  Signal '%s' caught.  Exiting.\n", UNIT_NAME, "SUCCESS", signame(signum));
        Application::quit(EXIT_SUCCESS);
      });
    }
    else
    {
      posix::signal(i, [](int signum) noexcept
      {
        terminal::write("%s - %s.  Incorrect signal '%s' caught.  Exiting.\n", UNIT_NAME, "FAILURE", signame(signum));
        Application::quit(EXIT_FAILURE);
      });
    }

  }

  if(scfs_path == nullptr)
  {
    terminal::write("%s - %s: SCFS is not mounted\n", UNIT_NAME, "FAILURE");
    return EXIT_FAILURE;
  }

  Application app;
  ServerSocket server;

  std::string base = scfs_path;
  base.append("/").append(argv[2]);

  if(!server.bind(base.c_str()))
  {
    terminal::write("%s - %s: Unable to bind test provider to '%s'\n", UNIT_NAME, "FAILURE", base.c_str());
    return EXIT_FAILURE;
  }

  terminal::write("%s - %s\n", UNIT_NAME, "SUCCESS");
  return app.exec();
}
