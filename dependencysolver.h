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

class DependencySolver
{
public:
  typedef uint32_t runlevel_t;
  constexpr static uint32_t invalid_runlevel = UINT32_MAX;

  struct start_stop_t
  {
    runlevel_t runlevel_number;
    std::vector<std::list<std::string>> start;
    std::vector<std::list<std::string>> stop;
  };

  inline virtual ~DependencySolver(void) noexcept = default;

  void resolveDependencies(void) noexcept;
  start_stop_t getRunlevelOrder(const std::string& runlevel) const noexcept;

  virtual const std::string& getConfigData(const std::string& config, const std::string& key) const noexcept = 0;
  virtual std::list<std::string> getConfigList(void) const noexcept = 0;
  virtual runlevel_t getRunlevelNumber(const std::string& rlname) const noexcept = 0;

  std::list<std::string> getErrorMessages(void) const noexcept { return m_errors; }

private:
  void queueErrorMessage(const std::string& context, const std::string& source, const std::string& problem) noexcept;
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
    std::string provider_name;
    std::set<std::string> service_names;

    depinfoset_t<std::string> dep_providers;
    depinfoset_t<std::string> dep_services;
    depinfoset_t<depnodeptr> dep_nodes; // cache

    std::set<runlevel_t> runlevel_number_start;
    std::set<runlevel_t> runlevel_number_stop;
  };

  std::map<depnodeptr, int32_t> m_dep_depths; // cache

  using runlevelorder_t = std::set<std::pair<posix::size_t, depnodeptr>>;
  std::map<runlevel_t, runlevelorder_t> m_orders_start; // the provider starting order by runlevel number
  std::map<runlevel_t, runlevelorder_t> m_orders_stop; // the provider stopping order by runlevel number

  int32_t dep_depth(depnodeptr origin, depinfo_t<depnodeptr> dep, depinfoset_t<depnodeptr> path, bool mandatory) noexcept;
  bool recurse_add(std::set<std::pair<posix::size_t, depnodeptr>>& subset, depnodeptr dep, bool active) const noexcept;
};

#endif // DEPENDENCYSOLVER_H
