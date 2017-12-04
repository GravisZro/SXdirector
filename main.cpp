// POSIX++
#include <cstdlib>
#include <csignal>
#include <cstdio>

// PDTK
#include <application.h>
#include <object.h>
#include <process.h>
#include <socket.h>
#include <cxxutils/syslogstream.h>
#include <specialized/capabilities.h>

// project
#include "executorconfigclient.h"
#include "configclient.h"

#ifndef EXECUTOR_APP_NAME
#define EXECUTOR_APP_NAME       "SXexecutor"
#endif

#ifndef MCFS_PATH
#define MCFS_PATH               "/mc"
#endif

#ifndef EXECUTOR_USERNAME
#define EXECUTOR_USERNAME       "executor"
#endif

#ifndef EXECUTOR_GROUPNAME
#define EXECUTOR_GROUPNAME      EXECUTOR_USERNAME
#endif

#ifndef CONFIG_USERNAME
#define CONFIG_USERNAME         "config"
#endif

#ifndef CONFIG_GROUPNAME
#define CONFIG_GROUPNAME        CONFIG_USERNAME
#endif

#ifndef EXECUTOR_IO_SOCKET
#define EXECUTOR_IO_SOCKET      MCFS_PATH "/" EXECUTOR_USERNAME "/io"
#endif

#ifndef CONFIG_IO_SOCKET
#define CONFIG_IO_SOCKET        MCFS_PATH "/" CONFIG_USERNAME "/io"
#endif

#ifndef CONFIG_EXECUTOR_SOCKET
#define CONFIG_EXECUTOR_SOCKET  MCFS_PATH "/" CONFIG_USERNAME "/executor"
#endif


void exiting(void)
{
  posix::syslog << posix::priority::notice << "daemon has exited." << posix::eom;
}

int main(int argc, char *argv[]) noexcept
{
  (void)argc;
  (void)argv;

  std::unordered_map<const char*, const char*> defaults = // config file default values
  {
    { "/Config/InitConfigPath", "/etc/executor" },
  };

  std::atexit(exiting);
  std::signal(SIGPIPE, SIG_IGN);
  posix::syslog.open(EXECUTOR_APP_NAME, posix::facility::daemon);

#if defined(POSIX_DRAFT_1E)
  if(::prctl(PR_SET_KEEPCAPS, 1) != posix::success_response)
  {
    posix::syslog << posix::priority::error
                  << "Daemon must be launched with the ability to manipulate process capabilities"
                  << posix::eom;
    std::exit(int(std::errc::permission_denied));
  }

  capability_data_t caps;

  if(::capget(caps, caps))
    posix::syslog << posix::priority::critical << "Failed to get capabilities: " << std::strerror(errno) << posix::eom;

  caps.effective
      .set(capflag::kill) // for killing the processes being supervised
      .set(capflag::net_admin) // for reading from the Process Event Connector
      .set(capflag::setuid)
      .set(capflag::setgid);

  if(::capset(caps, caps) != posix::success_response)
    posix::syslog << posix::priority::critical << "Failed to set capabilities: " << std::strerror(errno) << posix::eom;
#endif

  if((std::strcmp(posix::getgroupname(::getgid()), EXECUTOR_GROUPNAME) && // if current username is NOT what we want AND
      ::setgid(posix::getgroupid(EXECUTOR_GROUPNAME)) == posix::error_response) || // unable to change user id
     (std::strcmp(posix::getusername(::getuid()), EXECUTOR_USERNAME) && // if current username is NOT what we want AND
      ::setuid(posix::getuserid (EXECUTOR_USERNAME)) == posix::error_response)) // unable to change user id
  {
    posix::syslog << posix::priority::error
                  << "Daemon must be launched as user/group "
                  << '"' << EXECUTOR_USERNAME << '"'
                  << " or have permissions to setuid/setgid"
                  << posix::eom;
    std::exit(int(std::errc::permission_denied));
  }

  Application app;
  ConfigClient config;
  ExecutorConfigClient exconfig;

  if(config.connect(CONFIG_IO_SOCKET))
  {
    posix::syslog << posix::priority::debug << "Connected to " << CONFIG_IO_SOCKET << posix::eom;
  }
  else
  {
    posix::syslog << posix::priority::debug << "Unable to connected to " << CONFIG_IO_SOCKET << posix::eom;
  }


  if(exconfig.connect(CONFIG_EXECUTOR_SOCKET))
  {
    posix::syslog << posix::priority::debug << "Executor connected to " << CONFIG_EXECUTOR_SOCKET << posix::eom;
  }
  else
  {
    posix::syslog << posix::priority::debug << "Executor unable to connected to " << CONFIG_EXECUTOR_SOCKET << posix::eom;
  }

  return app.exec();
}
