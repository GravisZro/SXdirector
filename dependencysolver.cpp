#include "dependencysolver.h"

// STL
#include <algorithm>

// POSIX++
#include <climits>

#define LIST_DELIM ','

static std::set<std::string> clean_explode(const std::string& str, char delim) noexcept
{
  std::set<std::string> strs;
  std::string newstr;
  newstr.reserve(NAME_MAX);

  for(auto& character : str)
  {
    if(character == delim)
    {
      if(!newstr.empty())
      {
        strs.emplace(newstr);
        newstr.clear();
      }
    }
    else if(std::isgraph(character))
      newstr.push_back(character);
  }

  if(!newstr.empty())
    strs.emplace(newstr);

  return strs;
}

int DependencySolver::queueErrorMessage(const std::string& context, const std::string& source, const std::string& problem) noexcept
{
  m_errors.emplace_back(context);
  m_errors.back()
      .append("|").append(source)
      .append("|").append(problem);
  return posix::error_response;
}

int DependencySolver::dep_depth(depnodeptr dep, std::set<depnodeptr> activepath, std::set<depnodeptr> inactivepath) noexcept
{
  int max_depth = 0;

  if(dep == nullptr)
    return posix::error_response; // report unresolved dependency

  auto res = m_dep_depths.find(dep);
  if(res != m_dep_depths.end())
    return res->second;

  if(!activepath.emplace(dep).second ||
     !inactivepath.emplace(dep).second) // if failed to add to path (because it exists)
    return m_dep_depths.emplace(dep, posix::error_response).first->second; // report circular dependency

  for(auto& ptr : dep->dep_nodes.active.requirement)
  {
    int depth = dep_depth(ptr, activepath, std::set<depnodeptr>());
    if(depth < 0)
      return queueErrorMessage(dep->daemon_name, "dependencies.active.requirement", "circular"); // report circular dependency because these dependencies are mandatory
    max_depth = std::max(max_depth, depth);
  }

  for(auto& ptr : dep->dep_nodes.inactive.requirement)
  {
    int depth = dep_depth(ptr, std::set<depnodeptr>(), inactivepath);
    if(depth < 0)
      return queueErrorMessage(dep->daemon_name, "dependencies.inactive.requirement", "circular"); // report circular dependency because these dependencies are mandatory
    max_depth = std::max(max_depth, depth);
  }

  for(auto& ptr : dep->dep_nodes.active.enhancement)
    max_depth = std::max(max_depth, dep_depth(ptr, activepath, std::set<depnodeptr>()));
    // NOTE: ignore circular dependencies because these dependencies are optional enhancements

  for(auto& ptr : dep->dep_nodes.inactive.enhancement)
    max_depth = std::max(max_depth, dep_depth(ptr, std::set<depnodeptr>(), inactivepath));
    // NOTE: ignore circular dependencies because these dependencies are optional enhancements

  return m_dep_depths.emplace(dep, max_depth + 1).first->second; // add one to include self
}

bool DependencySolver::recurse_add(std::set<std::pair<int, depnodeptr>>& superset, depnodeptr dep, std::function<depsets_t<depnodeptr>&(depnodeptr)>& selector) const noexcept
{
  auto iter = m_dep_depths.find(dep);
  if(iter == m_dep_depths.end())
    return false;

  const int& depth = iter->second;
  if(depth == posix::error_response)
    return false; // dep has unmet dependencies

  for(auto subdep : selector(dep).requirement)
    if(!recurse_add(superset, subdep, selector))
      return false; // one of the required dependencies is unmet

  std::set<std::pair<int, depnodeptr>> subset;
  for(auto subdep : selector(dep).enhancement)
  {
    subset.clear();
    if(recurse_add(subset, subdep, selector)) // if no bad unmet dependencies found
      for(auto subsubdep : subset) // incorporate subset into superset
        superset.emplace(subsubdep);
  }

   // all required dependencies have been met!
  superset.emplace(depth, dep);
  return true;
}

