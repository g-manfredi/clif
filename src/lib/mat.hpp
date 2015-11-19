#ifndef _CLIF_MAT_H
#define _CLIF_MAT_H

#include <memory>

#include "enumtypes.hpp"

//FIXME use forward declaration!
#include <hdf5.h>

namespace clif {
  
class Idx : public std::vector<int>
{
public:
  Idx();
  Idx(int size);
  template<typename ... TS> Idx(TS ... idxs);
  
  off_t total();
  
  static Idx zeroButOne(int size, int pos, int idx);
};
  
namespace {
  template<typename T> off_t calc_offset(Idx &extent, int pos)
  {
    return 0;
  }
  
  template<typename T, typename ... Idxs> off_t calc_offset(Idx &extent, int pos, int idx, Idxs ... rest)
  {
    return idx + extent[pos]*calc_offset<T>(extent, pos+1, rest...);
  }
}
  
class Mat : public Idx {
public:
  Mat() {};
  Mat(BaseType type, Idx size);
  
  void create(BaseType type, Idx size);
  void release();
  
  void* data();
  
  BaseType type();
  
  //FIXME/DOCUMENT: this should only be used after make-unique etc...
  template<typename T, typename ... Idxs> T& operator()(Idxs ... idxs)
  {
    return ((T*)_data.get())[calc_offset<T>(*(Idx*)this, 0, idxs...)];
  }
  
protected:
  BaseType _type;
  std::shared_ptr<void> _data;
  
private:
  Idx _step; //not yet used!
};

template<typename T> class Mat_ : public Mat {
public:  
  Mat_();
  Mat_(Idx size);
  Mat_(BaseType type, Idx size);
  
  void create(Idx size);
  void create(BaseType type, Idx size);
  
  //FIXME/DOCUMENT: this should only be used after make-unique etc...
  template<typename ... Idxs> T& operator()(Idxs ... idxs)
  {   
    return ((T*)_data.get())[calc_offset<T>(*(Idx*)this, 0, idxs...)];
  }
};

hvl_t *Mat_H5vlenbuf(Mat &m);

namespace {
  void _set_array_from(int *ar) {};
  
  template<typename T, typename ... TS> void _set_array_from(int *ar, T idx, TS ... rest)
  {
    *ar = idx;
    _set_array_from(ar+1,rest...);
  }
}

template <typename ... TS> Idx::Idx(TS ... idxs)
{
  const int n = sizeof...(TS);
  
  resize(n);
  
  _set_array_from(&(*this)[0], idxs...);
}

template <typename T> Mat_<T>::Mat_()
{
  _type = toBaseType<T>();
}

template <typename T> Mat_<T>::Mat_(Idx size)
{
  _type = toBaseType<T>();
  create(size);
}

template <typename T> Mat_<T>::Mat_(BaseType type, Idx size)
{
  _type = toBaseType<T>();
  create(type, size);
}

template <typename T> void Mat_<T>::create(Idx size)
{
  Mat::create(_type, size);
}

template <typename T> void Mat_<T>::create(BaseType type, Idx size)
{
  if (type != _type)
    abort();
  
  Mat::create(_type, size);
}


} //namespace clif

#endif