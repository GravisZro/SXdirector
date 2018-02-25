#include "dependencysolver.h"

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

int DependencySolver::dep_depth(depnodeptr dep, std::set<depnodeptr> activepath, std::set<depnodeptr> inactivepath) noexcept
{
  int max_activedepth = 0;
  int max_inactivedepth = 0;

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
    int activedepth = dep_depth(ptr, activepath, std::set<depnodeptr>());
    if(activedepth < 0)
      return posix::error_response; // report circular dependency because these dependencies are mandatory
    if(activedepth > max_activedepth)
      max_activedepth = activedepth;
  }

  for(auto& ptr : dep->dep_nodes.inactive.requirement)
  {
    int inactivedepth = dep_depth(ptr, std::set<depnodeptr>(), inactivepath);
    if(inactivedepth < 0)
      return posix::error_response; // report circular dependency because these dependencies are mandatory
    if(inactivedepth > max_inactivedepth)
      max_inactivedepth = inactivedepth;
  }

  for(auto& ptr : dep->dep_nodes.active.enhancement)
  {
    int activedepth = dep_depth(ptr, activepath, std::set<depnodeptr>());
    // NOTE: ignore circular dependencies because these dependencies are optional enhancements
    if(activedepth > max_activedepth)
      max_activedepth = activedepth;
  }

  for(auto& ptr : dep->dep_nodes.inactive.enhancement)
  {
    int inactivedepth = dep_depth(ptr, std::set<depnodeptr>(), inactivepath);
    // NOTE: ignore circular dependencies because these dependencies are optional enhancements
    if(inactivedepth > max_inactivedepth)
      max_inactivedepth = inactivedepth;
  }

  return m_dep_depths.emplace(dep, max_activedepth + max_inactivedepth + 1).first->second; // add one to include self
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

  return superset.emplace(depth, dep).second; // all required dependencies have been met!
}

void DependencySolver::resolveDependencies(void) noexcept
{
  // destroy all existing data
  m_dep_by_daemon.clear();
  m_dep_by_service.clear();
  m_orders.clear();

  // create node of each config
  for(auto& configname : getConfigList())
    m_dep_by_daemon.emplace(configname, std::make_shared<depnode_t>());

  // process each config
  for(auto& configname : getConfigList())
  {
    auto get_set = [this, configname](const std::string& source)
                     { return clean_explode(getConfigData(configname, source), LIST_DELIM); };

    auto& node = m_dep_by_daemon[configname];
    m_all_deps.emplace(node);

    node->daemon_name = configname;
    node->service_names = get_set("/Process/ProvidedServices");
    for(const std::string& service  : node->service_names)
      m_dep_by_service.emplace(service, node);

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

    for(const std::string& service : node->dep_services.active  .requirement) { node->dep_nodes.active  .requirement.emplace(m_dep_by_service[service]); }
    for(const std::string& service : node->dep_services.active  .enhancement) { node->dep_nodes.active  .enhancement.emplace(m_dep_by_service[service]); }
    for(const std::string& service : node->dep_services.inactive.requirement) { node->dep_nodes.inactive.requirement.emplace(m_dep_by_service[service]); }
    for(const std::string& service : node->dep_services.inactive.enhancement) { node->dep_nodes.inactive.enhancement.emplace(m_dep_by_service[service]); }
    for(const std::string& daemon  : node->dep_daemons .active  .requirement) { node->dep_nodes.active  .requirement.emplace(m_dep_by_daemon [daemon ]); }
    for(const std::string& daemon  : node->dep_daemons .active  .enhancement) { node->dep_nodes.active  .enhancement.emplace(m_dep_by_daemon [daemon ]); }
    for(const std::string& daemon  : node->dep_daemons .inactive.requirement) { node->dep_nodes.inactive.requirement.emplace(m_dep_by_daemon [daemon ]); }
    for(const std::string& daemon  : node->dep_daemons .inactive.enhancement) { node->dep_nodes.inactive.enhancement.emplace(m_dep_by_daemon [daemon ]); }

    for(const std::string& rl_alias : get_set("/Requirements/StartOnRunLevels" ))
    {
      if(getRunlevel(rl_alias) == posix::error_response)
      {}
      else
        node->runlevel_start.emplace(uint8_t(getRunlevel(rl_alias))); // started on runlevel
    }

    for(const std::string& rl_alias : get_set("/Requirements/StopOnRunLevels"))
    {
      if(getRunlevel(rl_alias) == posix::error_response)
      {}
      else
        node->runlevel_stop.emplace(uint8_t(getRunlevel(rl_alias))); // started on runlevel
    }
  }

//=== begin resolution ===

  // find depths of all daemons
  for(auto& dep : m_all_deps)
    m_dep_depths.emplace(dep, dep_depth(dep, std::set<depnodeptr>(), std::set<depnodeptr>()));

  // build a list of to start/stop daemons for each runlevel

  std::function<depsets_t<depnodeptr>&(depnodeptr)> active   = [](depnodeptr dep) noexcept ->depsets_t<depnodeptr>& { return dep->dep_nodes.active;   };
  std::function<depsets_t<depnodeptr>&(depnodeptr)> inactive = [](depnodeptr dep) noexcept ->depsets_t<depnodeptr>& { return dep->dep_nodes.inactive; };

  for(auto& dep : m_all_deps)
  {
    for(uint8_t rl : dep->runlevel_start)
      recurse_add(m_orders[rl].active, dep, active); // add all dependencies to the map of starting orders

    for(uint8_t rl : dep->runlevel_stop)
      recurse_add(m_orders[rl].inactive, dep, inactive); // add all dependencies to the map of stopping orders
  }
}

start_stop_t DependencySolver::getRunlevelOrder(const std::string& runlevel) const noexcept
{
  start_stop_t data;

  int rlnum = getRunlevel(runlevel);
  if(rlnum != posix::error_response)
  {
    data.runlevel = uint8_t(rlnum); // copy the runlevel numeric value
    auto order_iter = m_orders.find(data.runlevel); // find the data for the runlevel
    if(order_iter != m_orders.end()) // ensure that the data was found
    {
      activity_t<std::list<std::pair<int, depnodeptr>>> sorted;
      std::copy(order_iter->second.active  .begin(), order_iter->second.active  .end(), std::back_inserter(sorted.active  )); // copy the runlevel data over for starting daemons
      std::copy(order_iter->second.inactive.begin(), order_iter->second.inactive.end(), std::back_inserter(sorted.inactive)); // copy the runlevel data over for stoping daemons

      sorted.active.sort(std::less<std::pair<int, depnodeptr>>()); // sort the runlevel data for starting daemons by it's depth
      for(auto& pair : sorted.active)
        data.start.emplace_back(pair.second->daemon_name); // add daemon to ordered list

      sorted.inactive.sort(std::greater<std::pair<int, depnodeptr>>()); // sort the runlevel data for stopping daemons by it's depth
      for(auto& pair : sorted.inactive)
        data.stop.emplace_back(pair.second->daemon_name); // add daemon to ordered list
    }
  }
  return data; // return ordered list of daemons to start/stop for this runlevel
}
