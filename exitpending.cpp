#include "exitpending.h"

// POSIX++
#include <climits>

// PDTK
#include <cxxutils/mountpoint_helpers.h>
#include <specialized/procstat.h>


ExitPending::ExitPending(void) noexcept
{
  Object::connect(m_timer.expired,
                  Object::fslot_t<void>([this]() noexcept
                    { Object::enqueue(still_exist() ? timeout : exited); }));
}

// test if they exist or not
bool ExitPending::still_exist(void) noexcept
{
  char path[PATH_MAX];
  if(!m_services.empty())
  {
    if(scfs_path == nullptr)
      initialize_paths();
    for(const std::string& service : m_services)
    {
      std::snprintf(path, sizeof(path), "%s/%s", scfs_path, service.c_str());
      if(posix::access(path, F_OK) == posix::success_response)
        return true;
    }
  }
  else if (m_pids.empty())
  {
    struct process_state_t state;
    for(const std::pair<pid_t, pid_t>& pid_pair : m_pids)
      if(procstat(pid_pair.second, state) == posix::success_response &&
         state.state != Zombie)
        return true;
  }
  return false;
}
