// STL
#include <memory>
#include <list>
#include <string>
#include <unordered_map>

// PUT
#include <put/socket.h>
#include <put/cxxutils/posix_helpers.h>
#include <put/cxxutils/vfifo.h>
#include <put/cxxutils/vterm.h>
#include <put/cxxutils/hashing.h>
#include <put/cxxutils/configmanip.h>
#include <put/specialized/fileevent.h>
#include <put/specialized/mountpoints.h>


#define UNITTEST_USERNAME  "unittest"
#define UNITTEST_GROUPNAME UNITTEST_USERNAME

int main(int argc, char *argv[]) noexcept
{
  if(scfs_path == nullptr)
    reinitialize_paths();
  if(scfs_path == nullptr)
  {
    terminal::write("FAILURE: SCFS is not mounted\n");
    return EXIT_FAILURE;
  }

  if(posix::strcmp(posix::getgroupname(posix::getgid()), UNITTEST_GROUPNAME) && // if current groupname is NOT what we want AND
     !posix::setgid(posix::getgroupid(UNITTEST_GROUPNAME))) // unable to change group id
  {
    terminal::write("FAILURE: Unable to change group id to '%s'\n", UNITTEST_GROUPNAME);
    return EXIT_FAILURE;
  }

  if(posix::strcmp(posix::getusername(posix::getuid()), UNITTEST_USERNAME) && // if current username is NOT what we want AND
     !posix::setuid(posix::getuserid(UNITTEST_USERNAME))) // unable to change user id
  {
    terminal::write("FAILURE: Unable to change user id to '%s'\n", UNITTEST_USERNAME);
    return EXIT_FAILURE;
  }

  std::string base = scfs_path;
  base.append("/" UNITTEST_USERNAME "/io");

  ServerSocket server;

  if(!server.bind(base.c_str()))
  {
    terminal::write("FAILURE: Unable to bind test provider to '%s'\n", base.c_str());
    return EXIT_FAILURE;
  }


  terminal::write("SUCCESS\n");
  return EXIT_SUCCESS;
}
