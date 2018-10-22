#ifndef EXITPENDING_H
#define EXITPENDING_H

// STL
#include <set>
#include <list>
#include <string>

// PDTK
#include <object.h>
#include <specialized/TimerEvent.h>

class ExitPending : public Object
{
public:
  ExitPending(void) noexcept;
  ~ExitPending(void) noexcept = default;

  void clear(void) noexcept
    { m_services.clear(); m_pids.clear(); m_timer.stop(); }

  void setPids(const std::list<std::pair<pid_t, pid_t>>& pids) noexcept
    { clear(); m_pids = pids; }

  void setServices(const std::list<std::string>& services) noexcept
    { clear(); m_services = services; }

  bool setTimeout(microseconds_t timeout) noexcept
    { return m_timer.start(timeout); }

  signal<> timeout;
  signal<> exited;
private:
  bool still_exist(void) noexcept;
  std::list<std::pair<pid_t, pid_t>> m_pids;
  std::list<std::string> m_services;
  TimerEvent m_timer;
};

#endif // EXITPENDING_H
