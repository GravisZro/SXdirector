#include "jobcontainer.h"

#include <put/cxxutils/translate.h>
#include <put/cxxutils/hashing.h>
#include "servicecheck.h"

JobContainer::JobContainer(const std::string& name) noexcept
  : m_name(name)
{
  Object::connect(m_waitstart.event_trigger, startSuccess); // job started properly :)
  Object::connect(m_waitexit.event_trigger , stopSuccess); // job exited properly :)
}

void JobContainer::start(milliseconds_t timeout,
                         const std::list<std::string>& services,
                         const std::unordered_map<std::string, std::string>& options) noexcept
{
  m_childproc.reset(new ChildProcess());
  JobController::add(posix::getpid(), m_childproc->processId());

  Object::disconnect(m_waitstart.event_timeout);
  Object::connect(m_waitstart.event_timeout,
                  [this, services]() noexcept // cannot guarantee 'services' won't change: copy it
                  {
                    for(const std::string& service : services) // check all services (if any)
                      if(!service_exists(service)) // ensure service exists
                        m_log << "Provider: %1\nField: %3\nError: timed out waiting for service to start\nCause: service %2 does not exist."_xlate
                              << m_name
                              << service
                              << "/Process/ProvidedServices"
                              << posix::eom; // record error
                    Object::enqueue(startFailure); // job did not start in allotted time :(
                  });

  Object::connect(m_childproc->started, [this](pid_t) noexcept { Object::enqueue_copy(state, "Initilizing"_xlate); });
  for(auto pair : options)
    m_childproc->setOption(pair.first, pair.second);

  if(m_childproc->invoke())
  {
    //display::providerStatus(config, "starting");
  }

  m_waitstart.setServices(services);
  if(!timeout) // safeguard from bad config value
    timeout = seconds(20); // 20 second timeout
  m_waitstart.setTimeout(timeout);
}

void JobContainer::stop(milliseconds_t timeout,
                        const std::list<std::string>& services,
                        posix::Signal::EId exit_signal,
                        const std::string& exit_type) noexcept
{
  uint32_t exit_type_hash = hash(exit_type);

  if(exit_type_hash == "HaltService"_hash && // if halting waits for services to disappear AND
     services.empty()) // no services are provided
    exit_type_hash = "ProcessTermination"_hash; // switch exit to waiting for the process to stop existing

  switch(exit_type_hash)
  {

    case "AssumeExit"_hash: // just send the signal and assume it exits
    {
      sendSignal(exit_signal); // send job the signal to exit
      Object::enqueue(stopSuccess); // assume success
      break;
    }

    case "HaltServices"_hash: // Wait for services to disappear and assume it exits
    {
      Object::disconnect(m_waitexit.event_timeout);
      Object::connect(m_waitexit.event_timeout,
                      [this, services]() noexcept // cannot guarantee 'services' won't change: copy it
                      {
                        for(const std::string& service : services) // check all services (if any)
                          if(service_exists(service)) // ensure service doesn't exist
                            m_log << "Provider: %1\nField: %3\nError: timed out waiting for service to end\nCause: service %2 does not exist."_xlate
                                  << m_name
                                  << service
                                  << "/Process/ProvidedServices"
                                  << posix::eom; // record error
                        Object::enqueue(stopFailure); // job did not exit in allotted time :(
                      });
      m_waitexit.setServices(services);
      m_waitexit.setTimeout(timeout ? timeout : seconds(10)); // start timer (10 seconds if 0 value)
      sendSignal(exit_signal); // send job the signal to exit
      break;
    }

    default: // Unexpected value! Default to waiting for process to stop existing
    case "ProcessTermination"_hash: // wait for the process to stop existing
    {
      Object::disconnect(m_waitexit.event_timeout);
      Object::connect(m_waitexit.event_timeout,
                      [this]() noexcept
                      {
        // getPids()
                        std::string pidlist;
                        m_log << "Provider: %1\nField: %3\nError: timed out waiting for provider to exit\nCause: PID(s) %2 exist."_xlate
                              << m_name
                              << pidlist
                              << "/Process/ProvidedServices"
                              << posix::eom; // record error
                        Object::enqueue(stopFailure); // job did not exit in allotted time :(
                      });

      m_waitexit.setPids(getPids());
      m_waitexit.setTimeout(timeout ? timeout : seconds(10)); // start timer (10 seconds if 0 value)
      sendSignal(exit_signal); // send job the signal to exit
      break;
    }
  }
}
