#ifndef DIRECTORCONFIGCLIENT_H
#define DIRECTORCONFIGCLIENT_H

// STL
#include <atomic>
#include <vector>
#include <string>

// PUT
#include <put/socket.h>
#include <put/cxxutils/vfifo.h>
#include <put/cxxutils/posix_helpers.h>

#ifndef DIRECTOR_USERNAME
#define DIRECTOR_USERNAME       "director"
#endif

#ifndef CONFIG_USERNAME
#define CONFIG_USERNAME         "config"
#endif

#ifndef CONFIG_DIRECTOR_SOCKET
#define CONFIG_DIRECTOR_SOCKET      "/" CONFIG_USERNAME "/" DIRECTOR_USERNAME
#endif


class DirectorConfigClient : public ClientSocket
{
public:
  DirectorConfigClient(void) noexcept;

  std::list<std::string> listConfigs(void) const noexcept;
  const std::string& get(const std::string& config, const std::string& key) const noexcept;
  void set  (const std::string& config, const std::string& key, const std::string& value) noexcept;
  void unset(const std::string& config, const std::string& key) noexcept;

  bool isSynchronized(void) const noexcept { return m_sync; }
  signal<> synchronized;

  const std::unordered_map<std::string, std::string>& data(const std::string& config) const;
private:
  void resync(posix::error_t errcode) noexcept;
  void receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;
  void valueSet(const std::string& config, const std::string& key, const std::string& value) noexcept;
  void valueUnset(const std::string& config, const std::string& key) noexcept;

  std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_data;
  std::atomic_bool m_sync;
};

#endif
