#ifndef EXECUTORCORE_H
#define EXECUTORCORE_H

// STL
#include <string>
#include <unordered_map>

// PDTK
#include <object.h>

// project
#include "jobcontroller.h"
#include "configclient.h"
#include "executorconfigclient.h"

class ExecutorCore : public Object
{
public:
  ExecutorCore(posix::fd_t shmemid = posix::invalid_descriptor) noexcept; // take shared memory identifier from previous instance
 ~ExecutorCore(void) noexcept;

  void reloadSettings(void) noexcept;
private:
  void reloadBinary  (void) noexcept;
  std::unordered_map<std::string, JobController> m_process_map; // indexed by config name
  ConfigClient m_config_client;
  ExecutorConfigClient m_executor_client;
};

#endif // EXECUTORCORE_H
