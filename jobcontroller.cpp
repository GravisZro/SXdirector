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

    Object::connect(m_procs.back().exited,
                    Object::fslot_t<void, pid_t, posix::error_t>([this](pid_t pid, int rval) noexcept
                      { remove(pid); if(m_procs.empty()) { Object::enqueue(exited, rval); } }));

    Object::connect(m_procs.back().killed,
                    Object::fslot_t<void, pid_t, posix::signal::EId>([this](pid_t pid, int rval) noexcept
                      { remove(pid); if(m_procs.empty()) { Object::enqueue(exited, rval); } }));
  }
}

void JobController::remove(pid_t pid) noexcept
{
  auto iter = m_pids.begin();
  while(iter != m_pids.end())
  {
    if(iter->second == pid) // if child matches
      iter = m_pids.erase(iter); // erase entry for parent and child
    else if(iter->first == pid) // if parent matches
    {
      m_pids.emplace_front(0, iter->second); // reparent to preserve child
      iter = m_pids.erase(iter); // erase entry
    }
    else
      ++iter;
  }

  for(auto piter = m_procs.begin(); piter != m_procs.end(); ++piter) // search all processes
    if(piter->pid() == pid) // if found process with matching PID
      { m_procs.erase(piter); break; } // erase and bail out of loop
}

bool JobController::sendSignal(posix::signal::EId signum) noexcept
{
  bool sent = true;
  for(auto& proc : m_procs)
    sent &= posix::signal::send(proc.pid(), signum);
  return sent;
}
