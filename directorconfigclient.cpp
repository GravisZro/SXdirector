#include "directorconfigclient.h"

// POSIX++
#include <climits>
#include <cstdio>

// PDTK
#include <object.h>
#include <cxxutils/hashing.h>
#include <cxxutils/syslogstream.h>

#ifndef SCFS_PATH
#define SCFS_PATH               "/svc"
#endif

#ifndef DIRECTOR_CONFIG_DIR
#define DIRECTOR_CONFIG_DIR     "/etc/" DIRECTOR_USERNAME
#endif


#define NO_CONNECTION_TO_CONFIGURATION_PROVIDER 0x10
#define UNABLE_TO_READ_CONFIGURATION_DIRECTORY  0x11
#define UNABLE_TO_READ_CONFIGURATION            0x11
#define UNABLE_TO_PARSE_CONFIGURATION           0x12

#ifndef NO_CONFIG_FALLBACK
#include <dirent.h>
#include <cxxutils/configmanip.h>


static const char* extract_provider_name(const char* filename)
{
  char provider[NAME_MAX] = { 0 };
  const char* start = posix::strrchr(filename, '/');
  const char* end   = posix::strrchr(filename, '.');

  if(start == nullptr || // if '/' NOT found OR
     end   == nullptr || // '.' found AND
     end < start || // occur in the incorrect order OR
     posix::strcmp(end, ".conf")) // doesn't end with ".conf"
    return nullptr;
  return posix::strncpy(provider, start + 1, posix::size_t(end - start + 1)); // extract provider name
}

static const char* director_configfilename(const char* filename)
{
  // construct config filename
  static char fullpath[PATH_MAX];
  posix::memset(fullpath, 0, PATH_MAX);
  if(posix::snprintf(fullpath, PATH_MAX, "%s/%s", DIRECTOR_CONFIG_DIR, filename) == posix::error_response) // I don't how this could fail
    return nullptr; // unable to build config filename
  return fullpath;
}

static bool readconfig(const char* name, std::string& buffer)
{
  posix::FILE* file = posix::fopen(name, "rb");

  if(file == nullptr)
  {
    posix::syslog << posix::priority::warning
                  << "Unable to open file: %1 : %2"
                  << name
                  << posix::strerror(errno)
                  << posix::eom;
    return false;
  }

  buffer.clear();
  buffer.resize(posix::size_t(posix::ftell(file)), '\n');
  if(buffer.size())
  {
    posix::rewind(file);
    posix::fread(const_cast<char*>(buffer.data()), sizeof(std::string::value_type), buffer.size(), file);
  }
  posix::fclose(file);
  return true;
}
#endif

DirectorConfigClient::DirectorConfigClient(void) noexcept
  : m_sync(false)
{
  Object::connect(newMessage, this, &DirectorConfigClient::receive);
  Object::singleShot(this, &DirectorConfigClient::resync, errno = posix::success_response);
}

const std::unordered_map<std::string, std::string>& DirectorConfigClient::data(const std::string& config) const
{
  static const std::unordered_map<std::string, std::string> nullval;
  auto pos = m_data.find(config);
  if(pos == m_data.end())
    return nullval;
  return pos->second;
}

void DirectorConfigClient::resync(posix::error_t errcode) noexcept
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
     connect(SCFS_PATH CONFIG_DIRECTOR_SOCKET) &&
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
                      << SCFS_PATH CONFIG_DIRECTOR_SOCKET
                      << posix::eom;
      else
        posix::syslog << posix::priority::warning
                      << "Connection error for socket file %1 : %2"
                      << SCFS_PATH CONFIG_DIRECTOR_SOCKET
                      << posix::strerror(errno)
                      << posix::eom;
    }
#ifdef NO_CONFIG_FALLBACK
    Application::quit(NO_CONNECTION_TO_CONFIGURATION_PROVIDER);
