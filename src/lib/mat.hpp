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
  
  //convert to/from std::vector<int>
  Idx(const std::vector<int> &v) : std::vector<int>(v) {};
  operator std::vector<int>() { return *static_cast<std::vector<int>*>(this); };
  
  off_t total() const;
  
  Idx& operator+=(const Idx& rhs);
  void step(int dim, const Idx& max);
  
  static Idx zeroButOne(int size, int pos, int idx);
  template<typename T> static Idx invert(int size, const T * const dims);
};

inline bool operator< (const Idx& lhs, const Idx& rhs)
{
  if (lhs.size() != rhs.size())
    return (lhs.size() < rhs.size());
  
  int curr = lhs.size()-1;
  while (lhs[curr] < rhs[curr]) {
    if (curr == 0)
      return true;
    curr--;
  }
  return false;
}
inline bool operator> (const Idx& lhs, const Idx& rhs){return rhs < lhs;}
inline bool operator<=(const Idx& lhs, const Idx& rhs){return !(lhs > rhs);}
inline bool operator>=(const Idx& lhs, const Idx& rhs){return !(lhs < rhs);}
  
namespace {
  template<typename T> off_t calc_offset(const std::vector<off_t> &step, int pos)
  {
    return 0;
  }
  
  template<typename T, typename ... Idxs> off_t calc_offset(const std::vector<off_t> &step, int pos, off_t idx, Idxs ... rest)
  {
    return idx*step[pos] + calc_offset<T>(step, pos+1, rest...);
  }
  
  
  //TODO dim switch for higher speed?
  off_t calc_offset(const std::vector<off_t> &step, const Idx &pos)
  {
    off_t off = 0;
    for(int i=0;i<pos.size();i++)
      off += step[i]*pos[i];
    return off;
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
  
  void reshape(const Idx &newsize);
  
  int read(const char *path);
  int write(const char *path);
  Mat bind(int dim, int pos) const;
  
  void copyTo(Mat &m);
  
  void* data() const;
  
  const std::vector<off_t> & step() const;
  
  BaseType const & type() const;
  
  template<typename T, typename ... Idxs>
    T& operator()(int first, Idxs ... idxs) const;
    
  //overloading causes compile problems with boost?!?
  template<typename T>
    T& operator()(Idx pos) const;
    
  void* ptr(Idx pos) const { return (void*)(((char*)_data)+calc_offset(_step, pos)); }
  
  template<template<typename> class F, typename ... ArgTypes>
    void call(ArgTypes ... args);
  template<template<typename> class F, template<typename> class CHECK, typename ... ArgTypes>
    void callIf(ArgTypes ... args);

  
protected:
  BaseType _type = BaseType::INVALID;
  void *_data = NULL;
  std::shared_ptr<void> _mem;
  std::vector<off_t> _step;
  
private:
};

template<typename T> class Mat_ : public Mat {
public:  
  Mat_();
  Mat_(Idx size);
  Mat_(const Mat &m);
  Mat_(BaseType type, Idx size);
  
  void create(Idx size);
  void create(BaseType type, Idx size);
  
  //FIXME/DOCUMENT: this should only be used after make-unique etc...
  template<typename ... Idxs> T& operator()(int first, Idxs ... idxs) const
  {
    return Mat::operator()<T>(first, idxs...);
  }
  T& operator()(Idx pos) const
  {
    return Mat::operator()<T>(pos);
  }
};

hvl_t *Mat_H5vlenbuf(Mat &m);
void   Mat_H5AttrWrite(Mat &m, H5::H5File &f, const boost::filesystem::path &path);
void   Mat_H5AttrRead(Mat &m, H5::Attribute &a);

Mat Mat3d(const cv::Mat &img);

void read(H5::DataSet &data, Mat &m);
void write(Mat &m, H5::DataSet &data);

//reads into m number dims_order.size() dimensional subset
//dim order specifies mapping from m dims to dataset dims
void read_full_subdims(H5::DataSet &data, Mat &m, const Idx& offset, const Idx& dim_order);
void write_full_subdims(H5::DataSet &data, const Mat &m, const Idx& offset, const Idx& dim_order = Idx());

cv::Mat cvMat(const Mat &m);

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

template<typename T, typename ... Idxs> T& Mat::operator()(int first, Idxs ... idxs) const
{
  return *(T*)(((char*)_data)+calc_offset<T>(_step, 0, first, idxs...));
} 

template<typename T> T& Mat::operator()(Idx pos) const
{
  return *(T*)(((char*)_data)+calc_offset(_step, pos));
}


template<typename T> Mat_<T>::Mat_(const Mat &m)
: Mat(m)
{
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