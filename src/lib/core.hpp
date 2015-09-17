#ifndef _CLIF_CORE_H
#define _CLIF_CORE_H

#include <boost/filesystem.hpp>

#include "helpers.hpp"
#include "hdf5.hpp"
#include "enumtypes.hpp"

enum class ClifUnit : int {INVALID,MM,PIXELS};

namespace clif {
  
using boost::filesystem::path;

enum class Interpolation : int {INVALID,NEAREST,LINEAR}; //TODO lanczos etc.

int baseType_size(BaseType t);

int combinedTypeElementCount(BaseType type, DataOrg org, DataOrder order);
int combinedTypePlaneCount(BaseType type, DataOrg org, DataOrder order);

H5::PredType H5PredType(BaseType type);

}

#endif