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

static inline std::string active_string(bool is_active) { return is_active ? "active" : "inactive"; }
static inline std::string required_string(bool is_required) { return is_required ? "requirement" : "enhancement"; }

int DependencySolver::dep_depth(depnodeptr origin, depinfo_t<depnodeptr> dep, depinfoset_t<depnodeptr> path, bool mandatory) noexcept
{
  int max_depth = 0;

  if(dep.data == nullptr)
    return posix::error_response; // report unresolved dependency

  auto res = m_dep_depths.find(dep.data);
  if(res != m_dep_depths.end())
    return res->second;

  if(!path.emplace(dep).second) // if failed to add to path (because it exists)
    return posix::error_response; // report circular dependency

  for(auto& ptr : dep.data->dep_nodes)
  {
    int depth = dep_depth(origin, ptr, path, mandatory && ptr.is_required);
    if(mandatory && ptr.is_required && depth < 0)
      queueErrorMessage(origin->daemon_name, dep.data->daemon_name + ".dependencies." + active_string(dep.is_active) + ".requirement", "circular"); // report circular dependency because these dependencies are mandatory
    max_depth = std::max(max_depth, depth);
  }

  return max_depth + 1;
  //return m_dep_depths.emplace(dep.data, max_depth + 1).first->second; // add one to include self
}

bool DependencySolver::recurse_add(std::set<std::pair<int, depnodeptr>>& superset, depnodeptr dep, bool is_active) noexcept
{
  const bool requirement = true;
//  int depth = dep_depth(dep, depinfo_t<depnodeptr>{requirement, is_active, dep}, {}, true);

  auto iter = m_dep_depths.find(dep);
  if(iter == m_dep_depths.end())
    return false;

  const int& depth = iter->second;
  if(depth == posix::error_response)
    return false; // dep has unmet dependencies

  if(!superset.emplace(depth, dep).second)
    return false; // dep already exists

  for(auto& subdep : dep->dep_nodes)
    if(is_active && !recurse_add(superset, subdep.data, is_active))  // if required AND have an unmet dependency
      return false; // fail

  std::set<std::pair<int, depnodeptr>> subset;
  for(auto& subdep : dep->dep_nodes)
  {
    subset.clear();
    subset.emplace(depth, dep);
    if(!is_active && recurse_add(subset, subdep.data, is_active)) // if optional AND have no unmet dependencies
      for(auto subsubdep : subset) // incorporate subset into superset
        superset.emplace(subsubdep);
  }

   // all required dependencies have been met!
  return true;
}

