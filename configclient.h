
#ifndef CONFIGCLIENT_H
#define CONFIGCLIENT_H

// STL
#include <list>
#include <string>

// POSIX
#include <sys/types.h>

// PDTK
#include <socket.h>
#include <cxxutils/vfifo.h>
#include <cxxutils/posix_helpers.h>

class ConfigClient : public ClientSocket
{
public:
  ConfigClient(void) noexcept;

  bool setCall  (const std::string& key, const std::string& value) const noexcept { return write(vfifo("RPC", "setCall"  , key, value), posix::invalid_descriptor); }
  bool getCall  (const std::string& key                          ) const noexcept { return write(vfifo("RPC", "getCall"  , key       ), posix::invalid_descriptor); }
  bool unsetCall(const std::string& key                          ) const noexcept { return write(vfifo("RPC", "unsetCall", key       ), posix::invalid_descriptor); }

private:
  void configUpdated(void) noexcept;
  void unsetReturn(int errcode) noexcept;
  void setReturn  (int errcode) noexcept;
  void getReturn  (int errcode, std::string& value, std::list<std::string>& children) noexcept;

  void receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;
  void request(posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept;
};

#endif
