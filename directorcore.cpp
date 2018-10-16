#include "directorcore.h"

// POSIX
#include <sys/shm.h> // shared memory

// POSIX++
#include <climits>

// STL
#include <string>

// PDTK
#include <cxxutils/syslogstream.h>
#include <cxxutils/vterm.h>
#include <specialized/procstat.h>
#include <specialized/proclist.h>

// Director
#include "string_helpers.h"

#include <cassert>

static_assert(sizeof(size_t) == sizeof(std::unordered_map<std::string, JobController>::size_type), "bad size");
static_assert(sizeof(size_t) == sizeof(std::list<std::pair<pid_t, pid_t>>::size_type), "bad size");

DirectorCore::DirectorCore(uid_t euid, gid_t egid, posix::fd_t fd_num) noexcept
  : m_euid(euid), m_egid(egid)
{
  if(fd_num == posix::invalid_descriptor) // initial run
  {
    // search existing processes for those we should be managing (not 100% foolproof)
    process_state_t state;
    std::vector<pid_t> pidlist;
    const pid_t thispid = ::getpid();
    if(proclist(pidlist) == posix::success_response)
      for(pid_t pid : pidlist)
      {
        std::memset(reinterpret_cast<void*>(&state), 0, sizeof(state));
        if(procstat(pid, state) == posix::success_response && // if get process state works AND
           (!state.parent_process_id || state.parent_process_id == thispid)) // process has unknown parent process OR director is the parent process
          for(const std::string& config : m_director_config_client.listConfigs()) // try each config
            if(m_director_config_client.get(config, "/Process/Executable") == state.executable) // if the executable matches
            {
              m_process_map[config].add(thispid, pid); // claim this as a managed process
              std::memset(reinterpret_cast<void*>(&state), 0, sizeof(state));
              for(pid_t childpid : pidlist)
                if(procstat(childpid, state) == posix::success_response && // if get process state works AND
                   state.parent_process_id == pid) // process is a child of this parent pid
                  m_process_map[config].add(pid, childpid); // claim this as a managed process
            }
      }
  }
  else // reloaded
  {
    posix::size_t buffer_size = 0;
    char* reload_buffer = nullptr;

    reload_buffer = reinterpret_cast<char*>(::shmat(fd_num, nullptr, 0)); // retrieve shared memory
    if(reload_buffer == nullptr)
      posix::syslog << posix::priority::error
                    << "Unable to allocate shared memory for reload buffer: %1"
                    << std::strerror(errno)
                    << posix::eom;
    else
    {
      shmid_ds stat_data;
      std::memset(&stat_data, 0, sizeof(stat_data));
      if(::shmctl(fd_num, IPC_STAT, &stat_data) == posix::success_response)
        buffer_size = stat_data.shm_segsz;
    }

    if(reload_buffer != nullptr && buffer_size)
    {
      std::string name;
      size_t job_count = 0;
      size_t pid_count = 0;
      pid_t parent_pid = 0;
      pid_t child_pid = 0;

      // load state from memory
      vfifo storage(reload_buffer, posix::ssize_t(buffer_size)); // give it the shared memory buffer
      storage.expand(posix::ssize_t(buffer_size)); // tell it the buffer is full

      storage >> m_runlevel;
      storage >> job_count;
      for(size_t i = 0; i < job_count; ++i)
      {
        storage >> name >> pid_count;
        auto proc = m_process_map[name];
        for(size_t j = 0; j < pid_count; ++j)
        {
          storage >> parent_pid >> child_pid;
          proc.add(parent_pid, child_pid);
        }
      }
      std::memset(reload_buffer, 0, buffer_size); // zero out memory for safety
    }

    ::shmctl(fd_num, IPC_RMID, nullptr); // release memory
  }

  Object::connect(m_config_client.synchronized, this, &DirectorCore::reloadSettings);
  Object::connect(m_director_config_client.synchronized, this, &DirectorCore::reloadSettings);
}

DirectorCore::~DirectorCore(void) noexcept
{
}

const std::string& DirectorCore::getConfigData(const std::string& config, const std::string& key) const noexcept
{
  return m_director_config_client.get(config, key);
}

std::list<std::string> DirectorCore::getConfigList(void) const noexcept
{
  return m_director_config_client.listConfigs();
}


DependencySolver::runlevel_t DirectorCore::getRunlevelNumber(const std::string& rlname) const noexcept
{
  auto iter = m_runlevel_aliases.find(rlname);
  if(iter == m_runlevel_aliases.end())
    return invalid_runlevel;
  return iter->second;
}

bool DirectorCore::setRunlevel(const std::string& rlname) noexcept
{
  if(!m_action_queue.empty())
    return false;

  runlevel_t rlnum = getRunlevelNumber(rlname);
  if(rlnum == invalid_runlevel)
    return false;

  m_action_queue = getRunlevelOrder(rlname);

  if(rlnum == getRunlevelNumber("bootstrap"))
  {
    posix::error_t val = posix::success_response;
    Object::singleShot(&m_director_config_client, &DirectorConfigClient::resync, val);
    Object::singleShot(&m_config_client, &ConfigClient::resync, val);
  }

  m_runlevel = rlname;
  return true;
}

