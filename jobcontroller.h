#ifndef JOBCONTROLLER_H
#define JOBCONTROLLER_H

// STL
#include <list>

// PDTK
#include <object.h>
#include <specialized/ProcessEvent.h>

class JobController : public Object
{
public:
  void add(pid_t parent_pid, pid_t child_pid) noexcept;
  const std::list<std::pair<pid_t, pid_t>>& list(void) noexcept { return m_pids; }
private:
  void remove(pid_t pid, posix::error_t exit_code) noexcept;
  std::list<std::pair<pid_t, pid_t>> m_pids;
  std::list<ProcessEvent> m_procs;
};

#endif // JOBCONTROLLER_H
