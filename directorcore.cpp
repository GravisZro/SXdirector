#include "directorcore.h"

// POSIX
#include <sys/shm.h> // shared memory

// STL
#include <string>
#include <cassert>

// PUT
#include <put/cxxutils/syslogstream.h>
#include <put/cxxutils/vterm.h>
#include <put/cxxutils/hashing.h>
#include <put/cxxutils/translate.h>
#include <put/specialized/procstat.h>
#include <put/specialized/proclist.h>

// Director
#include "string_helpers.h"
#include "servicecheck.h"

static_assert(sizeof(posix::size_t) == sizeof(std::unordered_map<int, int>::size_type), "bad size");
static_assert(sizeof(posix::size_t) == sizeof(std::list<int>::size_type), "bad size");

DirectorCore::DirectorCore(uid_t euid, gid_t egid, posix::fd_t shmid) noexcept
  : m_euid(euid), m_egid(egid)
{
  if(!shmLoad(shmid)) // if loading from shared memory failed
    buildProcessMap(); // rebuild the process map from scratch

  Object::connect(m_config_client.synchronized, this, &DirectorCore::multiSyncReloadSettings); // config has been updated
  Object::connect(m_director_config_client.synchronized, this, &DirectorCore::multiSyncReloadSettings); // config has been updated
}

DirectorCore::~DirectorCore(void) noexcept
{
}

// prevent m_config_client and m_director_config_client from invoking reloadSettings() multiple times
void DirectorCore::multiSyncReloadSettings(void) noexcept
{
  ++m_synchronized_count;
  if(m_synchronized_count == 2) // ensure fully synchronized to avoid multiple reloads
  {
    m_synchronized_count = 0; // reset synch count for next sync
    reloadSettings(); // actually reload settings
  }
}

bool DirectorCore::buildProcessMap(void) noexcept
{
  // search existing processes for those we should be managing (not 100% foolproof)
  process_state_t state;
  std::set<pid_t> pidlist;
  const pid_t thispid = posix::getpid();
  if(proclist(pidlist) == posix::success_response)
  {
    for(pid_t pid : pidlist)
    {
      posix::memset(reinterpret_cast<void*>(&state), 0, sizeof(state));
      if(procstat(pid, state) && // if get process state works AND
         (!state.parent_process_id || state.parent_process_id == thispid)) // process has unknown parent process OR director is the parent process
        for(const std::string& config : getConfigList()) // try each config
          if(getConfigValue(config, "/Process/Executable") == state.executable) // if the executable matches
          {
            m_process_map[config]->add(thispid, pid); // claim this as a managed process
            posix::memset(reinterpret_cast<void*>(&state), 0, sizeof(state)); // wipe process state for safety
            for(pid_t childpid : pidlist)
              if(procstat(childpid, state) && // if get process state works AND
                 state.parent_process_id == pid) // process is a child of this parent pid
                m_process_map[config]->add(pid, childpid); // claim this as a managed process
          }
    }
    return true;
  }
  return false;
}

posix::fd_t DirectorCore::shmStore(void) noexcept
{
  // buffer size calculation
  posix::size_t buffer_size = 4 + m_runlevel.size() +
                              4 + sizeof(posix::size_t);
  for(auto& pair : m_process_map)
    buffer_size += 4 + pair.first.size() + // string
                   4 + sizeof(posix::size_t) + // list size
                   (4 * pair.second->getPids().size() * 2 * sizeof(pid_t)); // list of pairs
  // end buffer size calculation

  posix::fd_t shmid = ::shmget(IPC_PRIVATE, buffer_size, IPC_CREAT | SHM_R | SHM_W); // create shared memory segment
  if(shmid != posix::invalid_descriptor)
  {
    char* reload_buffer = reinterpret_cast<char*>(::shmat(shmid, nullptr, 0)); // retrieve shared memory
    if(reload_buffer == reinterpret_cast<char*>(-1)) // if error
    {
      ::shmctl(shmid, IPC_RMID, nullptr); // release shared memory
      shmid = posix::invalid_descriptor; // disclaim shared memory id
    }
    else
    {
      posix::memset(reload_buffer, 0, buffer_size); // zero out for safety
      // save state to shared memory
      vfifo storage(reload_buffer, posix::ssize_t(buffer_size)); // give it the shared memory buffer

      storage << m_runlevel;
      storage << m_process_map.size();
      for(auto& pair : m_process_map)
      {
        storage << pair.first;
        auto list = pair.second->getPids();
        storage << list.size();
        for(auto& pair : list)
          storage << pair.first << pair.second;
      }
    }
  }
  return shmid;
}

