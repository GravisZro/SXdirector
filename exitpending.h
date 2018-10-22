#ifndef EXITPENDING_H
#define EXITPENDING_H

// STL
#include <set>
#include <string>

// PDTK
#include <object.h>
#include <specialized/TimerEvent.h>

class ExitPending : public Object
{
public:
  ExitPending(std::set<std::string> services) noexcept;
  ~ExitPending(void) = default;

  bool setTimeout(microseconds_t timeout) noexcept
    { return m_timer.start(timeout); }

  signal<> timeout;
  signal<> exited;
private:
  bool still_exist(void) noexcept;
  std::set<std::string> m_services;
  TimerEvent m_timer;
};

#endif // EXITPENDING_H
