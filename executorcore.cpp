#include "executorcore.h"

// PDTK
#include <cxxutils/syslogstream.h>
#include <cxxutils/vterm.h>
#include <specialized/procstat.h>
#include <specialized/proclist.h>


ExecutorCore::ExecutorCore(void)
{
  posix::success();
  std::vector<pid_t> list;
  if(::proclist(list) == posix::success_response)
  {
    for(pid_t pid : list)
    {
      process_state_t data;
      if(::procstat(pid, data) == posix::success_response)
      {
        if(!data.executable.empty())
          terminal::write("pid: %i - '%s'\n", pid, data.executable.data());
//          terminal::write("pid: %i - %s\n", pid, data.name.data());
//        else

      }
      else
        terminal::write("failed: %i - %s\n", pid, std::strerror(errno));
    }
  }
  Application::quit(0);
}
