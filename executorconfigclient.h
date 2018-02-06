#ifndef EXECUTORCONFIGCLIENT_H
#define EXECUTORCONFIGCLIENT_H

// STL
#include <atomic>
#include <vector>
#include <string>

// PDTK
#include <socket.h>
#include <cxxutils/vfifo.h>
#include <cxxutils/posix_helpers.h>

class ExecutorConfigClient : public ClientSocket
{
public:
  ExecutorConfigClient(void) noexcept;

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
