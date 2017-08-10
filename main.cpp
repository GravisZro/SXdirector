// POSIX++
#include <cstdlib>
#include <csignal>

// PDTK
#include <application.h>
#include <object.h>
#include <process.h>
#include <socket.h>
#include <cxxutils/syslogstream.h>

// project
#include "executorconfigclient.h"

constexpr const char* const appname     = "SXexecutor";
constexpr const char* const username    = "executor";
constexpr const char* const groupname   = "executor";
constexpr const char* const socket_path = "/mc/executor/io";
constexpr const char* const config_path = "/mc/config/executor";

void exiting(void)
{
  posix::syslog << posix::priority::notice << "daemon has exited." << posix::eom;
}

int main(int argc, char *argv[]) noexcept
{
  (void)argc;
  (void)argv;
  std::atexit(exiting);
  std::signal(SIGPIPE, SIG_IGN);

  posix::syslog.open(appname, posix::facility::daemon);

  if((std::strcmp(posix::getgroupname(::getgid()), groupname) && // if current username is NOT what we want AND
      ::setgid(posix::getgroupid(groupname)) == posix::error_response) || // unable to change user id
     (std::strcmp(posix::getusername(::getuid()), username) && // if current username is NOT what we want AND
      ::setuid(posix::getuserid (username)) == posix::error_response)) // unable to change user id
  {
    posix::syslog << posix::priority::critical
                  << "daemon must be launched as username "
                  << '"' << username << '"'
                  << " or have permissions to setuid/setgid"
                  << posix::eom;
    std::exit(int(std::errc::permission_denied));
  }

  Application app;
  ExecutorConfigClient config;

  if(config.connect(config_path))
  {
    posix::syslog << posix::priority::info << "Executor connected to " << config_path << posix::eom;
  }
  else
  {
    posix::syslog << posix::priority::info << "Executor unable to connected to " << config_path << posix::eom;
  }

  return app.exec();
}