void DependencySolver::resolveDependencies(void) noexcept
{
  // temporary caches
  std::set<depnodeptr> all_deps;
  std::map<std::string, depnodeptr> dep_by_daemon;
  std::map<std::string, depnodeptr> dep_by_service;

  // clear existing data
  m_orders.clear(); // previously ordered
  m_errors.clear(); // clear error messages

  // create node of each config
  for(auto& configname : getConfigList())
    dep_by_daemon.emplace(configname, std::make_shared<depnode_t>());

  // process each config
  for(auto& configname : getConfigList())
  {
    auto get_set = [this, configname](const std::string& source)
                     { return clean_explode(getConfigData(configname, source), LIST_DELIM); };

    auto& node = dep_by_daemon.at(configname);
    all_deps.emplace(node);

    node->daemon_name = configname;
    node->service_names = get_set("/Process/ProvidedServices");
    for(const std::string& service  : node->service_names)
      dep_by_service.emplace(service, node);

    node->dep_services =
      {
        { // active
          get_set("/Requirements/ActiveServices"  ), // requirement
          get_set("/Enhancements/ActiveServices"  ), // enhancement
        },
        { // inactive
          get_set("/Requirements/InactiveServices"), // requirement
          get_set("/Enhancements/InactiveServices"), // enhancement
        },
      };

    node->dep_daemons =
      {
        { // active
          get_set("/Requirements/ActiveDaemons"   ), // requirement
          get_set("/Enhancements/ActiveDaemons"   ), // enhancement
        },
        { // inactive
          get_set("/Requirements/InactiveDaemons" ), // requirement
          get_set("/Enhancements/InactiveDaemons" ), // enhancement
        },
      };


    for(const std::string& rl_alias : get_set("/Requirements/StartOnRunLevels" ))
    {
      if(getRunlevel(rl_alias) == posix::error_response)
        queueErrorMessage(rl_alias, "runlevel.start.requirement", "unresolved");
      else
        node->runlevel_start.emplace(uint8_t(getRunlevel(rl_alias))); // started on runlevel
    }

    for(const std::string& rl_alias : get_set("/Requirements/StopOnRunLevels"))
    {
      if(getRunlevel(rl_alias) == posix::error_response)
        queueErrorMessage(rl_alias, "runlevel.stop.requirement", "unresolved");
      else
        node->runlevel_stop.emplace(uint8_t(getRunlevel(rl_alias))); // started on runlevel
    }
  }

//=== post-processing ===
  for(auto& configname : getConfigList())
  {
    auto& node = dep_by_daemon.at(configname);
    for(const std::string& service : node->dep_services.active  .requirement) { if(dep_by_service.find(service) == dep_by_service.end()) { queueErrorMessage(service, "service.active.requirement"  , "unresolved"); } else { node->dep_nodes.active.requirement.emplace(dep_by_service.at(service)); } }
    for(const std::string& service : node->dep_services.active  .enhancement) { if(dep_by_service.find(service) == dep_by_service.end()) { queueErrorMessage(service, "service.active.enhancement"  , "unresolved"); } else { node->dep_nodes.active.enhancement.emplace(dep_by_service.at(service)); } }
    for(const std::string& daemon  : node->dep_daemons .active  .requirement) { if(dep_by_daemon .find(daemon ) == dep_by_daemon .end()) { queueErrorMessage(daemon , "daemon.active.requirement"   , "unresolved"); } else { node->dep_nodes.active.requirement.emplace(dep_by_daemon .at(daemon )); } }
    for(const std::string& daemon  : node->dep_daemons .active  .enhancement) { if(dep_by_daemon .find(daemon ) == dep_by_daemon .end()) { queueErrorMessage(daemon , "daemon.active.enhancement"   , "unresolved"); } else { node->dep_nodes.active.enhancement.emplace(dep_by_daemon .at(daemon )); } }
    for(const std::string& service : node->dep_services.inactive.requirement) { if(dep_by_service.find(service) == dep_by_service.end()) { queueErrorMessage(service, "service.inactive.requirement", "unresolved"); } else { dep_by_service.at(service)->dep_nodes.inactive.requirement.emplace(node); } }
    for(const std::string& service : node->dep_services.inactive.enhancement) { if(dep_by_service.find(service) == dep_by_service.end()) { queueErrorMessage(service, "service.inactive.enhancement", "unresolved"); } else { dep_by_service.at(service)->dep_nodes.inactive.enhancement.emplace(node); } }
    for(const std::string& daemon  : node->dep_daemons .inactive.requirement) { if(dep_by_daemon .find(daemon ) == dep_by_daemon .end()) { queueErrorMessage(daemon , "daemon.inactive.requirement" , "unresolved"); } else { dep_by_daemon .at(daemon )->dep_nodes.inactive.requirement.emplace(node); } }
    for(const std::string& daemon  : node->dep_daemons .inactive.enhancement) { if(dep_by_daemon .find(daemon ) == dep_by_daemon .end()) { queueErrorMessage(daemon , "daemon.inactive.enhancement" , "unresolved"); } else { dep_by_daemon .at(daemon )->dep_nodes.inactive.enhancement.emplace(node); } }
  }

//=== begin resolution ===

  // find depths of all daemons
  for(auto& dep : all_deps)
    m_dep_depths.emplace(dep, dep_depth(dep, std::set<depnodeptr>(), std::set<depnodeptr>()));

  // build a list of to start/stop daemons for each runlevel

  std::function<depsets_t<depnodeptr>&(depnodeptr)> active   = [](depnodeptr dep) noexcept ->depsets_t<depnodeptr>& { return dep->dep_nodes.active;   };
  std::function<depsets_t<depnodeptr>&(depnodeptr)> inactive = [](depnodeptr dep) noexcept ->depsets_t<depnodeptr>& { return dep->dep_nodes.inactive; };

  for(auto& dep : all_deps)
  {
    for(uint8_t rl : dep->runlevel_start)
      if(!recurse_add(m_orders[rl].active, dep, active)) // add all dependencies to the map of starting orders
        queueErrorMessage(dep->daemon_name, "runlevel.active", "add failed");

    for(uint8_t rl : dep->runlevel_stop)
      if(!recurse_add(m_orders[rl].inactive, dep, inactive)) // add all dependencies to the map of stopping orders
        queueErrorMessage(dep->daemon_name, "runlevel.inactive", "add failed");
  }

  // destroy all cached data
  m_dep_depths.clear();
  for(auto& dep : all_deps)
  {
    dep->dep_nodes.active.requirement.clear();
    dep->dep_nodes.active.enhancement.clear();
    dep->dep_nodes.inactive.requirement.clear();
    dep->dep_nodes.inactive.enhancement.clear();
  }
}

start_stop_t DependencySolver::getRunlevelOrder(const std::string& runlevel) const noexcept
{
  start_stop_t data;

  int rlnum = getRunlevel(runlevel);
  if(rlnum != posix::error_response)
  {
    data.runlevel = uint8_t(rlnum); // copy the runlevel numeric value
    data.start.reserve(64);
    data.stop .reserve(64);

    auto order_iter = m_orders.find(data.runlevel); // find the data for the runlevel
    if(order_iter != m_orders.end()) // ensure that the data was found
    {
      for(auto& pair : order_iter->second.active)
      {
        if(data.start.size() < posix::size_t(pair.first + 1))
          data.start.resize(pair.first + 1);
        data.start.at(pair.first).emplace_back(pair.second->daemon_name); // add daemon to ordered list
      }

      for(auto& pair : order_iter->second.inactive)
      {
        if(data.stop.size() < posix::size_t(pair.first + 1))
          data.stop.resize(pair.first + 1);
        data.stop.at(pair.first).emplace_back(pair.second->daemon_name); // add daemon to ordered list
      }
    }
  }
  return data; // return ordered list of daemons to start/stop for this runlevel
}
