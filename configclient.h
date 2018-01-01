#ifndef CONFIGCLIENT_H
#define CONFIGCLIENT_H

// STL
#include <vector>
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

  bool fullUpdateCall (void)                                       const noexcept { return write(vfifo("RPC", "fullUpdateCall"       ), posix::invalid_descriptor); }
  bool setCall  (const std::string& key, const std::string& value) const noexcept { return write(vfifo("RPC", "setCall"  , key, value), posix::invalid_descriptor); }
  bool getCall  (const std::string& key                          ) const noexcept { return write(vfifo("RPC", "getCall"  , key       ), posix::invalid_descriptor); }
  bool unsetCall(const std::string& key                          ) const noexcept { return write(vfifo("RPC", "unsetCall", key       ), posix::invalid_descriptor); }


  signal<posix::error_t           /* errcode  */> fullUpdateReturn;
  signal<std::string              /* name     */,
         std::string              /* value    */> valueUpdate;
  signal<posix::error_t           /* errcode  */> unsetReturn;
  signal<posix::error_t           /* errcode  */> setReturn;
  signal<posix::error_t           /* errcode  */,
         std::string              /* value    */,
         std::vector<std::string> /* children */> getReturn;

private:
  void receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept;
};

#endif
