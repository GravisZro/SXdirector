// POSIX++
#include <cstdlib>
#include <csignal>

// PDTK
#include <application.h>
#include <object.h>
#include <process.h>
#include <socket.h>
#include <cxxutils/syslogstream.h>
#include <specialized/capabilities.h>

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

#if defined(_POSIX_DRAFT_1E)
  if(::prctl(PR_SET_KEEPCAPS, 1) != posix::success_response)
  {
    posix::syslog << posix::priority::error
                  << "daemon must be launched with the ability to manipulate process capabilities"
                  << posix::eom;
    std::exit(int(std::errc::permission_denied));
  }
#endif

  if((std::strcmp(posix::getgroupname(::getgid()), groupname) && // if current username is NOT what we want AND
      ::setgid(posix::getgroupid(groupname)) == posix::error_response) || // unable to change user id
     (std::strcmp(posix::getusername(::getuid()), username) && // if current username is NOT what we want AND
      ::setuid(posix::getuserid (username)) == posix::error_response)) // unable to change user id
  {
    posix::syslog << posix::priority::error
                  << "daemon must be launched as user/group "
                  << '"' << username << '"'
                  << " or have permissions to setuid/setgid"
                  << posix::eom;
    std::exit(int(std::errc::permission_denied));
  }

#if defined(_POSIX_DRAFT_1E)
  capability_data_t caps;

  if(::capget(caps, caps))
    fprintf(stderr, "failed to get: %s\n", strerror(errno));

  caps.effective
      .set(capflag::net_admin)
      .set(capflag::setuid)
      .set(capflag::setgid);

  if(::capset(caps, caps) != posix::success_response)
    fprintf(stderr, "failed to set: %s\n", strerror(errno));
#endif

  Application app;
  ExecutorConfigClient config;

  if(config.connect(config_path))
  {
    posix::syslog << posix::priority::debug << "Executor connected to " << config_path << posix::eom;
  }
  else
  {
    posix::syslog << posix::priority::debug << "Executor unable to connected to " << config_path << posix::eom;
  }

  return app.exec();
}
