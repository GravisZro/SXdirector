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
#include "exitpending.h"
#include "directorconfigclient.h"
#include "dependencysolver.h"


class DirectorCore : public Object,
                     public DependencySolver
{
public:
  DirectorCore(uid_t euid, gid_t egid, posix::fd_t shmid = posix::invalid_descriptor) noexcept; // take shared memory identifier from previous instance
  virtual ~DirectorCore(void) noexcept = default;

  bool        setRunlevel(const std::string& rlname) noexcept;
  std::string getRunlevel(void) const noexcept { return m_runlevel; }

  void reloadBinary  (void) noexcept;
  void reloadSettings(void) noexcept;

private:
// implemented for DependencySolver
  virtual const std::string& getConfigData(const std::string& config, const std::string& key) const noexcept;
  virtual std::list<std::string> getConfigList(void) const noexcept;
  virtual runlevel_t getRunlevelNumber(const std::string& rlname) const noexcept;

// signals
  signal<const std::string> runlevel_changed;

// functions
  bool buildProcessMap(void) noexcept;
  posix::fd_t shmStore(void) noexcept;
  bool shmLoad(posix::fd_t shmid) noexcept;
  void processJob(void) noexcept;
  void jobDone(void) noexcept;
  void jobStuck(void) noexcept;

// variables
  std::string m_runlevel;
  std::map<std::string, runlevel_t> m_runlevel_aliases;
  std::unordered_map<std::string, JobController> m_process_map; // indexed by provider name

  std::queue<std::pair<bool, std::string>> m_action_queue; // bool (start/stop) + name

  void multiSyncReloadSettings(void) noexcept;
  uint8_t m_synchronized_count;
  ConfigClient m_config_client;
  DirectorConfigClient m_director_config_client;
  uid_t m_euid;
  gid_t m_egid;
  ExitPending m_waitexit;
};

#endif // DIRECTORCORE_H
