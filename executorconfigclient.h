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

  bool listConfigsCall(void)                                        const noexcept { return write(vfifo("RPC", "listConfigsCall"      ), posix::invalid_descriptor); }
  bool setCall  (const std::string& key, const std::string& value ) const noexcept { return write(vfifo("RPC", "setCall"  , key, value), posix::invalid_descriptor); }
  bool getCall  (const std::string& key                           ) const noexcept { return write(vfifo("RPC", "getCall"  , key       ), posix::invalid_descriptor); }
  bool unsetCall(const std::string& key                           ) const noexcept { return write(vfifo("RPC", "unsetCall", key       ), posix::invalid_descriptor); }

  signal<std::list<std::string>& /* names    */> listConfigsReturn;
  signal<std::string&            /* name     */> configUpdated;
  signal<posix::error_t          /* errcode  */> unsetReturn;
  signal<posix::error_t          /* errcode  */> setReturn;
  signal<posix::error_t          /* errcode  */,
         std::string&            /* value    */,
         std::list<std::string>& /* children */> getReturn;

private:
  void receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;
};

#endif
