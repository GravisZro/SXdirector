#include "directorcore.h"

// POSIX
#include <sys/shm.h>

// POSIX++
#include <climits>

// PDTK
#include <cxxutils/syslogstream.h>
#include <cxxutils/vterm.h>
#include <specialized/procstat.h>
#include <specialized/proclist.h>

#define SHARED_MEMORY_SIZE   0x4000
#define LIST_DELIM ','

DirectorCore::DirectorCore(uid_t euid, gid_t egid, posix::fd_t shmemid) noexcept
  : m_euid(euid), m_egid(egid)
{
  if(shmemid != posix::invalid_descriptor)
  {
    char* shmem = reinterpret_cast<char*>(::shmat(shmemid, nullptr, 0)); // retrieve shared memory

    // load state from shared memory
    vfifo storage(shmem, SHARED_MEMORY_SIZE);

    static_assert(sizeof(size_t) == sizeof(std::unordered_map<std::string, JobController>::size_type), "bad size");
    static_assert(sizeof(size_t) == sizeof(std::list<std::pair<pid_t, pid_t>>::size_type), "bad size");
    std::string name;
    size_t job_count = 0;
    size_t pid_count = 0;
    pid_t parent_pid = 0;
    pid_t child_pid = 0;

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

    // get rid of shared memory
    std::memset(shmem, 0, SHARED_MEMORY_SIZE); // zero out for safety
    ::shmctl(shmemid, IPC_RMID, nullptr); // release memory
  }

  Object::connect(m_config_client.synchronized, this, &DirectorCore::reloadSettings);
  Object::connect(m_director_client.synchronized, this, &DirectorCore::reloadSettings);
}

DirectorCore::~DirectorCore(void) noexcept
{
}

const std::string& DirectorCore::getConfigData(const std::string& config, const std::string& key) const noexcept
{
  return m_director_client.get(config, key);
}

std::list<std::string> DirectorCore::getConfigList(void) const noexcept
{
  return m_director_client.listConfigs();
}


int DirectorCore::getRunlevel(const std::string& rlname) const noexcept
{
  auto iter = m_runlevel_aliases.find(rlname);
  if(iter == m_runlevel_aliases.end())
    return posix::error_response;
  return iter->second;
}

bool DirectorCore::setRunLevel(const std::string& rlname) noexcept
{
  if(getRunlevel(rlname) == posix::error_response)
    return false;

  start_stop_t ssorder = getRunlevelOrder(rlname);

  return true;
}

void DirectorCore::reloadBinary(void) noexcept
{
  int shmemid = ::shmget(IPC_PRIVATE, SHARED_MEMORY_SIZE, IPC_CREAT | SHM_R | SHM_W); // create shared memory segment
  char* shmem = reinterpret_cast<char*>(::shmat(shmemid, nullptr, 0)); // retrieve shared memory
  std::memset(shmem, 0, SHARED_MEMORY_SIZE); // zero out for safety

  // save state to shared memory
  vfifo storage(shmem, SHARED_MEMORY_SIZE);

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

  if(!posix::setegid(m_egid) || // unable to change effective group id
     !posix::seteuid(m_euid)) // unable to change effective user id
  {
    posix::syslog << posix::priority::error << "Unable to restore effective UID and effective GID." << posix::eom;
    std::exit(posix::error_t(std::errc::permission_denied));
  }

  // reload process
  process_state_t data;
  if(::procstat(::getpid(), data) == posix::success_response)
    ::execl(data.executable.c_str(), data.executable.c_str(), std::to_string(shmemid).c_str(), NULL);
  terminal::write("%s%s\n", terminal::critical, "Failed to reload Director from binary!");
  Application::quit(errno);
}


static bool starts_with(const char* seek, const std::string& str) noexcept
{
  for(const char* other = str.data(); *seek; ++seek, ++other)
    if(*seek != *other || !*other)
      return false;
  return true;
}

void DirectorCore::reloadSettings(void) noexcept
{
  if(m_config_client.isSynchronized() &&
     m_director_client.isSynchronized())
  {
    // destroy all existing data
    m_runlevel_aliases.clear();

    // initialize runlevel aliases with numeric entries
    for(uint16_t i = 0; i < 256; ++i)
      m_runlevel_aliases.emplace(std::to_string(i), uint8_t(i));

    // add custom runlevel aliases
    for(auto& pair : m_config_client.data()) // check every config client entry
      if(starts_with("/Runlevels/", pair.first) && // if this is a runlevel alias entry AND
         m_runlevel_aliases.find(pair.second) != m_runlevel_aliases.end()) // it's a valid number (0 through 255) or an existing alias
        m_runlevel_aliases.emplace(pair.first.substr(sizeof("/Runlevels/") - 1),  // add new alias (or ignore if already existing)
                                   m_runlevel_aliases[pair.second]);

    resolveDependencies();
  }
}
