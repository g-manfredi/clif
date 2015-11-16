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

//FIXME other types!
H5::PredType H5PredType(BaseType type)
{
  switch (type) {
    case BaseType::UINT8 : return H5::PredType::STD_U8LE;
    case BaseType::UINT16 : return H5::PredType::STD_U16LE;
    case BaseType::FLOAT : return H5::PredType::IEEE_F32LE;
    default :
      assert(type != BaseType::UINT16);
      abort();
  }
}

//FIXME other types!
H5::PredType H5PredType_Native(BaseType type)
{
  switch (type) {
    case BaseType::UINT8 : return H5::PredType::NATIVE_UINT8;
    case BaseType::UINT16 : return H5::PredType::NATIVE_UINT16;
    case BaseType::FLOAT : return H5::PredType::NATIVE_FLOAT;
    default :
      assert(type != BaseType::UINT16);
      abort();
  }
}


Idx::Idx() : std::vector<int>() {};
Idx::Idx(int size) : std::vector<int>(size) {};

static Idx zeroButOne(int size, int pos, int i)
{
  Idx idx = Idx(size);
  idx[pos] = i;
}

}