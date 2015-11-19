#ifndef _CLIF_CORE_H
#define _CLIF_CORE_H

#include <boost/filesystem.hpp>

#include "helpers.hpp"
#include "enumtypes.hpp"


namespace clif {
  
using boost::filesystem::path;

enum class Interpolation : int {INVALID,NEAREST,LINEAR}; //TODO lanczos etc.
enum class Unit : int {INVALID,MM,PIXELS};


//int combinedTypeElementCount(BaseType type, DataOrg org, DataOrder order);
//int combinedTypePlaneCount(BaseType type, DataOrg org, DataOrder order);

}

#endif