#ifndef _CLIF_HELPERS_H
#define _CLIF_HELPERS_H

#include <algorithm>
#include <string>
#include <boost/filesystem/path.hpp>

namespace clif {
  
template<typename T> T clamp(T v, T l, T u)
{
  return std::min<T>(u, std::max<T>(v, l));
}

std::string appendToPath(std::string str, std::string append);
std::string remove_last_part(std::string in, char c);
std::string get_last_part(std::string in, char c);
std::string get_first_part(std::string in, char c);

boost::filesystem::path get_abs_path(boost::filesystem::path path);
boost::filesystem::path remove_prefix(boost::filesystem::path path, boost::filesystem::path prefix);
bool has_prefix(boost::filesystem::path path, boost::filesystem::path prefix);

}

#endif