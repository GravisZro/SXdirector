#include "servicecheck.h"

// POSIX++
#include <climits>

// PUT
#include <put/cxxutils/posix_helpers.h>
#include <put/specialized/mountpoints.h>


static char path[PATH_MAX];

bool service_exists(const std::string& service)
{
  return service_exists(service.c_str());
}

bool service_exists(const char* service)
{
  if(scfs_path == nullptr &&
    !reinitialize_paths())
    return false;
  posix::snprintf(path, sizeof(path), "%s/%s", scfs_path, service);
  if(posix::access(path, posix::file_exists))
    return false;
  return true;
}
