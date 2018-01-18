#ifndef EXECUTORCORE_H
#define EXECUTORCORE_H

// STL
#include <string>
#include <unordered_map>

// PDTK
#include <object.h>
#include <process.h>
#include <cxxutils/configmanip.h>
#include <specialized/ProcessEvent.h>

// project
#include "configclient.h"
#include "executorconfigclient.h"

class ExecutorCore : public Object
{
public:
  ExecutorCore(posix::fd_t shmemid = posix::invalid_descriptor) noexcept; // take shared memory identifier from previous instance
 ~ExecutorCore(void) noexcept;

private:
  void reload(void) noexcept;
  std::unordered_map<std::string, Process*> m_process_map; // indexed by config name
  ConfigClient m_config_client;
  ExecutorConfigClient m_executor_client;
};

#endif // EXECUTORCORE_H
