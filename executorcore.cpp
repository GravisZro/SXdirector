#include "executorcore.h"

// POSIX
#include <sys/shm.h>

// PDTK
#include <cxxutils/syslogstream.h>
#include <cxxutils/vterm.h>
#include <specialized/procstat.h>
#include <specialized/proclist.h>

#define SHARED_MEMORY_SIZE   0x4000

ExecutorCore::ExecutorCore(posix::fd_t shmemid) noexcept
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
  Object::connect(m_config_client.synchronized, this, &ExecutorCore::reloadSettings);
  Object::connect(m_executor_client.synchronized, this, &ExecutorCore::reloadSettings);
}

ExecutorCore::~ExecutorCore(void) noexcept
{
}

void ExecutorCore::reloadBinary(void) noexcept
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

  // reload process
  process_state_t data;
  if(::procstat(::getpid(), data) == posix::success_response)
    ::execl(data.executable.c_str(), data.executable.c_str(), std::to_string(shmemid).c_str(), NULL);
  terminal::write("%s%s\n", terminal::critical, "Failed to reload Executor from binary!");
  Application::quit(errno);
}

void ExecutorCore::reloadSettings(void) noexcept
{
  if(m_config_client.isSynchronized() &&
     m_executor_client.isSynchronized())
  {
    const std::vector<std::string> keys =
    {
      "/Requirements/ActiveServices",
      "/Requirements/InactiveServices",
      "/Requirements/ActiveDaemons",
      "/Requirements/InactiveDaemons",
      "/Requirements/IncludeRunLevels",
      "/Requirements/ExcludeRunLevels",
      "/Requirements/MinRunLevel",
      "/Requirements/MaxRunLevel",
    };
    for(auto& configname : m_executor_client.listConfigs())
    {
      m_executor_client.get(configname, "/Process/ProvidesServices");
      for(auto& key : keys)
        m_executor_client.get(configname, key);
    }
  }
}
