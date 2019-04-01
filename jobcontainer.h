#ifndef JOBCONTAINER_H
#define JOBCONTAINER_H

#include <memory>

#include <put/childprocess.h>
#include <put/cxxutils/syslogstream.h>

#include "jobcontroller.h"
#include "eventpending.h"

class JobContainer : public JobController
{
public:
  JobContainer(const std::string& name) noexcept;
  ~JobContainer(void) noexcept = default;

  void start(milliseconds_t timeout,
             const std::list<std::string>& services,
             const std::unordered_map<std::string, std::string>& options) noexcept;

  void stop (milliseconds_t timeout,
             const std::list<std::string>& services,
             posix::Signal::EId exit_signal,
             const std::string& exit_type) noexcept;

  ErrorLogStream log(void) const { return m_log; }

  signal<const char*> state;

  signal<> startFailure;
  signal<> startSuccess;

  signal<> stopFailure;
  signal<> stopSuccess;

private:
  const std::string& m_name;
  ErrorLogStream m_log;
  std::unique_ptr<ChildProcess> m_childproc;
  ExitPending  m_waitexit;
  StartPending m_waitstart;
};

#endif // JOBCONTAINER_H
