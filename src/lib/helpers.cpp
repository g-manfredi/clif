#include "helpers.hpp"

#include <boost/filesystem.hpp>

#include "core.hpp"
using namespace boost::filesystem;

namespace clif {

std::string appendToPath(std::string str, std::string append)
{
  if (str.back() != '/')
    str = str.append("/");
    
  return str.append(append);
}


std::string remove_last_part(std::string in, char c)
{
  size_t pos = in.rfind(c);
  
  if (pos == std::string::npos)
    return std::string("");
  
  return in.substr(0, pos);
}

std::string get_last_part(std::string in, char c)
{
  size_t pos = in.rfind(c);
  
  if (pos == std::string::npos)
    return in;
  
  return in.substr(pos+1, in.npos);
}

std::string get_first_part(std::string in, char c)
{
  size_t pos = in.find(c);
  
  if (pos == std::string::npos)
    return in;
  
  return in.substr(0, pos);
}

boost::filesystem::path get_abs_path(boost::filesystem::path path)
{
  if (path.is_absolute())
    return path;
  else
    return current_path() / path;
}

boost::filesystem::path remove_prefix(boost::filesystem::path path, boost::filesystem::path prefix)
{
  boost::filesystem::path res;
  
  auto it_path = path.begin();
  auto it_prefix = prefix.begin();
  
  while (*it_path == *it_prefix) {
    ++it_path;
    ++it_prefix;
  }
  
  while (it_path != path.end()) {
    res /= *it_path;
    ++it_path;
  }
  
  return res;
}

bool has_prefix(cpath path, cpath prefix)
{
  auto it_path = path.begin();
  auto it_prefix = prefix.begin();
  
  while (it_prefix != prefix.end() && *it_path == *it_prefix) {
    ++it_path;
    ++it_prefix;
  }
  
  if (it_prefix == prefix.end())
    return true;
  
  return false;
}

}
