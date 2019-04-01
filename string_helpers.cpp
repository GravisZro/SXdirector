#include "string_helpers.h"

// POSIX++
#include <cstring>
#include <climits>

// PUT
#include <put/cxxutils/hashing.h>


std::list<std::string> clean_explode(const std::string& str, char delim) noexcept
{
  std::list<std::string> strs;
  std::string newstr;
  newstr.reserve(NAME_MAX);

  for(const auto& character : str)
  {
    if(character == delim)
    {
      if(!newstr.empty())
      {
        strs.emplace_back(newstr);
        newstr.clear();
      }
    }
    else if(std::isgraph(character))
      newstr.push_back(character);
  }

  if(!newstr.empty())
    strs.emplace_back(newstr);

  return strs;
}


int16_t convert_to_runlevel(const std::string& str, int16_t invalid_value)
{
  bool numeric = true; // for keeping track if every character is a digit
  int rl = 0;
  for(char c : str) // test each character to ensure this is a positive integer
    if(numeric &= posix::isdigit(c))
      rl = (rl * 10) + (c - '0');

  return (!numeric || rl < 0 || rl > INT16_MAX) ? invalid_value : rl;
}


posix::Signal::EId decode_signal_name(const std::string& signal_name) noexcept
{
  switch(hash(signal_name))
  {
    case "SIGABRT"_hash   : return posix::Signal::Abort;
    case "SIGALRM"_hash   : return posix::Signal::Timer;
    case "SIGBUS"_hash    : return posix::Signal::MemoryBusError;
    case "SIGCHLD"_hash   : return posix::Signal::ChildStatusChanged;
    case "SIGCONT"_hash   : return posix::Signal::Resume;
    case "SIGFPE"_hash    : return posix::Signal::FloatingPointException;
    case "SIGHUP"_hash    : return posix::Signal::Hangup;
    case "SIGILL"_hash    : return posix::Signal::IllegalInstruction;
    case "SIGINT"_hash    : return posix::Signal::Interrupt;
    case "SIGKILL"_hash   : return posix::Signal::Kill;
    case "SIGPIPE"_hash   : return posix::Signal::BrokenPipe;
    case "SIGQUIT"_hash   : return posix::Signal::Quit;
    case "SIGSEGV"_hash   : return posix::Signal::SegmentationViolation;
    case "SIGSTOP"_hash   : return posix::Signal::Stop;
    case "SIGTERM"_hash   : return posix::Signal::Terminate;
    case "SIGTSTP"_hash   : return posix::Signal::KeyboardStop;
    case "SIGTTIN"_hash   : return posix::Signal::TTYRead;
    case "SIGTTOU"_hash   : return posix::Signal::TTYWrite;
    case "SIGUSR1"_hash   : return posix::Signal::UserSignal1;
    case "SIGUSR2"_hash   : return posix::Signal::UserSignal2;
    case "SIGPOLL"_hash   : return posix::Signal::Poll;
    case "SIGPROF"_hash   : return posix::Signal::ProfilingTimer;
    case "SIGSYS"_hash    : return posix::Signal::BadSystemCall;
    case "SIGTRAP"_hash   : return posix::Signal::TraceTrap;
    case "SIGURG"_hash    : return posix::Signal::Urgent;
    case "SIGVTALRM"_hash : return posix::Signal::VirtualTimer;
    case "SIGXCPU"_hash   : return posix::Signal::LimitExceededCPU;
    case "SIGXFSZ"_hash   : return posix::Signal::LimitExceededFile;
  }
  return posix::Signal::Quit;
}
