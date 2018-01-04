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
  if(!(buffer >> str).hadError() && str == "RPC" &&
     !(buffer >> str).hadError())
  {
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
      case "fullUpdateReturn"_hash:
      {
        struct { posix::error_t errcode; } val;
        buffer >> val.errcode;
        if(!buffer.hadError())
          Object::enqueue(fullUpdateReturn, val.errcode);
      }
      break;
      case "valueUpdate"_hash:
      {
        struct { std::string config; std::string key; std::string value; } val;
        buffer >> val.config >> val.key >> val.value;
        if(!buffer.hadError())
          Object::enqueue(valueUpdate, val.config, val.key, val.value);
      }
      break;
      case "valueUnset"_hash:
      {
        struct { std::string config; std::string key; } val;
        buffer >> val.config >> val.key;
        if(!buffer.hadError())
          Object::enqueue(valueUnset, val.config, val.key);
      }
      break;
      case "unsetReturn"_hash:
      {
        struct { posix::error_t errcode; std::string config; std::string key; } val;
        buffer >> val.errcode >> val.config >> val.key;
        if(!buffer.hadError())
          Object::enqueue(unsetReturn, val.errcode, val.config, val.key);
      }
      break;
      case "setReturn"_hash:
      {
        struct { posix::error_t errcode; std::string config; std::string key; } val;
        buffer >> val.errcode >> val.config >> val.key;
        if(!buffer.hadError())
          Object::enqueue(setReturn, val.errcode, val.config, val.key);
      }
      break;
      case "getReturn"_hash:
      {
        struct { posix::error_t errcode; std::string config; std::string key; std::string value; std::vector<std::string> childlist; } val;
        buffer >> val.errcode >> val.config >> val.key >> val.value >> val.childlist;
        if(!buffer.hadError())
          Object::enqueue(getReturn, val.errcode, val.config, val.key, val.value, val.childlist);
      }
      break;
    }
  }
}
