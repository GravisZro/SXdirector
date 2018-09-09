#ifndef DIRECTORCORE_H
#define DIRECTORCORE_H

// STL
#include <string>
#include <unordered_map>
#include <map>

// PDTK
#include <object.h>

// project
#include "jobcontroller.h"
#include "configclient.h"
#include "directorconfigclient.h"
#include "dependencysolver.h"

class DirectorCore : public Object,
                     public DependencySolver
{
public:
  DirectorCore(uid_t euid, gid_t egid, posix::fd_t shmemid = posix::invalid_descriptor) noexcept; // take shared memory identifier from previous instance
  virtual ~DirectorCore(void) noexcept;

  bool    setRunLevel(const std::string& rlname) noexcept;
  uint8_t getRunlevel(void) const noexcept { return m_runlevel; }

  void reloadBinary  (void) noexcept;
  void reloadSettings(void) noexcept;

private:
  uint8_t m_runlevel;
  std::map<std::string, uint8_t> m_runlevel_aliases;
  std::unordered_map<std::string, JobController> m_process_map; // indexed by daemon name

  const std::string& getConfigData(const std::string& config, const std::string& key) const noexcept;
  std::list<std::string> getConfigList(void) const noexcept;
  int getRunlevel(const std::string& rlname) const noexcept;

  ConfigClient m_config_client;
  DirectorConfigClient m_director_config_client;
  uid_t m_euid;
  gid_t m_egid;
};

#endif // DIRECTORCORE_H
