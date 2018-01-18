#include "executorconfigclient.h"

// POSIX++
#include <climits>

// PDTK
#include <object.h>
#include <cxxutils/hashing.h>
#include <cxxutils/syslogstream.h>

#ifndef MCFS_PATH
#define MCFS_PATH               "/mc"
#endif

#ifndef EXECUTOR_USERNAME
#define EXECUTOR_USERNAME       "executor"
#endif

#ifndef CONFIG_USERNAME
#define CONFIG_USERNAME         "config"
#endif

#ifdef ANONYMOUS_SOCKET
#undef ANONYMOUS_SOCKET
#endif
#define ANONYMOUS_SOCKET        "\0"

#ifndef EXECUTOR_IO_SOCKET
#define EXECUTOR_IO_SOCKET      MCFS_PATH "/" EXECUTOR_USERNAME "/io"
#endif

#ifndef EXECUTOR_CONFIG_PATH
#define EXECUTOR_CONFIG_PATH  "/etc/executor"
#endif


#define NO_CONNECTION_TO_CONFIGURATION_DAEMON   0x10
#define UNABLE_TO_READ_CONFIGURATION_DIRECTORY  0x11
#define UNABLE_TO_READ_CONFIGURATION            0x11
#define UNABLE_TO_PARSE_CONFIGURATION           0x12

#ifndef NO_CONFIG_FALLBACK
#include <dirent.h>
#include <cxxutils/configmanip.h>

static const char* executor_configfilename(const char* base)
{
  // construct config filename
  static char name[PATH_MAX];
  std::memset(name, 0, PATH_MAX);
  if(std::snprintf(name, PATH_MAX, "%s/%s.conf", EXECUTOR_CONFIG_PATH, base) == posix::error_response) // I don't how this could fail
    return nullptr; // unable to build config filename
  return name;
}

