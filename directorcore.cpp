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

void DirectorCore::reloadBinary(void) noexcept
{
  int shmemid = ::shmget(IPC_PRIVATE, SHARED_MEMORY_SIZE, IPC_CREAT | SHM_R | SHM_W); // create shared memory segment
  char* shmem = reinterpret_cast<char*>(::shmat(shmemid, nullptr, 0)); // retrieve shared memory
  std::memset(shmem, 0, SHARED_MEMORY_SIZE); // zero out for safety

  // save state to shared memory
  vfifo storage(shmem, SHARED_MEMORY_SIZE);

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

static std::list<std::string> clean_explode(const std::string& str, char delim)
{
  std::list<std::string> strs;
  std::string newstr;
  newstr.reserve(NAME_MAX);

  for(auto& character : str)
  {
    if(character == delim)
    {
      if(!newstr.empty())
      {
        strs.push_back(newstr);
        newstr.clear();
      }
    }
    else if(std::isgraph(character))
      newstr.push_back(character);
  }

  if(!newstr.empty())
    strs.push_back(newstr);

  return strs;
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
    m_dep_by_daemon.clear();
    m_dep_by_runlevel.clear();
    m_dep_by_service.clear();
    m_runlevel_aliases.clear();

    // initialize runlevel aliases with numeric entries
    for(uint16_t i = 0; i < 256; ++i)
      m_runlevel_aliases.emplace(std::to_string(i), uint8_t(i));

    // add custom runlevel aliases
    for(auto& pair : m_config_client.data()) // check every config client entry
      if(starts_with("/RunlevelAliases/", pair.first) && // if this is a runlevel alias entry AND
         m_runlevel_aliases.find(pair.second) != m_runlevel_aliases.end()) // it's a valid number (0 through 255) or an existing alias
        m_runlevel_aliases.emplace(pair.first.substr(sizeof("/RunlevelAliases/") - 1),  // add new alias (or ignore if already existing)
                                   m_runlevel_aliases[pair.second]);

    // process each config file
    for(auto& configname : m_director_client.listConfigs())
    {
      auto& node = m_dep_by_daemon[configname];
      if(node.get() == nullptr)
        node = std::make_shared<depnode_t>();

      node->daemon_name = configname;
      node->service_names = clean_explode(m_director_client.get(configname, "/Process/ProvidedServices"), LIST_DELIM);
      for(std::string& service  : node->service_names)
        m_dep_by_service.emplace(service, node);

      for(std::string& service  : clean_explode(m_director_client.get(configname, "/Requirements/ActiveServices"  ), LIST_DELIM))
        node->service_active.push_back(m_dep_by_service[service]);

      for(std::string& service  : clean_explode(m_director_client.get(configname, "/Requirements/InactiveServices"), LIST_DELIM))
        node->service_inactive.push_back(m_dep_by_service[service]);

      for(std::string& daemon   : clean_explode(m_director_client.get(configname, "/Requirements/ActiveDaemons"   ), LIST_DELIM))
        node->daemon_active.push_back(m_dep_by_daemon[daemon]);

      for(std::string& daemon   : clean_explode(m_director_client.get(configname, "/Requirements/InactiveDaemons" ), LIST_DELIM))
        node->daemon_inactive.push_back(m_dep_by_daemon[daemon]);

      for(std::string& rl_alias : clean_explode(m_director_client.get(configname, "/Requirements/ActiveRunLevels" ), LIST_DELIM))
      {
        auto runlevel = m_runlevel_aliases.find(rl_alias); // find runlevel being referenced
        if(runlevel != m_runlevel_aliases.end()) // if found
        {
          node->runlevel_active[runlevel->second] = true; // enable for this node
          m_dep_by_runlevel[runlevel->second].insert(node); // add to list of runlevel enabled nodes
        }
      }

      for(std::string& rl_alias : clean_explode(m_director_client.get(configname, "/Requirements/InactiveRunLevels"), LIST_DELIM))
      {
        auto runlevel = m_runlevel_aliases.find(rl_alias); // find runlevel being referenced
        if(runlevel != m_runlevel_aliases.end()) // if found
          node->runlevel_inactive[runlevel->second] = true; // enable for this node
      }
    }
  }
}
