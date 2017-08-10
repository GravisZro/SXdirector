#ifndef EXECUTORCONFIGCLIENT_H
#define EXECUTORCONFIGCLIENT_H

// POSIX
#include <sys/types.h>

// STL
#include <vector>
#include <cstdint>

// PDTK
#include <object.h>
#include <socket.h>
#include <cxxutils/vfifo.h>
#include <cxxutils/hashing.h>
#include <cxxutils/posix_helpers.h>

class ExecutorConfigClient : public ClientSocket
{
public:
  ExecutorConfigClient(void) noexcept;

  bool setCall  (const std::string& key, const std::string& value ) const noexcept { return write(vfifo("RPC", "setCall"  , key, value), posix::invalid_descriptor); }
  bool getCall  (const std::string& key                           ) const noexcept { return write(vfifo("RPC", "getCall"  , key       ), posix::invalid_descriptor); }
  bool unsetCall(const std::string& key                           ) const noexcept { return write(vfifo("RPC", "unsetCall", key       ), posix::invalid_descriptor); }

private:
  void configUpdated(std::string& name) noexcept;
  void unsetReturn (int errcode) noexcept;
  void setReturn   (int errcode) noexcept;
  void getReturn   (int errcode, std::string& value) noexcept;

  void receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;
  void request(posix::fd_t socket, posix::sockaddr_t addr, proccred_t cred) noexcept;
};

#endif
