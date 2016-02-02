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

template<typename T> class get_intensities_dispatcher {
public:
  void operator()(const Mat* m, Mat* intensities, int x, int y, float disp)
  {
	for(int v = 0; v<m[3];v++)
	{
		float x_real = x + (m[3]/2] - v)*disp;
		int x_l = x_real;
		int x_r = x_l + 1;
		float f = r_real - x_l;
		if (x_l < 0 and x_r >= m[0])
			continue;
		for (int c=0; c<3; c++ )	
			intensities(v,c) = m(x_l, y, c, v)*(1-f) + m(x_r, y, c, v)*f; 
		
	}    
  }
};

Void get_intensities(const Mat& m, Mat& intensities, int x, int y, float disp)
{
	intensities.create(m.type(), {m[3],m[2]});
	m.callIf<get_intensities_dispatcher,_is_convertible_to_float>(&m,&intensities,x,y,disp);
}
}
