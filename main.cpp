
// PDTK
#include <object.h>
#include <application.h>
#include <cxxutils/syslogstream.h>
#include <cxxutils/translate.h>
#include <specialized/capabilities.h>

// project
#include "directorcore.h"

#ifndef DIRECTOR_APP_NAME
# define DIRECTOR_APP_NAME      "SXdirector"
#endif

#ifndef DIRECTOR_USERNAME
# define DIRECTOR_USERNAME      "director"
#endif

#ifndef DIRECTOR_GROUPNAME
# define DIRECTOR_GROUPNAME     DIRECTOR_USERNAME
#endif

#if defined(__linux__) && KERNEL_VERSION_CODE >= KERNEL_VERSION(2,1,44)
# include <sys/prctl.h>
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
  posix::atexit(exiting);
  posix::syslog.open(DIRECTOR_APP_NAME, posix::facility::provider);

#if defined(PR_SET_CHILD_SUBREAPER)
  if(::prctl(PR_SET_CHILD_SUBREAPER, 1) != posix::success_response &&
     posix::getpid() != 1)
  {
    posix::syslog << posix::priority::warning
                  << "%1\n%2"
                  << "Director was unable to install a child process subreaper and is not process identifier 1."_xlate
                  << "Director may lose track of processes that fork excessively."_xlate
                  << posix::eom;
  }
#else
  if(posix::getpid() != 1)
  {
    posix::syslog << posix::priority::warning
                  << "%1\n%2"
                  << "Director is not running as process identifier 1."_xlate
                  << "Director may lose track of processes that fork excessively."_xlate
                  << posix::eom;
  }
#endif

#if defined(POSIX_DRAFT_1E)
  if(::prctl(PR_SET_KEEPCAPS, 1) != posix::success_response)
  {
    posix::syslog << posix::priority::error
                  << "Director must be launched with the ability to manipulate process capabilities"_xlate
                  << posix::eom;
    posix::exit(int(posix::errc::permission_denied));
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

  if((posix::strcmp(posix::getgroupname(egid), DIRECTOR_GROUPNAME) && // if current effective group name is NOT what we want AND
      !posix::setegid(posix::getgroupid(DIRECTOR_GROUPNAME))) || // unable to change effective group id
     (posix::strcmp(posix::getusername(euid), DIRECTOR_USERNAME) && // if current effective user name is NOT what we want AND
      !posix::seteuid(posix::getuserid (DIRECTOR_USERNAME)))) // unable to change effective user id
  {
    posix::syslog << posix::priority::error
                  << "Director must be launched as user/group \"%1\" or have permissions to seteuid/setegid"_xlate
                  << DIRECTOR_USERNAME
                  << posix::eom;
//    posix::exit(posix::error_t(posix::errc::permission_denied));
  }
  Application app;
  posix::signal(SIGPIPE, SIG_IGN);

  posix::fd_t shmemid = posix::invalid_descriptor;
  if(argc > 1 && posix::atoi(argv[1]))
    shmemid = posix::atoi(argv[1]);
  DirectorCore core(euid, egid, shmemid);

#if defined(DEBUG)
  posix::signal(SIGINT, [](int){ posix::printf("quit!\n"); Application::quit(0); }); // exit gracefully
#else
  static std::function<void(void)> reload = [&core]() noexcept { core.reloadBinary(); }; // function to invoke program reload
  posix::signal(SIGINT, [](int) noexcept
  {
    posix::syslog << posix::priority::critical
                  << "Signal interupt caught.  Service management provider (Director) is reloading."_xlate
                  << posix::eom;
    Object::singleShot(reload);
  }); // queue function to invoke program reload
#endif

  return app.exec();
}
