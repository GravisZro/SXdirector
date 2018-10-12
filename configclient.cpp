#include "configclient.h"

// POSIX++
#include <climits>

// PDTK
#include <object.h>
#include <cxxutils/hashing.h>
#include <cxxutils/syslogstream.h>

#ifndef SCFS_PATH
#define SCFS_PATH               "/svc"
#endif

#ifndef CONFIG_CONFIG_PATH
#define CONFIG_CONFIG_PATH      "/etc/" CONFIG_USERNAME
#endif

#ifndef DIRECTOR_CONFIG_FILE
#define DIRECTOR_CONFIG_FILE    "director.conf"
#endif


#define NO_CONNECTION_TO_CONFIGURATION_PROVIDER 0x10
#define UNABLE_TO_READ_CONFIGURATION            0x11
#define UNABLE_TO_PARSE_CONFIGURATION           0x12

#ifndef NO_CONFIG_FALLBACK
#include <cxxutils/configmanip.h>

static bool readconfig(const char* name, std::string& buffer) noexcept
{
  std::FILE* file = std::fopen(name, "rb");

  if(file == nullptr)
  {
    posix::syslog << posix::priority::warning
                  << "Unable to open file: %1 : %2"
                  << name
                  << std::strerror(errno)
                  << posix::eom;
    return false;
  }

  buffer.clear();
  buffer.resize(posix::size_t(std::ftell(file)), '\n');
  if(buffer.size())
  {
    std::rewind(file);
    std::fread(const_cast<char*>(buffer.data()), sizeof(std::string::value_type), buffer.size(), file);
  }
  std::fclose(file);
  return true;
}

#endif

ConfigClient::ConfigClient(void) noexcept
  : m_sync(false)
{
  Object::connect(newMessage, this, &ConfigClient::receive);
  Object::singleShot(this, &ConfigClient::resync, errno = posix::success_response);
}

void ConfigClient::resync(posix::error_t errcode) noexcept
{
  m_data.clear();
  m_sync = false;
  if(isConnected())
    disconnect();

  bool try_connecting = true;
  if(errcode != posix::success_response) // if not the first connection attempt
  {
    // connection counter stuff here
  }

  if(try_connecting &&
     connect(SCFS_PATH CONFIG_IO_SOCKET) &&
     write(vfifo("RPC", "syncCall"), posix::invalid_descriptor)) // no errors!
  {
  }
  else
  {
    if(try_connecting)
    {
      if(!isConnected())
        posix::syslog << posix::priority::warning
                      << "Unable to connect to socket file %1"
                      << SCFS_PATH CONFIG_IO_SOCKET
                      << posix::eom;
      else
        posix::syslog << posix::priority::warning
                      << "Connection error for %1 : %2"
                      << SCFS_PATH CONFIG_IO_SOCKET
                      << std::strerror(errno)
                      << posix::eom;
    }
#ifdef NO_CONFIG_FALLBACK
    Application::quit(NO_CONNECTION_TO_CONFIGURATION_PROVIDER);
#else
    posix::syslog << posix::priority::warning
                  << "Continuing without configuration provider connection.  Falling back on direct file access."
                  << posix::eom;

    std::string buffer;
    ConfigManip tmp_config;
    if(!readconfig(CONFIG_CONFIG_PATH "/" DIRECTOR_CONFIG_FILE, buffer))
    {
      posix::syslog << posix::priority::critical
                    << "Unable to read Director provider configuation file: %1 : %2"
                    << (CONFIG_CONFIG_PATH "/" DIRECTOR_CONFIG_FILE)
                    << std::strerror(errno)
                    << posix::eom;
    }
    else if(!tmp_config.importText(buffer))
    {
      posix::syslog << posix::priority::critical
                    << "Parsing failed will processing Director provider configuation file: %1"
                    << (CONFIG_CONFIG_PATH "/" DIRECTOR_CONFIG_FILE)
                    << posix::eom;
    }
    else // no errors! :)
    {
      tmp_config.exportKeyPairs(m_data);
      m_sync = true;
      Object::enqueue(synchronized);
    }
#endif
  }
}

void ConfigClient::valueSet(const std::string& key, const std::string& value) noexcept
{
  m_data[key] = value;
}

void ConfigClient::valueUnset(const std::string& key) noexcept
{
  auto pos = m_data.begin();
  while(pos != m_data.end()) // search for key or children
  {
    if(pos->first.find(key) == 0) // key starts with search key (could be a child)
      pos = m_data.erase(pos); // delete it
    else
      ++pos;
  }
}

const std::string& ConfigClient::get(const std::string& key) const noexcept
{
  static std::string nullvalue;
  auto iter = m_data.find(key);
  if(iter == m_data.end())
    return nullvalue;
  return iter->second;
}

void ConfigClient::set(const std::string& key, const std::string& value) noexcept
{
  valueSet(key, value);
  if(isConnected() && !write(vfifo("RPC", "setCall", key, value), posix::invalid_descriptor))
    Object::singleShot(this, &ConfigClient::resync, errno);
}

void ConfigClient::unset(const std::string& key) noexcept
{
  valueUnset(key);
  if(isConnected() && !write(vfifo("RPC", "unsetCall", key), posix::invalid_descriptor))
    Object::singleShot(this, &ConfigClient::resync, errno);
}

void ConfigClient::receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept
{
  (void)socket;
  (void)fd;
  posix::error_t errcode;
  std::string str, key, value;
  if(!(buffer >> str).hadError() && str == "RPC" &&
     !(buffer >> str).hadError())
  {
    switch(hash(str))
    {
      case "syncReturn"_hash:
      {
        buffer >> errcode;
        if(buffer.hadError() || errcode != posix::success_response)
          Object::singleShot(this, &ConfigClient::resync, errcode);
        else if(errcode == posix::success_response)
        {
          m_sync = true;
          Object::enqueue(synchronized);
        }
      }
      break;
      case "valueSet"_hash:
      {
        buffer >> key >> value;
        if(buffer.hadError())
          Object::singleShot(this, &ConfigClient::resync, errcode);
        else
          valueSet(key, value);
      }
      break;
      case "valueUnset"_hash:
      {
        buffer >> key;
        if(buffer.hadError())
          Object::singleShot(this, &ConfigClient::resync, errcode);
        else
          valueUnset(key);
      }
      break;
      case "unsetReturn"_hash:
      {
        buffer >> errcode >> key;
        if(buffer.hadError() || errcode != posix::success_response)
          Object::singleShot(this, &ConfigClient::resync, errcode);
      }
      break;
      case "setReturn"_hash:
      {
        buffer >> errcode >> key;
        if(buffer.hadError() || errcode != posix::success_response)
          Object::singleShot(this, &ConfigClient::resync, errcode);
      }
      break;
    }
  }
}