void DependencySolver::resolveDependencies(void) noexcept
{
  const bool active = true;
  const bool inactive = false;
  const bool requirement = true;
  const bool enhancement = false;

  // temporary caches
  std::set<depnodeptr> all_deps;
  std::map<std::string, depnodeptr> dep_by_daemon;
  std::map<std::string, depnodeptr> dep_by_service;

  // clear existing data
  m_orders_start.clear(); // previously ordered
  m_orders_stop.clear(); // previously ordered
  m_errors.clear(); // clear error messages

  // create node of each config
  for(auto& configname : getConfigList())
    dep_by_daemon.emplace(configname, std::make_shared<depnode_t>());

  // process each config
  for(auto& configname : getConfigList())
  {
    auto get_set = [this, configname](const std::string& source)
                     { return clean_explode(getConfigData(configname, source), LIST_DELIM); };

    auto merge_in = [](depinfoset_t<std::string>& dest, const std::set<std::string> source, bool is_required, bool is_active)
    {
      for(const auto& str : source)
        dest.emplace(depinfo_t<std::string>{is_required, is_active, str});
    };

    auto& node = dep_by_daemon.at(configname);
    all_deps.emplace(node);

    node->daemon_name = configname;
    node->service_names = get_set("/Process/ProvidedServices");
    for(const std::string& service  : node->service_names)
      dep_by_service.emplace(service, node);

    merge_in(node->dep_services, get_set("/Requirements/ActiveServices"  ), requirement, active  );
    merge_in(node->dep_services, get_set("/Enhancements/ActiveServices"  ), enhancement, active  );
    merge_in(node->dep_services, get_set("/Requirements/InactiveServices"), requirement, inactive);
    merge_in(node->dep_services, get_set("/Enhancements/InactiveServices"), enhancement, inactive);

    merge_in(node->dep_daemons , get_set("/Requirements/ActiveDaemons"   ), requirement, active  );
    merge_in(node->dep_daemons , get_set("/Enhancements/ActiveDaemons"   ), enhancement, active  );
    merge_in(node->dep_daemons , get_set("/Requirements/InactiveDaemons" ), requirement, inactive);
    merge_in(node->dep_daemons , get_set("/Enhancements/InactiveDaemons" ), enhancement, inactive);

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
    for(const depinfo_t<std::string>& service : node->dep_services)
    {
      auto inverse_dep = dep_by_service.find(service.data);
      if(inverse_dep == dep_by_service.end())
        queueErrorMessage(service.data, "service." + active_string(service.is_active) + "." + required_string(service.is_required), "unresolved");
      else if(service.is_active)
        node->dep_nodes.emplace(depinfo_t<depnodeptr>{ service.is_required, service.is_active, inverse_dep->second });
      else
        inverse_dep->second->dep_nodes.emplace(depinfo_t<depnodeptr>{ service.is_required, service.is_active, node });
    }

    for(const depinfo_t<std::string>& daemon : node->dep_daemons)
    {
      auto inverse_dep = dep_by_daemon.find(daemon.data);
      if(inverse_dep == dep_by_daemon.end())
        queueErrorMessage(daemon.data, "daemon." + active_string(daemon.is_active) + "." + required_string(daemon.is_required), "unresolved");
      else if(daemon.is_active)
        node->dep_nodes.emplace(depinfo_t<depnodeptr>{ daemon.is_required, daemon.is_active, inverse_dep->second });
      else
        inverse_dep->second->dep_nodes.emplace(depinfo_t<depnodeptr>{ daemon.is_required, daemon.is_active, node });
    }
  }

//=== begin resolution ===

  // find depths of all daemons
  for(auto& dep : all_deps)
    m_dep_depths.emplace(dep, dep_depth(dep, depinfo_t<depnodeptr>{requirement, active, dep}, {}, true));

  // build a list of to start/stop daemons for each runlevel
  for(auto& dep : all_deps)
  {
    for(uint8_t rl : dep->runlevel_start)
      if(!recurse_add(m_orders_start[rl], dep, active)) // add all dependencies to the map of starting orders
        queueErrorMessage(dep->daemon_name, "runlevel." + std::to_string(rl) + ".active", "add failed");

    for(uint8_t rl : dep->runlevel_stop)
      if(!recurse_add(m_orders_stop[rl], dep, inactive)) // add all dependencies to the map of stopping orders
        queueErrorMessage(dep->daemon_name, "runlevel." + std::to_string(rl) + ".inactive", "add failed");
  }

  // destroy all cached data
  m_dep_depths.clear();
  for(auto& dep : all_deps)
    dep->dep_nodes.clear();
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

    auto order_start_iter = m_orders_start.find(data.runlevel); // find the data for the runlevel
    if(order_start_iter != m_orders_start.end()) // ensure that the data was found
      for(auto& pair : order_start_iter->second)
      {
        if(data.start.size() < posix::size_t(pair.first + 1))
          data.start.resize(pair.first + 1);
        data.start.at(pair.first).emplace_back(pair.second->daemon_name); // add daemon to ordered list
      }

    auto order_stop_iter = m_orders_stop.find(data.runlevel); // find the data for the runlevel
    if(order_stop_iter != m_orders_stop.end()) // ensure that the data was found
      for(auto& pair : order_stop_iter->second)
      {
        if(data.stop.size() < posix::size_t(pair.first + 1))
          data.stop.resize(pair.first + 1);
        data.stop.at(pair.first).emplace_back(pair.second->daemon_name); // add daemon to ordered list
      }
  }
  return data; // return ordered list of daemons to start/stop for this runlevel
}