bool DirectorCore::shmLoad(posix::fd_t shmid) noexcept
{
  posix::size_t buffer_size = 0;
  char* reload_buffer = nullptr;

  if(shmid != posix::invalid_descriptor)
    reload_buffer = reinterpret_cast<char*>(::shmat(shmid, nullptr, 0)); // retrieve shared memory
  if(reload_buffer != nullptr &&
     reload_buffer != reinterpret_cast<char*>(-1)) // if not an error
  {
    shmid_ds stat_data;
    posix::memset(&stat_data, 0, sizeof(stat_data));
    if(::shmctl(shmid, IPC_STAT, &stat_data) == posix::success_response)
    {
      buffer_size = stat_data.shm_segsz;
      std::string name;
      posix::size_t job_count = 0;
      posix::size_t pid_count = 0;
      pid_t parent_pid = 0;
      pid_t child_pid = 0;

      // load state from memory
      vfifo storage(reload_buffer, posix::ssize_t(buffer_size)); // give it the shared memory buffer
      storage.expand(posix::ssize_t(buffer_size)); // tell "storage" that the buffer is full

      storage >> m_runlevel;
      storage >> job_count;
      for(posix::size_t i = 0; i < job_count; ++i)
      {
        storage >> name >> pid_count;
        std::shared_ptr<JobContainer> proc = m_process_map[name];
        for(posix::size_t j = 0; j < pid_count; ++j)
        {
          storage >> parent_pid >> child_pid;
          proc->add(parent_pid, child_pid);
        }
      }
      posix::memset(reload_buffer, 0, buffer_size); // zero out shared memory for safety
    }
    ::shmctl(shmid, IPC_RMID, nullptr); // release shared memory
    return true;
  }
  return false;
}

void DirectorCore::reloadBinary(void) noexcept
{
  posix::fd_t shmid = shmStore();
  if(shmid == posix::invalid_descriptor)
    posix::syslog << posix::priority::error
                  << "Unable to allocate shared memory for reload buffer: %1"
                  << posix::strerror(errno)
                  << posix::eom;


  if(!posix::setegid(m_egid) || // unable to change effective group id
     !posix::seteuid(m_euid)) // unable to change effective user id
  {
    posix::syslog << posix::priority::error
                  << "Unable to restore effective UID and effective GID."
                  << posix::eom;
    Application::quit(posix::error_t(posix::errc::permission_denied));
  }
  else
  {
    // reload process
    process_state_t data;
    if(procstat(posix::getpid(), data))
      posix::execl(data.executable.c_str(), data.executable.c_str(), std::to_string(shmid).c_str(), NULL);
    terminal::write("%s%s\n", terminal::critical, "Failed to reload Director from binary!");
    Application::quit(errno);
  }
}


inline bool starts_with(const std::string& str, const char* seek)
  { return posix::memcmp(str.data(), seek, strlen(seek)) == 0; }

