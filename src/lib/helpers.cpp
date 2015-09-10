#include "helpers.hpp"

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

}