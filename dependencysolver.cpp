#include "dependencysolver.h"

// STL
#include <algorithm>

// Director
#include "string_helpers.h"

void DependencySolver::queueErrorMessage(const std::string& context, const std::string& source, const std::string& problem) noexcept
{
  m_errors.emplace_back(context);
  m_errors.back()
      .append("|").append(source)
      .append("|").append(problem);
}

constexpr const char* active_string(bool is_active) { return is_active ? "active" : "inactive"; }
constexpr const char* required_string(bool is_required) { return is_required ? "requirement" : "enhancement"; }

DependencySolver::depth_t DependencySolver::dependency_depth(depnodeptr origin,
                                                             depinfo_t<depnodeptr> dependency,
                                                             depinfoset_t<depnodeptr> path,
                                                             bool is_required) noexcept
{
  depth_t max_depth = 0;

  if(dependency.data == nullptr)
    return posix::error_response; // report unresolved dependency

  auto iter = m_dep_depths.find(dependency.data); // search for dep
  if(iter != m_dep_depths.end()) // if found
    return iter->second; // return previously calculated value

  if(!path.emplace(dependency).second) // if failed to add to path (because it exists)
    return posix::error_response; // report circular dependency

  for(const depinfo_t<depnodeptr>& ptr : dependency.data->dependencies) // for each dep dependency
  {
    depth_t depth = dependency_depth(origin, ptr, path, is_required && ptr.is_required); // get depth of dep dependency
    if(is_required && ptr.is_required && depth < 0) // if required and get an error response
      queueErrorMessage(origin->provider_name, dependency.data->provider_name + ".dependencies." + active_string(dependency.is_active) + ".requirement", "circular"); // report circular dependency because these dependencies are mandatory
    max_depth = std::max(max_depth, depth); // save the greater depth
  }

  return max_depth + 1; // return the deepest dependency depth (plus one for itself)
}

bool DependencySolver::recurse_add(runlevelorder_t& superset, depnodeptr dependency, bool is_active) const noexcept
{
  auto iter = m_dep_depths.find(dependency);
  if(iter == m_dep_depths.end())
    return false; // depth of dependency not found

  const depth_t& depth = iter->second;
  if(depth == posix::error_response)
    return false; // dependency has unmet dependencies

  if(!superset.emplace(depth, dependency).second)
    return false; // dependency already exists

  if(is_active)
  {
    for(const depinfo_t<depnodeptr>& subdependency : dependency->dependencies)
      if(!recurse_add(superset, subdependency.data, true))  // if have an unmet dependency
        return false; // fail
  }
  else
  {
    for(const depinfo_t<depnodeptr>& subdependency : dependency->dependencies)
    {
      runlevelorder_t subset = { { depth, dependency } };
      if(recurse_add(subset, subdependency.data, false)) // if subdependency has no unmet dependencies
        superset.insert(subset.begin(), subset.end()); // incorporate subset into superset
    }
  }

  return true; // all dependencies have been met!
}

