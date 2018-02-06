#ifndef EXECUTORCORE_H
#define EXECUTORCORE_H

// STL
#include <string>
#include <unordered_map>
#include <map>
#include <set>
#include <memory>
#include <bitset>

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

  void reloadBinary  (void) noexcept;
  void reloadSettings(void) noexcept;
private:
  std::map<std::string, uint8_t> m_runlevel_aliases;
  struct depnode_t
  {
    std::string daemon_name;
    std::list<std::string> service_names;
    std::list<std::shared_ptr<depnode_t>> service_active;
    std::list<std::shared_ptr<depnode_t>> service_inactive;
    std::list<std::shared_ptr<depnode_t>> daemon_active;
    std::list<std::shared_ptr<depnode_t>> daemon_inactive;
    std::bitset<0x0100> runlevel_active;
    std::bitset<0x0100> runlevel_inactive;
  };
  std::map<std::string, std::shared_ptr<depnode_t>> m_dep_by_daemon;
  std::map<std::string, std::shared_ptr<depnode_t>> m_dep_by_service;
  std::map<uint8_t, std::set<std::shared_ptr<depnode_t>>> m_dep_by_runlevel;

  std::unordered_map<std::string, JobController> m_process_map; // indexed by daemon name
  ConfigClient m_config_client;
  ExecutorConfigClient m_executor_client;
};

#endif // EXECUTORCORE_H
