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
  DirectorCore(uid_t euid, gid_t egid, posix::fd_t fd_num = posix::invalid_descriptor) noexcept; // take shared memory identifier from previous instance
  virtual ~DirectorCore(void) noexcept;

  bool        setRunlevel(const std::string& rlname) noexcept;
  std::string getRunlevel(void) const noexcept { return m_runlevel; }

  void reloadBinary  (void) noexcept;
  void reloadSettings(void) noexcept;

private:
  const std::string& getConfigData(const std::string& config, const std::string& key) const noexcept;
  std::list<std::string> getConfigList(void) const noexcept;
  runlevel_t getRunlevelNumber(const std::string& rlname) const noexcept;
  void processJob(void) noexcept;

  std::string m_runlevel;
  std::map<std::string, runlevel_t> m_runlevel_aliases;
  std::unordered_map<std::string, JobController> m_process_map; // indexed by provider name

  std::queue<std::pair<bool, std::string>> m_action_queue;

  ConfigClient m_config_client;
  DirectorConfigClient m_director_config_client;
  uid_t m_euid;
  gid_t m_egid;
};

#endif // DIRECTORCORE_H
