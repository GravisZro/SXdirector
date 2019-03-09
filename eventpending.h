#ifndef EXITPENDING_H
#define EXITPENDING_H

// STL
#include <set>
#include <list>
#include <string>

// PDTK
#include <object.h>
#include <specialized/timerevent.h>

class EventPending : public Object
{
public:
  EventPending(void) noexcept;
  virtual ~EventPending(void) noexcept { }

  bool setTimeout(milliseconds_t timeout) noexcept;

  signal<> event_timeout;
  signal<> event_trigger;
protected:
  virtual bool activateTrigger(void) noexcept = 0;
private:
  void timerExpired(void) noexcept;
  TimerEvent m_timer;
  milliseconds_t m_timeout_count;
  milliseconds_t m_max_timeout_count;
};

class ExitPending : public EventPending
{
public:
  ExitPending (void) noexcept { }
  ~ExitPending(void) noexcept { }

  void setPids(const std::list<std::pair<pid_t, pid_t>>& pids) noexcept
    { m_services.clear(); m_pids = pids; }

  void setServices(const std::list<std::string>& services) noexcept
    { m_pids.clear(); m_services = services; }

private:
  bool activateTrigger(void) noexcept;
  std::list<std::pair<pid_t, pid_t>> m_pids;
  std::list<std::string> m_services;
};

class StartPending : public EventPending
{
public:
  StartPending (void) noexcept { }
  ~StartPending(void) noexcept { }

  void setServices(const std::list<std::string>& services) noexcept
    { m_services = services; }

private:
  bool activateTrigger(void) noexcept;
  std::list<std::string> m_services;
};

#endif // EXITPENDING_H