void DirectorCore::reloadSettings(void) noexcept
{
  if(m_config_client.isSynchronized() && // ensure fully synchronized to avoid multiple reloads
     m_director_config_client.isSynchronized())
  {
    // replace existing runlevels with only special runlevels
    m_runlevel_aliases = { {"bootstrap", -1},
                           {"reboot"   , -2},
                           {"halt"     , -3},
                           {"poweroff" , -4} };

    // add custom runlevel aliases
    for(const std::pair<std::string, std::string>& pair : m_config_client.data()) // check every config client entry
    {
      if(starts_with(pair.first, "/Runlevels/")) // if this is a runlevel alias entry
      {
        runlevel_t rl = invalid_runlevel;
        // std::map<std::string, runlevel_t>::const_iterator iter =
        auto iter = m_runlevel_aliases.find(pair.second);
        if(iter == m_runlevel_aliases.end()) // if runlevel value doesn't exist
          rl = convert_to_runlevel(pair.second, invalid_runlevel); // convert string value to runlevel value (if possible)
        else // if runlevel value already exists
          rl = iter->second; // copy value

        m_runlevel_aliases.emplace(pair.first.substr(sizeof("/Runlevels/") - 1), rl); // add new alias (or ignore if already existing)

        if(rl == invalid_runlevel)
          posix::syslog << posix::priority::warning
                        << "Runlevel alias \"%1\" is invalid because \"%2\" is neither an existing runlevel alias nor a valid positive 16-bit integer."
                        << pair.first.substr(sizeof("/Runlevels/") - 1)
                        << pair.second
                        << posix::eom;
      }
    }

    resolveDependencies();

    if(!m_action_queue.empty()) // sync interrupted job queue
    {
      m_action_queue = std::queue<std::pair<bool, std::string>>(); // clear action queue
      setRunlevel(m_runlevel); // restart runlevel change
    }
    else if(m_runlevel.empty()) // runlevel is empty if the director was just just started
    {
      Object::connect(runlevel_changed,
                      [this](const std::string&) noexcept { reloadBinary(); });
      assert(setRunlevel("bootstrap")); // switch to the system init runlevel
    }
    else if(m_runlevel == "bootstrap")
      setRunlevel(m_config_client.get("/Settings/InitialRunlevel")); // switch to the initial runlevel
  }
}


inline const std::string& DirectorCore::getConfigValue(const std::string& config, const std::string& key) const noexcept
{
  return m_director_config_client.get(config, key);
}

inline std::list<std::string> DirectorCore::getConfigValues(const std::string& config, const std::string& key) const noexcept
{
  return clean_explode(m_director_config_client.get(config, key), LIST_DELIM);
}

inline std::list<std::string> DirectorCore::getConfigList(void) const noexcept
{
  return m_director_config_client.listConfigs();
}

inline DependencySolver::runlevel_t DirectorCore::getRunlevelNumber(const std::string& rlname) const noexcept
{
  // std::map<std::string, runlevel_t>::const_iterator iter =
  auto iter = m_runlevel_aliases.find(rlname);
  if(iter == m_runlevel_aliases.end())
    return convert_to_runlevel(rlname, invalid_runlevel); // last ditch effort to convert directly to a numeric value
  return iter->second;
}

inline const std::unordered_map<std::string, std::string>& DirectorCore::getConfigData(const std::string& config) const noexcept
{
  return m_director_config_client.data(config);
}

bool DirectorCore::setRunlevel(const std::string& rlname) noexcept
{
  if(!m_action_queue.empty()) // if still changing runlevels
    return false;

  runlevel_t rlnum = getRunlevelNumber(rlname);
  if(rlnum == invalid_runlevel || // if invalid OR
     rlnum == getRunlevelNumber(m_runlevel)) // already set
    return false;

  m_action_queue = getRunlevelOrder(rlname);
  m_runlevel = rlname;

  Object::singleShot(this, &DirectorCore::processJob);
  return true;
}

