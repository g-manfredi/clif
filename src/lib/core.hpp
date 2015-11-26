#ifndef _CLIF_CORE_H
#define _CLIF_CORE_H

#include <boost/filesystem.hpp>

#include "helpers.hpp"
#include "enumtypes.hpp"


namespace clif {
  
typedef boost::filesystem::path cpath;

enum class Interpolation : int {INVALID,NEAREST,LINEAR}; //TODO lanczos etc.
enum class Unit : int {INVALID,MM,PIXELS};

}

#endif