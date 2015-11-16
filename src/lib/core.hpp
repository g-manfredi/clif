#ifndef _CLIF_CORE_H
#define _CLIF_CORE_H

#include <boost/filesystem.hpp>

#include "helpers.hpp"
#include "hdf5.hpp"
#include "enumtypes.hpp"


namespace clif {
  
using boost::filesystem::path;

enum class Interpolation : int {INVALID,NEAREST,LINEAR}; //TODO lanczos etc.
enum class Unit : int {INVALID,MM,PIXELS};

int baseType_size(BaseType t);

int combinedTypeElementCount(BaseType type, DataOrg org, DataOrder order);
int combinedTypePlaneCount(BaseType type, DataOrg org, DataOrder order);

H5::PredType H5PredType(BaseType type);
H5::PredType H5PredType_Native(BaseType type);

class Idx : public std::vector<int>
{
public:
  Idx();
  Idx(int size);
  
  static Idx zeroButOne(int size, int pos, int idx);
};

}

#endif