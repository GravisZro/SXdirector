#include "jobcontroller.h"

#include <specialized/procstat.h>

void JobController::add(pid_t parent_pid, pid_t child_pid) noexcept
{
  process_state_t data;
  if(::procstat(child_pid, data) != posix::error_response) // if the process (still) exists AND
  {
    m_pids.emplace_back(parent_pid, child_pid);
    m_procs.emplace_back(child_pid, ProcessEvent::Fork | ProcessEvent::Exit); // add as child
    Object::connect(m_procs.back().forked, this, &JobController::add);
    Object::connect(m_procs.back().exited, this, &JobController::remove);
  }
}

void JobController::remove(pid_t pid, posix::error_t exit_code) noexcept
{
  auto iter = m_pids.begin();
  while(iter != m_pids.end())
  {
    if(iter->first == pid || iter->second == pid) // if parent or child matches
      iter = m_pids.erase(iter);
    else
      ++iter;
  }

  for(auto piter = m_procs.begin(); piter != m_procs.end(); ++piter) // search all processes
    if(piter->pid() == pid) // if found process with matching PID
      { m_procs.erase(piter); break; } // erase and bail out of loop
}
