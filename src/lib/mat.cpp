#include "mat.hpp"

#include "enumtypes.hpp"

namespace clif {
  
typedef unsigned int uint;
 
Idx::Idx() : std::vector<int>() {};

Idx::Idx(int size) : std::vector<int>(size) {};

off_t Idx::total()
{
  off_t t = 1;
  
  if (!size())
    return 0;
  
  for(uint i=0;i<size();i++)
    t *= operator[](i);

  return t;
}

Idx Idx::zeroButOne(int size, int pos, int i)
{
  Idx idx = Idx(size);
  idx[pos] = i;
  
  return idx;
}

template<typename T> class destruction_dispatcher {
public:
  void operator()(void *ptr, off_t count)
  {
    printf("FIXME: destruct array elements!\n");
    //delete[]...
  }
};

template<typename T> class creation_dispatcher {
public:
  void operator()(void *ptr, off_t count)
  {
    new(ptr) T[count]();
  }
};

void *BaseType_new(BaseType type, off_t count)
{
  void *ptr = malloc(count*baseType_size(type));
  
  if (type > BaseTypeMaxAtomicType)
    callByBaseType<creation_dispatcher>(type, ptr, count);
  
  return ptr;
}

struct BaseType_deleter
{
  BaseType_deleter(off_t count, BaseType type = BaseType::INVALID) : _count(count), _type(type) {};
  
  void operator()(void* ptr)
  {
    if (_type > BaseTypeMaxAtomicType)
      callByBaseType<destruction_dispatcher>(_type, ptr, _count);
    
    free(ptr);
  }
  
  off_t _count;
  BaseType _type;
};

Mat::Mat(BaseType type, Idx size)
{
  create(type, size);
}

void Mat::create(BaseType type, Idx size)
{
  off_t count = size.total();
  
  static_cast<Idx&>(*this) = size;
  
  _data = std::shared_ptr<void>(BaseType_new(type, count), BaseType_deleter(count, type));
}

void Mat::release()
{
  _data.reset();
}
  
} //namespace clif
