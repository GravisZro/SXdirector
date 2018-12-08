// STL
#include <functional>

// POSIX++
#include <cstdlib> // for std::atexit
#include <csignal> // for std::signal

// PDTK
#include <object.h>
#include <application.h>
#include <cxxutils/syslogstream.h>
#include <cxxutils/translate.h>
#include <specialized/capabilities.h>

// project
#include "directorcore.h"

#ifndef DIRECTOR_APP_NAME
#define DIRECTOR_APP_NAME       "SXdirector"
#endif

#ifndef DIRECTOR_USERNAME
#define DIRECTOR_USERNAME       "director"
#endif

#ifndef DIRECTOR_GROUPNAME
#define DIRECTOR_GROUPNAME      DIRECTOR_USERNAME
#endif


void exiting(void) noexcept
{
  posix::syslog << posix::priority::notice
                << "Service management provider (Director) has exited."_xlate
                << posix::eom;
}

//#include "demo.h"
int main(int argc, char *argv[]) noexcept
{

  uid_t euid = posix::geteuid();
  gid_t egid = posix::getegid();
  std::atexit(exiting);
  posix::syslog.open(DIRECTOR_APP_NAME, posix::facility::provider);

#if 0 && defined(POSIX_DRAFT_1E)
  if(::prctl(PR_SET_KEEPCAPS, 1) != posix::success_response)
  {
    posix::syslog << posix::priority::error
                  << "Director must be launched with the ability to manipulate process capabilities"_xlate
                  << posix::eom;
    std::exit(int(std::errc::permission_denied));
  }

  capability_data_t caps;

  if(::capget(caps, caps))
    posix::syslog << posix::priority::critical
                  << "Failed to get capabilities: %1"_xlate
                  << posix::strerror(errno)
                  << posix::eom;

  caps.effective
      .set(capflag::kill) // for killing the processes being supervised
      .set(capflag::net_admin) // for reading from the Process Event Connector
      .set(capflag::setuid)
      .set(capflag::setgid);

  caps.permitted = caps.effective;
  caps.inheritable = caps.effective;

  if(::capset(caps, caps) != posix::success_response)
    posix::syslog << posix::priority::critical
                  << "Failed to set capabilities: %1"_xlate
                  << posix::strerror(errno)
                  << posix::eom;
#endif
/*
  if((posix::strcmp(posix::getgroupname(egid), DIRECTOR_GROUPNAME) && // if current effective group name is NOT what we want AND
      !posix::setegid(posix::getgroupid(DIRECTOR_GROUPNAME))) || // unable to change effective group id
     (posix::strcmp(posix::getusername(euid), DIRECTOR_USERNAME) && // if current effective user name is NOT what we want AND
      !posix::seteuid(posix::getuserid (DIRECTOR_USERNAME)))) // unable to change effective user id
  {
    posix::syslog << posix::priority::error
                  << "Director must be launched as user/group \"%1\" or have permissions to seteuid/setegid"_xlate
                  << DIRECTOR_USERNAME
                  << posix::eom;
    std::exit(posix::error_t(std::errc::permission_denied));
  }
*/
  Application app;
  std::signal(SIGPIPE, SIG_IGN);
  std::signal(SIGINT, [](int){ printf("quit!\n"); Application::quit(0); }); // exit gracefully

  posix::fd_t shmemid = posix::invalid_descriptor;
  if(argc > 1 && std::atoi(argv[1]))
    shmemid = std::atoi(argv[1]);
  DirectorCore core(euid, egid, shmemid);
/*
  static std::function<void(void)> reload = [&core]() noexcept { core.reloadBinary(); }; // function to invoke program reload
  std::signal(SIGINT, [](int) noexcept
  {
    posix::syslog << posix::priority::critical
                  << "Signal interupt caught.  Service management provider (Director) is reloading."_xlate
                  << posix::eom;
    Object::singleShot(reload);
  }); // queue function to invoke program reload
*/
  return app.exec();
}
