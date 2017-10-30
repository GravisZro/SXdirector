#include "executorconfigclient.h"

ExecutorConfigClient::ExecutorConfigClient(void) noexcept
{
  Object::connect(newMessage, this, &ExecutorConfigClient::receive);
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
      case "listConfigsReturn"_hash:
      {
        struct { std::vector<std::string> names; } val;
        buffer >> val.names;
        if(!buffer.hadError())
          Object::enqueue(listConfigsReturn, val.names);
      }
      break;
      case "configUpdated"_hash:
      {
        struct { std::string name; } val;
        buffer >> val.name;
        if(!buffer.hadError())
          Object::enqueue(configUpdated, val.name);
      }
      break;
      case "unsetReturn"_hash:
      {
        struct { int errcode; } val;
        buffer >> val.errcode;
        if(!buffer.hadError())
          Object::enqueue(unsetReturn, val.errcode);
      }
      break;
      case "setReturn"_hash:
      {
        struct { int errcode; } val;
        buffer >> val.errcode;
        if(!buffer.hadError())
          Object::enqueue(setReturn, val.errcode);
      }
      break;
      case "getReturn"_hash:
      {
        struct { int errcode; std::string value; std::vector<std::string> children; } val;
        buffer >> val.errcode >> val.value >> val.children;
        if(!buffer.hadError())
          Object::enqueue(getReturn, val.errcode, val.value, val.children);
      }
      break;
    }
  }
}
