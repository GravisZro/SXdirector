#ifndef DEPENDENCYSOLVER_H
#define DEPENDENCYSOLVER_H

// STL
#include <string>
#include <map>
#include <list>
#include <set>
#include <memory>
#include <functional>

// PDTK
#include <cxxutils/posix_helpers.h>

struct start_stop_t
{
  uint8_t runlevel;
  std::list<std::string> start;
  std::list<std::string> stop;
};

class DependencySolver
{
public:
  void resolveDependencies(void) noexcept;
  start_stop_t getRunlevelOrder(const std::string& runlevel) const noexcept;

  virtual const std::string& getConfigData(const std::string& config, const std::string& key) const noexcept = 0;
  virtual std::list<std::string> getConfigList(void) const noexcept = 0;
  virtual int getRunlevel(const std::string& rlname) const noexcept = 0;

private:
  struct depnode_t;
  using depnodeptr = std::shared_ptr<depnode_t>;
  template <typename T>
  struct depsets_t
  {
    std::set<T> requirement;
    std::set<T> enhancement;
  };

  template <typename T>
  struct activity_t
  {
    T active;
    T inactive;
  };

  struct depnode_t
  {
    std::string daemon_name;
    std::set<std::string> service_names;

    activity_t<depsets_t<std::string>> dep_daemons;
    activity_t<depsets_t<std::string>> dep_services;
    activity_t<depsets_t<depnodeptr>> dep_nodes;

    std::set<uint8_t> runlevel_start;
    std::set<uint8_t> runlevel_stop;
  };

  std::set<depnodeptr> m_all_deps;
  std::map<std::string, depnodeptr> m_dep_by_daemon;
  std::map<std::string, depnodeptr> m_dep_by_service;
  std::map<depnodeptr, int> m_dep_depths;

  std::map<uint8_t, activity_t<std::set<std::pair<int, depnodeptr>>>> m_orders; // the daemon starting order by runlevels

  int dep_depth(depnodeptr dep, std::set<depnodeptr> activepath, std::set<depnodeptr> inactivepath) noexcept;
  bool recurse_add(std::set<std::pair<int, depnodeptr>>& subset, depnodeptr dep, std::function<depsets_t<depnodeptr>&(depnodeptr)>& selector) const noexcept;
};

#endif // DEPENDENCYSOLVER_H
