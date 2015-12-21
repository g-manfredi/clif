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
#include <unordered_map>

namespace clif {
  
class Idx;
  
class IdxRange {
public: 
  IdxRange(Idx *src_, int start_, int end_ = INT_MIN);
  IdxRange(int val_, const std::string& name = std::string());
  
  int start, end;
  Idx *src = NULL;
  std::string name; //for direct construction
  int val; //for direct construction
};


class DimSpec
{
  int _dim = INT_MIN;
  std::string _name;
  int _offset = 0;
public:
  
  DimSpec() {};
  DimSpec(int dim) : _dim(dim) {};
  DimSpec(const std::string& name) : _name(name) {};
  DimSpec(const char* name) : _name(std::string(name)) {};
  
  bool valid() const
  {
    if (_dim != INT_MIN || _name.size())
      return true;
    else
      return false;
  }
  
  int get(const Idx *ref) const;
  DimSpec operator+(const int& rhs) const;
  DimSpec operator-(const int& rhs) const;

};
  
class Idx : public std::vector<int>
{
public:  
  Idx();
  Idx(int size);
  Idx(std::initializer_list<int> l);
  Idx(std::initializer_list<IdxRange> l);
  Idx(const H5::DataSpace &space);
  
  //convert to/from std::vector<int>
  Idx(const std::vector<int> &v) : std::vector<int>(v) {};
  operator std::vector<int>() { return *static_cast<std::vector<int>*>(this); };
  const int& operator[](int idx) const { return std::vector<int>::operator[](idx); };
  int& operator[](int idx) { return std::vector<int>::operator[](idx); };
  int& operator[](const std::string &name) { return operator[](dim(name)); };
  
  //convert to IdxRange
  operator IdxRange() { return r(0, -1); };
  
  off_t total() const;
  
  Idx& operator+=(const Idx& rhs);
  void step(int dim, const Idx& max);
  void step(const std::string &name, const Idx& max);
  void names(std::initializer_list<std::string> l);
  void names(const std::vector<std::string> &l);
  const std::vector<std::string>& names() const;
  
  void name(int i, const std::string& name);
  const std::string& name(int i) const;
  int dim(const std::string &name) const;
  
  IdxRange r(const DimSpec &start, const DimSpec &end);
  /*IdxRange r(const std::string &str, const std::string &end = std::string());
  IdxRange r(int i, const std::string &end);
  IdxRange r(const std::string &str, int end);*/
  
  static Idx zeroButOne(int size, int pos, int idx);
  template<typename T> static Idx invert(int size, const T * const dims);
  
  friend std::ostream& operator<<(std::ostream& out, const Idx& idx);

private:
  std::unordered_map<std::string,int> _name_map;
  std::vector<std::string> _names;
};

IdxRange IR(int i, const std::string& name);

class Idx_Iter_Single_Dim : public std::iterator<std::input_iterator_tag, Idx>
{
  int _dim;
  Idx _pos;
public:
  Idx_Iter_Single_Dim(const Idx &pos, const DimSpec& dim) : _pos(pos), _dim(dim.get(&pos)) {};
  Idx_Iter_Single_Dim(const Idx_Iter_Single_Dim& other) : _pos(other._pos), _dim(other._dim) {}
  Idx_Iter_Single_Dim& operator++() {++_pos[_dim]; return *this;}
  Idx_Iter_Single_Dim operator++(int) { Idx_Iter_Single_Dim tmp(*this); operator++(); return tmp; }
  bool operator==(const Idx_Iter_Single_Dim& rhs) {return _pos[_dim] == rhs._pos[_dim];}
  bool operator!=(const Idx_Iter_Single_Dim& rhs) {return _pos[_dim] != rhs._pos[_dim];}
  Idx& operator*() {return _pos; }
};

class Idx_Iter_Multi_Dim : public std::iterator<std::input_iterator_tag, Idx>
{
  int _dim;
  Idx _pos;
  Idx _size;
public:
  Idx_Iter_Multi_Dim(const Idx &pos, const Idx &size, const DimSpec& dim) : _pos(pos), _size(size), _dim(dim.get(&size)) {};
  Idx_Iter_Multi_Dim(const Idx_Iter_Multi_Dim& other) : _pos(other._pos), _size(other._size), _dim(other._dim) {}
  Idx_Iter_Multi_Dim& operator++() {_pos.step(_dim, _size); return *this;}
  Idx_Iter_Multi_Dim operator++(int) { Idx_Iter_Multi_Dim tmp(*this); operator++(); return tmp; }
  bool operator!=(const Idx_Iter_Multi_Dim& rhs) { return _pos.back() < rhs._pos.back(); }
  Idx& operator*() {return _pos; }
};

class Idx_It_Dim
{
  Idx _start;
  Idx _size;
  int _dim;
public:
  Idx_It_Dim(const Idx &size, const DimSpec& dim)
  : _size(size), _dim(dim.get(&size))
  {
    printf("iter dim: %d\n", _dim);
    _start = _size;
    for(int i=0;i<_start.size();i++)
      _start[i] = 0;
  };
  
  Idx_It_Dim(const Idx &start, const Idx &size, const DimSpec& dim)
  : _size(size), _dim(dim.get(&size))
  {
    _start = start;
    _start[_dim] = 0;
  };
  
  Idx_Iter_Single_Dim begin() { return Idx_Iter_Single_Dim(_start, _dim); };
  Idx_Iter_Single_Dim end() { return Idx_Iter_Single_Dim(_size, _dim); };
};

class Idx_It_Dims
{
  Idx _start;
  Idx _size;
  int _mindim;
public:
  Idx_It_Dims(const Idx &size, const DimSpec& mindim, const DimSpec& maxdim)
  : _size(size), _mindim(mindim.get(&size))
  {
    _start = _size;
    for(int i=maxdim.get(&size)+1;i<_size.size();i++)
      _size[i] = 1;
    for(int i=0;i<_start.size();i++)
      _start[i] = 0;
  };
  
  Idx_Iter_Multi_Dim begin() { return Idx_Iter_Multi_Dim(_start, _size, _mindim); };
  Idx_Iter_Multi_Dim end() { return Idx_Iter_Multi_Dim(_size, _size, _mindim); };
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