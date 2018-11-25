#include "servicecheck.h"

// POSIX++
#include <climits>

// PDTK
#include <cxxutils/posix_helpers.h>
#include <specialized/mountpoints.h>


static char path[PATH_MAX];

bool service_exists(const std::string& service)
{
  if(scfs_path == nullptr &&
    !reinitialize_paths())
    return false;
  std::snprintf(path, sizeof(path), "%s/%s", scfs_path, service.c_str());
  if(posix::access(path, F_OK) == posix::success_response)
    return false;
  return true;
}