void DirectorCore::processJob(void) noexcept
{
  if(m_action_queue.empty())
  {
    terminal::write("runlevel is now: '%s'\n", m_runlevel.c_str());
    Object::enqueue(runlevel_changed, m_runlevel);
  }
  else
  {
    const std::pair<bool, std::string>& pair = m_action_queue.front();
    const bool& start = pair.first;
    const std::string& config = pair.second;

    if(getConfigData(config).empty()) // if the config file dons NOT exist
    {
      m_log << "No configuration for provider %1 was found."_xlate << config << posix::eom;
    }
    else // if the config file exists
    {
      std::list<std::string> services = getConfigValues(config, "/Process/ProvidedServices");

      if(start) // if starting provider
      {
        // std::unordered_map<std::string, std::shared_ptr<JobContainer>>::const_iterator iter =
        auto iter = m_process_map.find(config); // look to see if already started
        if(iter == m_process_map.end()) // if not already started
        {
          for(const std::string& service : getConfigValues(config, "/Requirements/ActiveServices"))
            if(!service_exists(service)) // service should exists
              m_log << "Provider: %1\nField: %3\nError: failed to start\nCause: service %2 must be active"_xlate
                    << config
                    << service
                    << "/Requirements/ActiveServices"
                    << posix::eom; // record error

          for(const std::string& service : getConfigValues(config, "/Requirements/InactiveServices"))
            if(service_exists(service)) // service should NOT exist
              m_log << "Provider: %1\nField: %3\nError: failed to start\nCause: service %2 must be inactive"_xlate
                    << config
                    << service
                    << "/Requirements/InactiveServices"
                    << posix::eom; // record error

          for(const std::string& provider : getConfigValues(config, "/Requirements/ActiveProviders"))
            if(m_process_map.find(provider) == m_process_map.end()) // provider should be running
              m_log << "Provider: %1\nField: %3\nError: failed to start\nCause: provider %2 must be active"_xlate
                    << config
                    << provider
                    << "/Requirements/ActiveProviders"
                    << posix::eom; // record error

          for(const std::string& provider : getConfigValues(config, "/Requirements/InactiveProviders"))
            if(m_process_map.find(provider) != m_process_map.end()) // provider should NOT be running
              m_log << "Provider: %1\nField: %3\nError: failed to start\nCause: provider %2 must be inactive"_xlate
                    << config
                    << provider
                    << "/Requirements/InactiveProviders"
                    << posix::eom; // record error

          if(m_log.empty()) // process requirements are satisified
          {
            // std::pair<std::unordered_map<std::string, std::shared_ptr<JobContainer>>::iterator,bool> rval =
            auto rval = m_process_map.emplace(config, std::make_shared<JobContainer>(config));
            if(rval.second)
            {
              std::shared_ptr<JobContainer> job = rval.first->second;
              Object::connect(job->startSuccess, this, &DirectorCore::processJob);
              Object::connect(job->stopSuccess , this, &DirectorCore::processJob);
              job->start(std::stoi(getConfigValue(config, "/Process/StartTimeout")),
                         services,
                         getConfigData(config));
            }
          }
        }
        else // if already started
        {
          for(const std::string& service : services) // check all services (if any)
            if(!service_exists(service)) // ensure service exists
              m_log << "Provider: %1\nField: %3\nError: not providing service\nCause: service %2 does not exist."_xlate
                    << config
                    << service
                    << "/Process/ProvidedServices"
                    << posix::eom; // record error

          if(m_log.empty()) // if service sockets extant (if have services)
            Object::singleShot(this, &DirectorCore::jobDone); // job is done
        }
      }
      else // if stopping provider
      {
        auto iter = m_process_map.find(config);
        if(iter != m_process_map.end())
        {
          std::shared_ptr<JobContainer> job = iter->second;
          job->stop(std::stoi(getConfigValue(config, "/Exiting/Timeout")),
                    services,
                    decode_signal_name(getConfigValue(config, "/Exiting/Signal")),
                    getConfigValue(config, "/Exiting/ExitWaitType"));
        }
      }
    }

    if(!m_log.empty()) // if there were arrors
      Object::singleShot(this, &DirectorCore::jobStuck); // job is stuck
  }
}

void DirectorCore::jobDone(void) noexcept
{
  const std::pair<bool, std::string>& pair = m_action_queue.front(); // get the job that just finished
  const bool& start = pair.first;
  const std::string& config = pair.second;
  //display::providerStatus(config, start ? "active" : "stopped");
  if(!start) // if job is ending a process
    m_process_map.erase(config); // remove dead process
  m_action_queue.pop(); // action has been fulfilled
  Object::singleShot(this, &DirectorCore::processJob); // start the next job
}

void DirectorCore::jobStuck(void) noexcept
{
  const std::pair<bool, std::string>& pair = m_action_queue.front(); // get the job that just finished
  const bool& start = pair.first;
  const std::string& config = pair.second;

//display::providerStatus(config, "error");
  for(const std::string& message : m_log.messages())
  {

  }

  m_log.clear();
}
