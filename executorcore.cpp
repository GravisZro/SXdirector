#include "executorcore.h"

// PDTK
#include <cxxutils/syslogstream.h>

#ifndef MCFS_PATH
#define MCFS_PATH               "/mc"
#endif

#ifndef EXECUTOR_USERNAME
#define EXECUTOR_USERNAME       "executor"
#endif

#ifndef CONFIG_USERNAME
#define CONFIG_USERNAME         "config"
#endif

#ifdef ANONYMOUS_SOCKET
#undef ANONYMOUS_SOCKET
#endif
#define ANONYMOUS_SOCKET        "\0"

#ifndef EXECUTOR_IO_SOCKET
#define EXECUTOR_IO_SOCKET      MCFS_PATH "/" EXECUTOR_USERNAME "/io"
#endif

#ifndef CONFIG_IO_SOCKET
#define CONFIG_IO_SOCKET        MCFS_PATH "/" CONFIG_USERNAME "/io"
#endif

#ifndef CONFIG_EXECUTOR_SOCKET
#define CONFIG_EXECUTOR_SOCKET  MCFS_PATH "/" CONFIG_USERNAME "/executor"
#endif

#ifndef CONFIG_CONFIG_PATH
#define CONFIG_CONFIG_PATH    "/etc/config"
#endif

#ifndef EXECUTOR_CONFIG_PATH
#define EXECUTOR_CONFIG_PATH  "/etc/executor"
#endif

ExecutorCore::ExecutorCore(void)
{
  bool socket_file = use_socket_file(CONFIG_IO_SOCKET) || use_socket_file(CONFIG_EXECUTOR_SOCKET);

  if(m_config_client.connect(socket_file ? CONFIG_IO_SOCKET : ANONYMOUS_SOCKET CONFIG_IO_SOCKET))
    posix::syslog << posix::priority::info << "Connected to " << (socket_file ? "socket file" : "anonymous socket") << CONFIG_IO_SOCKET << posix::eom;
  else
    posix::syslog << posix::priority::error << "Unable to connect to " << (socket_file ? "socket file" : "anonymous socket") << CONFIG_IO_SOCKET << posix::eom;

  if(m_executor_client.connect(socket_file ? CONFIG_EXECUTOR_SOCKET : ANONYMOUS_SOCKET CONFIG_EXECUTOR_SOCKET))
    posix::syslog << posix::priority::info << "Executor connected to " << (socket_file ? "socket file" : "anonymous socket") << CONFIG_EXECUTOR_SOCKET << posix::eom;
  else
    posix::syslog << posix::priority::error << "Executor unable to connect to " << (socket_file ? "socket file" : "anonymous socket") << CONFIG_EXECUTOR_SOCKET << posix::eom;

}
