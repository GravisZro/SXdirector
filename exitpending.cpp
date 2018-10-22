#include "exitpending.h"

// POSIX++
#include <climits>

// PDTK
#include <cxxutils/mountpoint_helpers.h>

ExitPending::ExitPending(std::set<std::string> services) noexcept
  : m_services(services)
{
  Object::connect(m_timer.expired,
                  Object::fslot_t<void>([this]() noexcept
                    { Object::enqueue(still_exist() ? timeout : exited); }));
}

// test if they exist or not
bool ExitPending::still_exist(void) noexcept
{
  char path[PATH_MAX];
  for(const std::string& service : m_services)
  {
    std::snprintf(path, sizeof(path), "%s/%s", scfs_path, service.c_str());
    if(posix::access(path, F_OK) == posix::success_response)
      return true;
  }
  return false;
}