#else
    posix::syslog << posix::priority::warning
                  << "Continuing without configuration provider connection for Director.  Falling back on direct file access."
                  << posix::eom;

    DIR* dir = ::opendir(DIRECTOR_CONFIG_DIR);
    dirent* entry = nullptr;
    const char* provider = nullptr;
    const char* filename = nullptr;
    if(dir == nullptr)
    {
      posix::syslog << posix::priority::critical
                    << "Unable to read directory of Director configuation files: %1"
                    << DIRECTOR_CONFIG_DIR
                    << posix::eom;
    }
    else
    {
      std::string buffer;
      ConfigManip tmp_config;

      while((entry = ::readdir(dir)) != nullptr)
      {
        if(entry->d_name[0] == '.') // skip dot files/dirs
          continue;

        if((provider = extract_provider_name  (entry->d_name)) == nullptr || // if provider name extraction failed OR
           (filename = director_configfilename(entry->d_name)) == nullptr) // failed to build filename
          continue; // skip file

        tmp_config.clear();
        if(!readconfig(filename, buffer))
        {
          posix::syslog << posix::priority::critical
                        << "Unable to read Director configuation file: %1 : %2"
                        << filename
                        << posix::strerror(errno)
                        << posix::eom;
        }
        else if(!tmp_config.importText(buffer))
        {
          posix::syslog << posix::priority::critical
                        << "Parsing failed will processing Director configuation file: %1"
                        << filename
                        << posix::eom;
        }

        tmp_config.exportKeyPairs(m_data[provider]);
      }
      m_sync = true;
      Object::enqueue(synchronized);
    }
#endif
  }
}

void DirectorConfigClient::valueSet(const std::string& config, const std::string& key, const std::string& value) noexcept
  { m_data[config][key] = value; }

void DirectorConfigClient::valueUnset(const std::string& config, const std::string& key) noexcept
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

std::list<std::string> DirectorConfigClient::listConfigs(void) const noexcept
{
  std::list<std::string> names;
  for(const auto& pair : m_data)
    names.emplace_back(pair.first);
  return names;
}

const std::string& DirectorConfigClient::get(const std::string& config, const std::string& key) const noexcept
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

void DirectorConfigClient::set(const std::string& config, const std::string& key, const std::string& value) noexcept
{
  valueSet(config, key, value);
  if(isConnected() && !write(vfifo("RPC", "setCall", config, key, value), posix::invalid_descriptor))
    Object::singleShot(this, &DirectorConfigClient::resync, errno);
}

void DirectorConfigClient::unset(const std::string& config, const std::string& key) noexcept
{
  valueUnset(config, key);
  if(isConnected() && !write(vfifo("RPC", "unsetCall", config, key), posix::invalid_descriptor))
    Object::singleShot(this, &DirectorConfigClient::resync, errno);
}

void DirectorConfigClient::receive(posix::fd_t socket, vfifo buffer, posix::fd_t fd) noexcept
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
          Object::singleShot(this, &DirectorConfigClient::resync, errcode);
        else if(errcode == posix::success_response)
        {
          m_sync = true;
          Object::enqueue(synchronized);
        }
      }
      break;
      case "valueSet"_hash:
      {
        buffer >> config >> key >> value;
        if(buffer.hadError())
          Object::singleShot(this, &DirectorConfigClient::resync, errcode);
        else
          valueSet(config, key, value);
      }
      break;
      case "valueUnset"_hash:
      {
        buffer >> config >> key;
        if(buffer.hadError())
          Object::singleShot(this, &DirectorConfigClient::resync, errcode);
        else
          valueUnset(config, key);
      }
      break;
      case "unsetReturn"_hash:
      {
        buffer >> errcode >> config >> key;
        if(buffer.hadError() || errcode != posix::success_response)
          Object::singleShot(this, &DirectorConfigClient::resync, errcode);
      }
      break;
      case "setReturn"_hash:
      {
        buffer >> errcode >> config >> key;
        if(buffer.hadError() || errcode != posix::success_response)
          Object::singleShot(this, &DirectorConfigClient::resync, errcode);
      }
      break;
    }
  }
}
