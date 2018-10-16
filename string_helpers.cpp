#include "string_helpers.h"

// POSIX++
#include <cstring>
#include <climits>

// PDTK
#include <cxxutils/hashing.h>


std::set<std::string> clean_explode(const std::string& str, char delim) noexcept
{
  std::set<std::string> strs;
  std::string newstr;
  newstr.reserve(NAME_MAX);

  for(const auto& character : str)
  {
    if(character == delim)
    {
      if(!newstr.empty())
      {
        strs.emplace(newstr);
        newstr.clear();
      }
    }
    else if(std::isgraph(character))
      newstr.push_back(character);
  }

  if(!newstr.empty())
    strs.emplace(newstr);

  return strs;
}


posix::signal::EId decode_signal_name(const std::string& signal_name) noexcept
{
  switch(hash(signal_name))
  {
    case "SIGABRT"_hash   : return posix::signal::Abort;
    case "SIGALRM"_hash   : return posix::signal::Timer;
    case "SIGBUS"_hash    : return posix::signal::MemoryBusError;
    case "SIGCHLD"_hash   : return posix::signal::ChildStatusChanged;
    case "SIGCONT"_hash   : return posix::signal::Resume;
    case "SIGFPE"_hash    : return posix::signal::FloatingPointException;
    case "SIGHUP"_hash    : return posix::signal::Hangup;
    case "SIGILL"_hash    : return posix::signal::IllegalInstruction;
    case "SIGINT"_hash    : return posix::signal::Interrupt;
    case "SIGKILL"_hash   : return posix::signal::Kill;
    case "SIGPIPE"_hash   : return posix::signal::BrokenPipe;
    case "SIGQUIT"_hash   : return posix::signal::Quit;
    case "SIGSEGV"_hash   : return posix::signal::SegmentationViolation;
    case "SIGSTOP"_hash   : return posix::signal::Stop;
    case "SIGTERM"_hash   : return posix::signal::Terminate;
    case "SIGTSTP"_hash   : return posix::signal::KeyboardStop;
    case "SIGTTIN"_hash   : return posix::signal::TTYRead;
    case "SIGTTOU"_hash   : return posix::signal::TTYWrite;
    case "SIGUSR1"_hash   : return posix::signal::UserSignal1;
    case "SIGUSR2"_hash   : return posix::signal::UserSignal2;
    case "SIGPOLL"_hash   : return posix::signal::Poll;
    case "SIGPROF"_hash   : return posix::signal::ProfilingTimer;
    case "SIGSYS"_hash    : return posix::signal::BadSystemCall;
    case "SIGTRAP"_hash   : return posix::signal::TraceTrap;
    case "SIGURG"_hash    : return posix::signal::Urgent;
    case "SIGVTALRM"_hash : return posix::signal::VirtualTimer;
    case "SIGXCPU"_hash   : return posix::signal::LimitExceededCPU;
    case "SIGXFSZ"_hash   : return posix::signal::LimitExceededFile;
  }
  return posix::signal::Quit;
}
