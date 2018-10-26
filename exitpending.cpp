#include "exitpending.h"

// POSIX++
#include <climits>

// PDTK
#include <cxxutils/mountpoint_helpers.h>
#include <specialized/procstat.h>

EventPending::EventPending(void) noexcept
  : m_timeout_count(0), m_max_timeout_count(0)
{
  Object::connect(m_timer.expired, this, &EventPending::timerExpired);
}

void EventPending::timerExpired(void) noexcept
{
  if(activateTrigger())
  {
    Object::enqueue(event_trigger);
    m_timer.stop();
  }
  else if(++m_timeout_count >= m_max_timeout_count) // increment and check if timeout count has been met
  {
    Object::enqueue(event_timeout);
    m_timer.stop();
  }
}

// Ensure the timer doesn't check to frequently or infrequently
bool EventPending::setTimeout(microseconds_t timeout) noexcept
{
  if(timeout > seconds(10)) // if timeout is longer than 10 seconds
  {
    m_timeout_count = 0;
    m_max_timeout_count = timeout / seconds(1);
    timeout = seconds(1);
  }
  else if(timeout > seconds(1) / 10) // if timeout is longer than 1/10 of a second
  {
    m_timeout_count = 0;
    m_max_timeout_count = 10;
    timeout /= 10;
  }
  else // if timeout is shorter than 1/10 of a second
  {
    m_timeout_count = 0;
    m_max_timeout_count = 1;
  }

  return m_timer.start(timeout, timeout);
}

// test if they exist or not
bool ExitPending::activateTrigger(void) noexcept
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
        return false;
    }
  }
  else if(m_pids.empty())
  {
    struct process_state_t state;
    for(const std::pair<pid_t, pid_t>& pid_pair : m_pids)
      if(procstat(pid_pair.second, state) == posix::success_response &&
         state.state != Zombie)
        return false;
  }
  return true;
}

// test if they exist or not
bool StartPending::activateTrigger(void) noexcept
{
  char path[PATH_MAX];
  if(scfs_path == nullptr)
    initialize_paths();
  for(const std::string& service : m_services)
  {
    std::snprintf(path, sizeof(path), "%s/%s", scfs_path, service.c_str());
    if(posix::access(path, F_OK) != posix::success_response)
      return false;
  }
  return true;
}