void DirectorCore::processJob(void) noexcept
{
  if(!m_action_queue.empty())
  {
    const std::pair<bool, std::string>& pair = m_action_queue.front();
    const bool& start = pair.first;
    const std::string& config = pair.second;

    if(start)
    {
      const std::string& executable = m_director_config_client.get(config, "/Process/Executable");
    }
    else
    {
      auto iter = m_process_map.find(config);
      if(iter != m_process_map.end())
      {
        JobController& job = iter->second;
        job.sendSignal(decode_signal_name(m_director_config_client.get(config, "/Exiting/Signal")));

        m_director_config_client.get(config, "/Exiting/Timeout");
        std::set<std::string> services = clean_explode(m_director_config_client.get(config, "/Process/ProvidedServices"), LIST_DELIM);

      }
    }
  }
}


void DirectorCore::reloadBinary(void) noexcept
{
  // buffer size calculation
  posix::size_t buffer_size = 4 + m_runlevel.size() +
                              4 + sizeof(size_t);
  for(auto& pair : m_process_map)
    buffer_size += 4 + pair.first.size() + // string
                   4 + sizeof(size_t) + // list size
                   (4 * pair.second.list().size() * 2 * sizeof(pid_t)); // list of pairs
  // end buffer size calculation

  posix::fd_t fd_num = ::shmget(IPC_PRIVATE, buffer_size, IPC_CREAT | SHM_R | SHM_W); // create shared memory segment
  char* reload_buffer = reinterpret_cast<char*>(::shmat(fd_num, nullptr, 0)); // retrieve shared memory
  if(reload_buffer == nullptr)
    posix::syslog << posix::priority::error
                  << "Unable to allocate shared memory for reload buffer: %1"
                  << std::strerror(errno)
                  << posix::eom;
  else
  {
    std::memset(reload_buffer, 0, buffer_size); // zero out for safety
    // save state to shared memory
    vfifo storage(reload_buffer, posix::ssize_t(buffer_size)); // give it the shared memory buffer

    storage << m_runlevel;
    storage << m_process_map.size();
    for(auto& pair : m_process_map)
    {
      storage << pair.first;
      auto list = pair.second.list();
      storage << list.size();
      for(auto& pair : list)
        storage << pair.first << pair.second;
    }
  }

  if(!posix::setegid(m_egid) || // unable to change effective group id
     !posix::seteuid(m_euid)) // unable to change effective user id
  {
    posix::syslog << posix::priority::error
                  << "Unable to restore effective UID and effective GID."
                  << posix::eom;
    Application::quit(posix::error_t(std::errc::permission_denied));
  }
  else
  {
    // reload process
    process_state_t data;
    if(::procstat(::getpid(), data) == posix::success_response)
      ::execl(data.executable.c_str(), data.executable.c_str(), std::to_string(fd_num).c_str(), NULL);
    terminal::write("%s%s\n", terminal::critical, "Failed to reload Director from binary!");
    Application::quit(errno);
  }
}


inline bool starts_with(const std::string& str, const char* seek)
  { return std::memcmp(str.data(), seek, strlen(seek)) == 0; }

void DirectorCore::reloadSettings(void) noexcept
{
  if(m_config_client.isSynchronized() &&
     m_director_config_client.isSynchronized())
  {
    // destroy all existing data
    m_runlevel_aliases.clear();
     // special runlevels
    m_runlevel_aliases.emplace("bootstrap", -1);
    m_runlevel_aliases.emplace("reboot"   , -2);
    m_runlevel_aliases.emplace("halt"     , -3);

    // add custom runlevel aliases
    for(auto& pair : m_config_client.data()) // check every config client entry
    {
      if(starts_with(pair.first, "/Runlevels/")) // if this is a runlevel alias entry
      {
        runlevel_t rl = invalid_runlevel;
        auto iter = m_runlevel_aliases.find(pair.second);
        if(iter == m_runlevel_aliases.end()) // if runlevel value doesn't exist
        {
          bool numeric = true;
          for(auto c : pair.second)
            numeric &= std::isdigit(c);
          if(numeric)
           rl = runlevel_t(std::stoi(pair.second)); // attempt to convert to an unsigned number
        }
        else
          rl = iter->second;

        m_runlevel_aliases.emplace(pair.first.substr(sizeof("/Runlevels/") - 1), rl); // add new alias (or ignore if already existing)

        if(rl == invalid_runlevel)
          posix::syslog << posix::priority::warning
                        << "Runlevel alias \"%1\" is invalid because runlevel alias \"%2\" is undefined or invalid."
                        << pair.first.substr(sizeof("/Runlevels/") - 1)
                        << pair.second
                        << posix::eom;
      }
    }

    resolveDependencies();
  }

  if(m_runlevel.empty()) // runlevel is empty if the director was just just started
  {
     setRunlevel("bootstrap"); // switch to the system init runlevel
     reloadBinary();
  }
  else if(m_runlevel == "bootstrap")
    setRunlevel(m_config_client.get("/Settings/InitialRunlevel")); // switch to the initial runlevel
}