void DependencySolver::resolveDependencies(void) noexcept
{
  constexpr bool active = true;
  constexpr bool inactive = false;
  constexpr bool requirement = true;
  constexpr bool enhancement = false;

  // temporary caches
  std::set<depnodeptr> all_deps;
  std::map<std::string, depnodeptr> dep_by_provider;
  std::map<std::string, depnodeptr> dep_by_service;

  // clear existing data
  m_orders_start.clear(); // previously ordered
  m_orders_stop.clear(); // previously ordered
  m_errors.clear(); // clear error messages

  // create node of each config
  for(const std::string& configname : getConfigList())
    dep_by_provider.emplace(configname, std::make_shared<depnode_t>());

  // process each config
  for(const std::string& configname : getConfigList())
  {
    auto get_set = [this, configname](const std::string& source)
                     { return clean_explode(getConfigData(configname, source), LIST_DELIM); };

    auto merge_in = [](depinfoset_t<std::string>& dest, const std::set<std::string> source, bool is_required, bool is_active)
    {
      for(const std::string& str : source)
        dest.emplace(depinfo_t<std::string>{is_required, is_active, str});
    };

    const depnodeptr& node = dep_by_provider.at(configname);
    all_deps.emplace(node);

    node->provider_name = configname;
    node->service_names = get_set("/Process/ProvidedServices");
    for(const std::string& service  : node->service_names)
      dep_by_service.emplace(service, node);

    merge_in(node->dep_services , get_set("/Requirements/ActiveServices"   ), requirement, active  );
    merge_in(node->dep_services , get_set("/Enhancements/ActiveServices"   ), enhancement, active  );
    merge_in(node->dep_services , get_set("/Requirements/InactiveServices" ), requirement, inactive);
    merge_in(node->dep_services , get_set("/Enhancements/InactiveServices" ), enhancement, inactive);

    merge_in(node->dep_providers, get_set("/Requirements/ActiveProviders"  ), requirement, active  );
    merge_in(node->dep_providers, get_set("/Enhancements/ActiveProviders"  ), enhancement, active  );
    merge_in(node->dep_providers, get_set("/Requirements/InactiveProviders"), requirement, inactive);
    merge_in(node->dep_providers, get_set("/Enhancements/InactiveProviders"), enhancement, inactive);

    for(const std::string& rl_name : get_set("/Requirements/StartOnRunLevels" ))
    {
      if(getRunlevelNumber(rl_name) == invalid_runlevel)
        queueErrorMessage(rl_name, "runlevel.start.requirement", "unresolved");
      else
        node->runlevel_number_start.emplace(getRunlevelNumber(rl_name)); // started on runlevel
    }

    for(const std::string& rl_name : get_set("/Requirements/StopOnRunLevels"))
    {
      if(getRunlevelNumber(rl_name) == invalid_runlevel)
        queueErrorMessage(rl_name, "runlevel.stop.requirement", "unresolved");
      else
        node->runlevel_number_stop.emplace(getRunlevelNumber(rl_name)); // started on runlevel
    }
  }

//=== post-processing ===
  for(const std::string& configname : getConfigList())
  {
    const depnodeptr& node = dep_by_provider.at(configname);
    for(const depinfo_t<std::string>& service : node->dep_services)
    {
      auto inverse_dependency_iter = dep_by_service.find(service.data);
      if(inverse_dependency_iter == dep_by_service.end())
        queueErrorMessage(service.data, std::string("service.") + active_string(service.is_active) + "." + required_string(service.is_required), "unresolved");
      else if(service.is_active)
        node->dependencies.emplace(depinfo_t<depnodeptr>{ service.is_required, service.is_active, inverse_dependency_iter->second });
      else
        inverse_dependency_iter->second->dependencies.emplace(depinfo_t<depnodeptr>{ service.is_required, service.is_active, node });
    }

    for(const depinfo_t<std::string>& provider : node->dep_providers)
    {
      auto inverse_dependency_iter = dep_by_provider.find(provider.data);
      if(inverse_dependency_iter == dep_by_provider.end())
        queueErrorMessage(provider.data, std::string("provider.") + active_string(provider.is_active) + "." + required_string(provider.is_required), "unresolved");
      else if(provider.is_active)
        node->dependencies.emplace(depinfo_t<depnodeptr>{ provider.is_required, provider.is_active, inverse_dependency_iter->second });
      else
        inverse_dependency_iter->second->dependencies.emplace(depinfo_t<depnodeptr>{ provider.is_required, provider.is_active, node });
    }
  }

//=== begin resolution ===

  // find depths of all providers
  for(const depnodeptr& dep : all_deps)
    m_dep_depths.emplace(dep, dependency_depth(dep, depinfo_t<depnodeptr>{requirement, active, dep}, {}, true));

  // build a list of to start/stop providers for each runlevel
  for(const depnodeptr& dep : all_deps)
  {
    for(runlevel_t rl : dep->runlevel_number_start)
      if(!recurse_add(m_orders_start[rl], dep, active)) // add all dependencies to the map of starting orders
        queueErrorMessage(dep->provider_name, "runlevel." + std::to_string(rl) + ".active", "add failed");

    for(runlevel_t rl : dep->runlevel_number_stop)
      if(!recurse_add(m_orders_stop[rl], dep, inactive)) // add all dependencies to the map of stopping orders
        queueErrorMessage(dep->provider_name, "runlevel." + std::to_string(rl) + ".inactive", "add failed");
  }

  // destroy all cached data
  m_dep_depths.clear();
  for(const depnodeptr& dep : all_deps)
    dep->dependencies.clear();
}

DependencySolver::runlevel_actions_t DependencySolver::getRunlevelOrder(const std::string& runlevel) const noexcept
{
  runlevel_actions_t data;
  runlevel_t runlevel_number = getRunlevelNumber(runlevel);
  if(runlevel_number != invalid_runlevel)
  {
    std::vector<std::list<std::string>> start; // NOTE: these are in order!
    std::vector<std::list<std::string>> stop;
    start.reserve(64);
    stop .reserve(64);

    auto order_start_iter = m_orders_start.find(runlevel_number); // find the data for the runlevel
    if(order_start_iter != m_orders_start.end()) // ensure that the data was found
      for(auto& pair : order_start_iter->second)
      {
        if(start.size() < posix::size_t(pair.first + 1))
          start.resize(posix::size_t(pair.first + 1));
        start.at(posix::size_t(pair.first)).emplace_back(pair.second->provider_name); // add provider to ordered list
      }

    auto order_stop_iter = m_orders_stop.find(runlevel_number); // find the data for the runlevel
    if(order_stop_iter != m_orders_stop.end()) // ensure that the data was found
      for(auto& pair : order_stop_iter->second)
      {
        if(stop.size() < posix::size_t(pair.first + 1))
          stop.resize(posix::size_t(pair.first) + 1);
        stop.at(posix::size_t(pair.first)).emplace_back(pair.second->provider_name); // add provider to ordered list
      }

    for(const std::list<std::string>& providers : stop)
      for(const std::string& provider : providers)
          data.emplace(false, provider);

    for(const std::list<std::string>& providers : start)
      for(const std::string& provider : providers)
          data.emplace(true, provider);
  }
  return data; // return ordered list of providers to start/stop for this runlevel
}
