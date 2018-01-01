#ifndef EXECUTORCORE_H
#define EXECUTORCORE_H

// STL
#include <string>
#include <unordered_map>

// PDTK
#include <object.h>
#include <cxxutils/configmanip.h>
#include <specialized/ProcessEvent.h>

// project
#include "configclient.h"
#include "executorconfigclient.h"

class ExecutorCore : public Object
{
public:
  ExecutorCore(void);

private:
  ConfigClient m_config_client;
  ExecutorConfigClient m_executor_client;

  ConfigManip m_executor_config;
  std::unordered_map<std::string, ConfigManip> m_configfiles;
};

#endif // EXECUTORCORE_H