static bool readconfig(const char* name, std::string& buffer)
{
  std::FILE* file = std::fopen(name, "a+b");

  if(file == nullptr)
  {
    posix::syslog << posix::priority::warning << "Unable to open file: " << name << " : " << std::strerror(errno) << posix::eom;
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

ExecutorConfigClient::ExecutorConfigClient(void) noexcept
{
  Object::connect(newMessage, this, &ExecutorConfigClient::receive);
  resync(posix::success_response);
}


void ExecutorConfigClient::resync(posix::error_t errcode) noexcept
{
  m_data.clear();
  m_sync = false;
  if(isConnected())
    disconnect();

  bool socket_file = use_socket_file(EXECUTOR_IO_SOCKET);

  bool try_connecting = true;
  if(errcode != posix::success_response)
  {
    // connection counter stuff here
  }

  if(try_connecting &&
     connect(socket_file ? EXECUTOR_IO_SOCKET : ANONYMOUS_SOCKET EXECUTOR_IO_SOCKET) &&
     write(vfifo("RPC", "syncCall"), posix::invalid_descriptor)) // no errors!
  {
  }
  else
  {
    if(try_connecting)
    {
      if(!isConnected())
        posix::syslog << posix::priority::warning << "Unable to connect to " << (socket_file ? "socket file" : "anonymous socket") << EXECUTOR_IO_SOCKET << posix::eom;
      else
        posix::syslog << posix::priority::warning << "Connection error for " << (socket_file ? "socket file" : "anonymous socket") << EXECUTOR_IO_SOCKET << ": " << std::strerror(errno) << posix::eom;
    }
#ifdef NO_CONFIG_FALLBACK
    Application::quit(NO_CONNECTION_TO_CONFIGURATION_DAEMON);
#else
    posix::syslog << posix::priority::warning << "Continuing without configuration daemon connection for Executor.  Falling back on direct file access." << posix::eom;

    std::string buffer;
    ConfigManip tmp_config;

    DIR* dir = ::opendir(EXECUTOR_CONFIG_PATH);
    dirent* entry = nullptr;
    char base[NAME_MAX];
    if(dir == nullptr)
    {
      posix::syslog << posix::priority::critical << "Unable to read directory of Executor configuation files: " << EXECUTOR_CONFIG_PATH << posix::eom;
      Application::quit(UNABLE_TO_READ_CONFIGURATION_DIRECTORY);
    }
    else
    {
      while((entry = ::readdir(dir)) != nullptr)
      {
        tmp_config.clear();
        if(!readconfig(executor_configfilename(base), buffer))
        {
          posix::syslog << posix::priority::critical << "Unable to read Executor daemon configuation file: " << executor_configfilename(base) << ": " << std::strerror(errno) << posix::eom;
          Application::quit(UNABLE_TO_READ_CONFIGURATION);
        }
        else if(!tmp_config.importText(buffer))
        {
          posix::syslog << posix::priority::critical << "Parsing failed will processing Executor daemon configuation file: " << executor_configfilename(base) << posix::eom;
          Application::quit(UNABLE_TO_PARSE_CONFIGURATION);
        }
        else // no errors! :)
          tmp_config.exportKeyPairs(m_data[base]);
      }
      m_sync = true;
    }
#endif
  }
}

void ExecutorConfigClient::valueSet(const std::string& config, const std::string& key, const std::string& value) noexcept
  { m_data[config][key] = value; }

void ExecutorConfigClient::valueUnset(const std::string& config, const std::string& key) noexcept
{
  auto configdata = m_data.find(config);
  if(configdata != m_data.end())
  {
    auto pos = configdata->second.begin();
    while(pos != configdata->second.end()) // search for key or children
    {
      if(pos->first.find(key) == 0) // key starts with search key (could be a child)
        pos = configdata->second.erase(pos); // delete it
      else
        ++pos;
    }
  }
}

std::list<std::string> ExecutorConfigClient::listConfigs(void) const noexcept
{
  std::list<std::string> names;
  for(const auto& pair : m_data)
    names.emplace_back(pair.first);
  return names;
}

const std::string& ExecutorConfigClient::get(const std::string& config, const std::string& key) const noexcept
{
  static std::string nullvalue;
  auto configdata = m_data.find(config);
  if(configdata == m_data.end())
    return nullvalue;
  auto keydata = configdata->second.find(key);
  if(keydata == configdata->second.end())
    return nullvalue;
  return keydata->second;
}

void ExecutorConfigClient::set(const std::string& config, const std::string& key, const std::string& value) noexcept
{
  valueSet(config, key, value);
  if(isConnected() && !write(vfifo("RPC", "setCall", config, key, value), posix::invalid_descriptor))
    Object::singleShot(this, &ExecutorConfigClient::resync, errno);
}

void ExecutorConfigClient::unset(const std::string& config, const std::string& key) noexcept
{
  valueUnset(config, key);
  if(isConnected() && !write(vfifo("RPC", "unsetCall", config, key), posix::invalid_descriptor))
    Object::singleShot(this, &ExecutorConfigClient::resync, errno);
}

void ExecutorConfigClient::receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept
{
  (void)socket;
  (void)fd;
  posix::error_t errcode;
  std::string str, config, key, value;
  if(!(buffer >> str).hadError() && str == "RPC" &&
     !(buffer >> str).hadError())
  {
    switch(hash(str))
    {
      case "syncReturn"_hash:
      {
        buffer >> errcode;
        if(buffer.hadError() || errcode != posix::success_response)
          Object::singleShot(this, &ExecutorConfigClient::resync, errcode);
        else if(errcode == posix::success_response)
          m_sync = true;
      }
      break;
      case "valueSet"_hash:
      {
        buffer >> config >> key >> value;
        if(buffer.hadError())
          Object::singleShot(this, &ExecutorConfigClient::resync, errcode);
        else
          valueSet(config, key, value);
      }
      break;
      case "valueUnset"_hash:
      {
        buffer >> config >> key;
        if(buffer.hadError())
          Object::singleShot(this, &ExecutorConfigClient::resync, errcode);
        else
          valueUnset(config, key);
      }
      break;
      case "unsetReturn"_hash:
      {
        buffer >> errcode >> config >> key;
        if(buffer.hadError() || errcode != posix::success_response)
          Object::singleShot(this, &ExecutorConfigClient::resync, errcode);
      }
      break;
      case "setReturn"_hash:
      {
        buffer >> errcode >> config >> key;
        if(buffer.hadError() || errcode != posix::success_response)
          Object::singleShot(this, &ExecutorConfigClient::resync, errcode);
      }
      break;
    }
  }
}
