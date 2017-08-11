#include "executorconfigclient.h"

ExecutorConfigClient::ExecutorConfigClient(void) noexcept
{
  Object::connect(newMessage, this, &ExecutorConfigClient::receive);
}


void ExecutorConfigClient::configUpdated(std::string& name) noexcept
{
  (void)name;
}

void ExecutorConfigClient::unsetReturn(int errcode) noexcept
{
  (void)errcode;
}

void ExecutorConfigClient::setReturn(int errcode) noexcept
{
  (void)errcode;
}

void ExecutorConfigClient::getReturn(int errcode, std::string& value) noexcept
{
  (void)errcode;
  (void)value;
}


void ExecutorConfigClient::receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept
{
  (void)socket;
  (void)fd;
  std::string str;
  if(!(buffer >> str).hadError() && str == "RPC")
  {
    buffer >> str;
    switch(hash(str))
    {
      case "configUpdated"_hash:
      {
        struct { std::string name; } val;
        buffer >> val.name;
        if(!buffer.hadError())
          configUpdated(val.name);
      }
      break;
      case "unsetReturn"_hash:
      {
        struct { int errcode; } val;
        buffer >> val.errcode;
        if(!buffer.hadError())
          unsetReturn(val.errcode);
      }
      break;
      case "setReturn"_hash:
      {
        struct { int errcode; } val;
        buffer >> val.errcode;
        if(!buffer.hadError())
          setReturn(val.errcode);
      }
      break;
      case "getReturn"_hash:
      {
        struct { int errcode; std::string value; } val;
        buffer >> val.errcode >> val.value;
        if(!buffer.hadError())
          getReturn(val.errcode, val.value);
      }
      break;
    }
  }
}
