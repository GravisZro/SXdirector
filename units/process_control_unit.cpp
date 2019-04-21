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

  for(int i = 0; i < 32; ++i )
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
