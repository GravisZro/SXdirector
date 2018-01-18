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

    std::unordered_map<std::string, Process*>::size_type count;
    storage >> count;

    // get rid of shared memory
    std::memset(shmem, 0, SHARED_MEMORY_SIZE); // zero out for safety
    ::shmctl(shmemid, IPC_RMID, nullptr); // release memory
  }
}

ExecutorCore::~ExecutorCore(void) noexcept
{
  for(auto& proc : m_process_map)
    delete proc.second;
}

void ExecutorCore::reload(void) noexcept
{
  int shmemid = ::shmget(IPC_PRIVATE, SHARED_MEMORY_SIZE, IPC_CREAT | SHM_R | SHM_W); // create shared memory segment
  char* shmem = reinterpret_cast<char*>(::shmat(shmemid, nullptr, 0)); // retrieve shared memory
  std::memset(shmem, 0, SHARED_MEMORY_SIZE); // zero out for safety

  // save state to shared memory
  vfifo storage(shmem, SHARED_MEMORY_SIZE);

  storage << m_process_map.size();

  for(auto& pair : m_process_map)
  {
    storage << pair.first
            << pair.second->processId()
            << pair.second->getStdIn()
            << pair.second->getStdOut()
            << pair.second->getStdErr();
  }

  // reload process
  process_state_t data;
  if(::procstat(::getpid(), data) == posix::success_response)
    ::execl(data.executable.c_str(), data.executable.c_str(), std::to_string(shmemid).c_str(), NULL);
  terminal::write("Failed to reload Executor from binary!");
  Application::quit(errno);
}
