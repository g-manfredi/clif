#include "core.hpp"

namespace clif {

template<typename T> class basetype_size_dispatcher {
public:
  int operator()()
  {
    return sizeof(T);
  }
};

int baseType_size(BaseType t)
{
  return callByBaseType<basetype_size_dispatcher,int>(t);
}

int combinedTypeElementCount(BaseType type, DataOrg org, DataOrder order)
{
  switch (org) {
    case DataOrg::PLANAR : return 1;
    case DataOrg::INTERLEAVED :
      switch (order) {
        case DataOrder::RGB : return 3;
        default:
          abort();
      }
    case DataOrg::BAYER_2x2 : return 1;
    default :
      abort();
  }
}

int combinedTypePlaneCount(BaseType type, DataOrg org, DataOrder order)
{
  switch (org) {
    case DataOrg::PLANAR :
      switch (order) {
        case DataOrder::RGB   : return 3;
        default:
          abort();
      }
    case DataOrg::INTERLEAVED : return 1;
    case DataOrg::BAYER_2x2   : return 1;
  }
}

//FIXME other types!
H5::PredType H5PredType(BaseType type)
{
  switch (type) {
    case BaseType::UINT8 : return H5::PredType::STD_U8LE;
    case BaseType::UINT16 : return H5::PredType::STD_U16LE;
    default :
      assert(type != BaseType::UINT16);
      abort();
  }
}

}