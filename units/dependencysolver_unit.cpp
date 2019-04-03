#ifndef DEMO_H
#define DEMO_H

#include <list>
#include <map>
#include <string>

#include "dependencysolver.h"

class DepDemo : public DependencySolver
{
private:
  std::map<std::string, uint8_t> m_runlevel_aliases;
  std::map<std::string, std::map<std::string, std::string>> m_config_data;
public:
  DepDemo(const std::map<std::string, uint8_t>& runlevels,
          const std::map<std::string, std::map<std::string, std::string>>& config_data)
    : m_runlevel_aliases(runlevels),
      m_config_data(config_data)
  {}


  const std::string& getConfigData(const std::string& config, const std::string& key) const noexcept
  {
    static const std::string bad;
    auto iter = m_config_data.find(config);
    if(iter != m_config_data.end())
    {
      auto subiter = iter->second.find(key);
      if(subiter != iter->second.end())
        return subiter->second;
    }
    return bad;
  }

  std::list<std::string> getConfigList(void) const noexcept
  {
    std::list<std::string> config_list;
    for(const std::pair<std::string, std::map<std::string, std::string>> & pair : m_config_data)
      config_list.push_back(pair.first);
    return config_list;
  }

  int getRunlevel(const std::string& rlname) const noexcept
  {
    auto iter = m_runlevel_aliases.find(rlname);
    if(iter == m_runlevel_aliases.end())
      return -1;
    return iter->second;
  }
};

#include <iostream>

int main(int argc, char *argv[]) noexcept
{
  DepDemo demo( { { "0", 0 }, { "1", 1 }, { "2", 2 }, { "3", 3 }, { "4", 4 }, { "5", 5 }, { "6", 6 } },
                {
                  { "director",
                    {
                      {"/Requirements/StartOnRunLevels", "1"},
                      {"/Enhancements/ActiveServices", "config/io, config/director"},
                    }
                  },
                  { "demo1",
                    {
                      {"/Requirements/StartOnRunLevels", "2"},
                      {"/Enhancements/ActiveServices", "config/io"},
                      {"/Enhancements/ActiveDaemons", "director"},
                    }
                  },
                  { "demo2",
                    {
                      {"/Requirements/StartOnRunLevels", "1,2"},
                      {"/Requirements/ActiveServices", "config/io"},
                      {"/Requirements/InactiveDaemons", "demo1"},
                    }
                  },
                  { "config",
                    {
                      {"/Requirements/StartOnRunLevels", "1"},
                      {"/Process/ProvidedServices", "config/io, config/director"},
                      {"/Enhancements/ActiveDaemons", "demo1"},
                    }
                  },
                });

  demo.resolveDependencies();

  for(const std::string& err : demo.getErrorMessages())
    std::cout << "resolution error: " << err << std::endl;

  for(std::string rl_alias : { "0", "1", "2"})
  {
    //DependencySolver::
        start_stop_t data = demo.getRunlevelOrder(rl_alias);

    std::cout << "runlevel: " << rl_alias << std::endl;

    std::cout << "start:" << std::endl;
    std::cout << "<list>";
    for(const std::list<std::string>& list : data.start)
      for(const std::string& info : list)
        std::cout << " " << info;
    std::cout << " </list>" << std::endl;

    std::cout << "stop:" << std::endl;
    std::cout << "<list>";
    for(const std::list<std::string>& list : data.stop)
      for(const std::string& info : list)
        std::cout << " " << info;
//    for(const DependencySolver::depinfoset_t<std::string>& set : data.stop)
//      for(const DependencySolver::depinfo_t<std::string>& info : set)
//        std::cout << " " << info.data;
    std::cout << " </list>" << std::endl;
  }

  std::cout << std::flush;
  sleep(1);
  return 0;
}

#endif // DEMO_H
