#ifndef DEPENDENCYSOLVER_H
#define DEPENDENCYSOLVER_H

// STL
#include <string>
#include <map>
#include <list>
#include <vector>
#include <set>
#include <memory>
#include <functional>

// PDTK
#include <cxxutils/posix_helpers.h>

struct start_stop_t
{
  uint8_t runlevel;
  std::vector<std::list<std::string>> start;
  std::vector<std::list<std::string>> stop;
};

class DependencySolver
{
public:
  void resolveDependencies(void) noexcept;
  start_stop_t getRunlevelOrder(const std::string& runlevel) const noexcept;

  virtual const std::string& getConfigData(const std::string& config, const std::string& key) const noexcept = 0;
  virtual std::list<std::string> getConfigList(void) const noexcept = 0;
  virtual int getRunlevel(const std::string& rlname) const noexcept = 0;

  std::list<std::string> getErrorMessages(void) const noexcept { return m_errors; }

private:
  int queueErrorMessage(const std::string& context, const std::string& source, const std::string& problem) noexcept;
  std::list<std::string> m_errors;

  struct depnode_t;
  using depnodeptr = std::shared_ptr<depnode_t>;

  template <typename T>
  struct depinfo_t
  {
    bool is_required;
    bool is_active;
    T data;
    bool operator <(const depinfo_t& other) const noexcept { return data < other.data; }
  };

  template <typename T>
  using depinfoset_t = std::set<depinfo_t<T>>;

  struct depnode_t
  {
    std::string daemon_name;
    std::set<std::string> service_names;

    depinfoset_t<std::string> dep_daemons;
    depinfoset_t<std::string> dep_services;
    depinfoset_t<depnodeptr> dep_nodes; // cache

    std::set<uint8_t> runlevel_start;
    std::set<uint8_t> runlevel_stop;
  };

  std::map<depnodeptr, int> m_dep_depths; // cache

  using runlevelorder_t = std::set<std::pair<int, depnodeptr>>;
  std::map<uint8_t, runlevelorder_t> m_orders_start; // the daemon starting order by runlevels
  std::map<uint8_t, runlevelorder_t> m_orders_stop; // the daemon stopping order by runlevels

  int dep_depth(depnodeptr origin, depinfo_t<depnodeptr> dep, depinfoset_t<depnodeptr> path, bool mandatory) noexcept;
  bool recurse_add(std::set<std::pair<int, depnodeptr>>& subset, depnodeptr dep, bool active) noexcept;
};

#endif // DEPENDENCYSOLVER_H
