#ifndef _CLIF_MAT_H
#define _CLIF_MAT_H

#include <vigra/multi_array.hxx>

#include <memory>

#include "enumtypes.hpp"
#include "core.hpp"

//FIXME use forward declaration - better put in extra header...
#include <hdf5.h>
#include <boost/filesystem.hpp>
#include <H5File.h>

namespace clif {
  
class Idx : public std::vector<int>
{
public:
  Idx();
  Idx(int size);
  Idx(std::initializer_list<int> l);
  Idx(const H5::DataSpace &space);
  
  off_t total() const;
  
  static Idx zeroButOne(int size, int pos, int idx);
  template<typename T> static Idx invert(int size, const T * const dims);
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


template<typename T> Idx Idx::invert(int size, const T * const dims)
{
  Idx idx = Idx(size);
  
  for(int i=0;i<size;i++)
    idx[i] = dims[size-i-1];
  
  return idx;
}
  
class Mat : public Idx {
public:
  Mat() {};
  Mat(BaseType type, Idx size);
  //warning - document sharing semantics between opencv mat and us (extra user)
  Mat(cv::Mat &m);
  Mat(cv::Mat *m);
  
  void create(BaseType type, Idx size);
  void release();
  
  int read(const char *path);
  int write(const char *path);
  
  void* data();
  
  const std::vector<int> & step() const;
  
  BaseType const & type() const;
  
  template<typename T, typename ... Idxs>
    T& operator()(Idxs ... idxs);
  
  template<template<typename> class F, typename ... ArgTypes>
    void call(ArgTypes ... args);
  template<template<typename> class F, template<typename> class CHECK, typename ... ArgTypes>
    void callIf(ArgTypes ... args);

  
protected:
  BaseType _type = BaseType::INVALID;
  std::shared_ptr<void> _data;
  Idx _step; //not yet used!
  
private:
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
    return ((T*)_data.get())[calc_offset<T>(_step, 0, idxs...)];
  }
};

hvl_t *Mat_H5vlenbuf(Mat &m);
void   Mat_H5AttrWrite(Mat &m, H5::H5File &f, const boost::filesystem::path &path);
void   Mat_H5AttrRead(Mat &m, H5::Attribute &a);

void read(H5::DataSet &data, Mat &m);
void write(Mat &m, H5::DataSet &data);

//reads into m number dims_order.size() dimensional subset
//dim order specifies mapping from m dims to dataset dims
void read_full_subdims(H5::DataSet &data, Mat &m, std::vector<int> dim_order, Idx offset);

cv::Mat cvMat(Mat &m);

//FIXME implement stride
template<int DIM, typename T> vigra::MultiArrayView<DIM,T> vigraMAV(Mat &m)
{
  vigra::TinyVector<vigra::MultiArrayIndex, DIM> shape;
  
  if (DIM != m.size())
    abort();
  
  if (toBaseType<T>() != m.type())
    abort();
  
  for(int i=0;i<DIM;i++)
    shape[i] = m[i];
  
  return vigra::MultiArrayView<DIM,T>(shape, (T*)m.data());
}

template<int DIM, typename T> vigra::MultiArrayView<DIM,T> vigraMAV(Mat *m)
{
  return vigraMAV<DIM,T>(*m);
}

template<typename T, typename ... Idxs> T& Mat::operator()(Idxs ... idxs)
{
  return ((T*)_data.get())[calc_offset<T>(*(Idx*)this, 0, idxs...)];
} 

template<template<typename> class F, typename ... ArgTypes> void Mat::call(ArgTypes ... args)
{
  callByBaseType<F>(_type, args...);
}
template<template<typename> class F, template<typename> class CHECK, typename ... ArgTypes> void Mat::callIf(ArgTypes ... args)
{
  clif::callIf<F,CHECK>(_type, args...);
  
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

void h5_dataset_read(H5::H5File f, const cpath &path, Mat &m);

} //namespace clif

#endif