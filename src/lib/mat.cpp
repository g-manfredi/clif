#include "mat.hpp"

#ifdef CLIF_BUILD_QT
  #include <QFile>
#endif
  
#include "enumtypes.hpp"

//FIXME move functions in extra header...
#include "hdf5.hpp"

namespace clif {
  
typedef unsigned int uint;
 
Idx::Idx() : std::vector<int>() {};

Idx::Idx(std::initializer_list<int> l) : std::vector<int>(l) {};

Idx::Idx(int size) : std::vector<int>(size) { printf("vec constr %d\n", size); };


Idx::Idx(const H5::DataSpace &space)
{
  hsize_t *dims = new hsize_t[space.getSimpleExtentNdims()];
  
  space.getSimpleExtentDims(dims);
  
  
  *this = Idx::invert(space.getSimpleExtentNdims(), dims);
  
  delete[] dims;
};


off_t Idx::total() const
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
    for (int i=0;i<count;i++)
      ((T*)ptr)[i].~T();
  }
};

template<typename T> class creation_dispatcher {
public:
  void operator()(void *ptr, off_t count)
  {
    for (int i=0;i<count;i++)
      new(((T*)ptr)+i) T;
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

#ifdef CLIF_BUILD_QT
struct QFile_deleter
{
  QFile_deleter(QFile *f) : _f(f) {};
  
  void operator()(void* ptr)
  {
    //FIXME call destruct dispatcher
    //FIXME delete QFile and mapping
    printf("FIXME qfile delete!\n");
    //delete _f;
  }
  
  QFile *_f;
};
#endif

struct cvMat_deleter
{
  cvMat_deleter(cv::Mat &m) : _m(m) {};
  
  void operator()(void* ptr)
  {
    //this is not actually necessary, _m should be freed when deleter is destroyed
    //_m.release();
  }
  
  cv::Mat _m;
};

BaseType const & Mat::type() const
{
  return _type;
}

void* Mat::data()
{
  return _data.get();
}

Mat::Mat(BaseType type, Idx size)
{
  create(type, size);
}

Mat::Mat(cv::Mat &m)
{
  _type = CvDepth2BaseType(m.depth());
  
  resize(m.dims);
  _step.resize(m.dims);
  
  for(int i=0;i<m.dims;i++) {
    operator[](i) = m.size[m.dims-i-1];

    if (i < m.dims-i)
      _step[i] = m.step[m.dims-i-2]/baseType_size(_type);
  }
  
  _data = std::shared_ptr<void>(m.data, cvMat_deleter(m));
}

Mat::Mat(cv::Mat *m)
: Mat(*m)
{
}

void Mat::create(BaseType type, Idx size)
{
  if (type == _type && size.total() == total())
  {
    static_cast<Idx&>(*this) = size;
    return;
  }
  
  off_t count = size.total();
  
  static_cast<Idx&>(*this) = size;
  _step = size;
  _type = type;
  
  _data = std::shared_ptr<void>(BaseType_new(type, count), BaseType_deleter(count, type));
}


int Mat::write(const char *path)
{
  FILE *f = fopen(path, "wb");
  
  if (!f)
    return -1;
  
  int len = fwrite(data(), 1, baseType_size(_type)*total(), f);
  fclose(f);
  
  if (len != baseType_size(_type)*total())
    return -1;
  return 0;
}

int Mat::read(const char *path)
{
//#ifndef CLIF_BUILD_QT 
  FILE *f = fopen(path, "rb");
  
  create(type(), *(Idx*)this);
  
  if (!f)
    return -1;
  
  int len = fread(data(), 1, baseType_size(_type)*total(), f);
  fclose(f);
  
  if (len != baseType_size(_type)*total())
    return -1;
  
  return 0;
/*#else
  QFile *f = new QFile(path);
  
  if (!f->open(QIODevice::ReadOnly)) {
    delete f;
    return -1;
  }
  
  void *cdata = (void*)f->map(0, baseType_size(_type)*total());
  if (!cdata) {
    delete f;
    return -1;
  }
  
  _data = std::shared_ptr<void>(cdata, QFile_deleter(f));
  
  return 0;
#endif*/
}

void Mat::release()
{
  _data.reset();
}

template<typename T> class _vdata_write_dispatcher{
public:
  void operator()(hvl_t *v, Mat *m)
  {
    abort();
  }
};

template<typename T> class _vdata_read_dispatcher{
public:
  void operator()(hvl_t *v, Mat *m)
  {
    abort();
  }
};


template<typename T> class _vdata_write_dispatcher<std::vector<T>>{
public:
  void operator()(hvl_t *v, Mat *m)
  {
    for(int i=0;i<m->total();i++) {
      v[i].len = m->operator()<std::vector<T>>(i).size();
      if (v[i].len)
        v[i].p = &m->operator()<std::vector<T>>(i)[0];
      else
        v[i].p = NULL;
    }
  }
};

template<typename T> class _vdata_read_dispatcher<std::vector<T>>{
public:
  void operator()(hvl_t *v, Mat *m)
  {
    for(int i=0;i<m->total();i++) {
      std::vector<T> &vec = m->operator()<std::vector<T>>(i);
      vec.resize(v[i].len);
      //TODO will not work for actual class types!
      if (v[i].len)
        memcpy(&vec[0], v[i].p, sizeof(T)*v[i].len);
    }
  }
};


hvl_t *Mat_H5vlenbuf_alloc(Mat &m)
{
  hvl_t *vdata;
  
  if (int(m.type() & BaseType::VECTOR) == 0)
    return NULL;
  
  vdata = (hvl_t*)malloc(sizeof(hvl_t)*m.total());
    
  return vdata;
}

void Mat_H5vlenbuf_read(Mat &m, hvl_t *v)
{
  if (int(m.type() & BaseType::VECTOR) == 0)
    abort();

  callByBaseType<_vdata_read_dispatcher>(m.type(), v, &m);
}

hvl_t *Mat_H5vlenbuf(Mat &m)
{
  hvl_t *vdata = Mat_H5vlenbuf_alloc(m);
  
  if (vdata)
    callByBaseType<_vdata_write_dispatcher>(m.type(), vdata, &m);
    
  return vdata;
}

void Mat_H5AttrWrite(Mat &m, H5::H5File &f, const boost::filesystem::path &path)
{
  boost::filesystem::path parent = path.parent_path();
  boost::filesystem::path name = path.filename();
  
  hsize_t *dim = new hsize_t[m.size()];
  for(uint i=0;i<m.size();i++)
    dim[i] = m[i];
  
  H5::DataSpace space(m.size(), dim);
  H5::Attribute attr;
  H5::Group g;

  delete[] dim;
    
  if (!h5_obj_exists(f, parent))
    h5_create_path_groups(f, parent);
  
  g = f.openGroup(parent.generic_string().c_str());
  
  uint min, max;
  
  H5Pget_attr_phase_change(H5Gget_create_plist(g.getId()), &max, &min);
  
  if (min || max)
    printf("WARNING: could not set dense storage on group, may not be able to write large attributes\n");
  
  if (H5Aexists(g.getId(), name.generic_string().c_str()))
    g.removeAttr(name.generic_string().c_str());
    
  attr = g.createAttribute(name.generic_string().c_str(), toH5DataType(m.type()), space);
  
  hvl_t *vdata = Mat_H5vlenbuf(m);
  if (vdata) {
    attr.write(toH5NativeDataType(m.type()), vdata);
    free(vdata);
  }
  else
    attr.write(toH5NativeDataType(m.type()), m.data());
}

void Mat_H5AttrRead(Mat &m, H5::Attribute &a)
{
  Idx extent;
  BaseType type = toBaseType(H5Aget_type(a.getId()));
  
  H5::DataSpace space = a.getSpace();
  int dimcount = space.getSimpleExtentNdims();

  hsize_t *dims = new hsize_t[dimcount];
  hsize_t *maxdims = new hsize_t[dimcount];
  extent.resize(dimcount);
  
  space.getSimpleExtentDims(dims, maxdims);
  for(int i=0;i<dimcount;i++)
    extent[i] = dims[i];
  
  m.create(type, extent);
    
  hvl_t *v = Mat_H5vlenbuf_alloc(m);
  if (!v)
    a.read(toH5NativeDataType(type), m.data());
  else {
    //FIXME
    H5::DataType native = toH5NativeDataType(type);
    a.read(native, v);
    Mat_H5vlenbuf_read(m, v);
    H5Dvlen_reclaim(native.getId(), space.getId(), H5P_DEFAULT, v);
    free(v);
  }

  delete[] dims;
  delete[] maxdims;
}

void h5_dataset_read(H5::H5File f, const cpath &path, Mat &m)
{
  H5::DataSet data = f.openDataSet(path.string().c_str());
  read(data, m);
}

void read(H5::DataSet &data, Mat &m)
{
  H5::DataSpace space = data.getSpace();
  BaseType type = toBaseType(H5Dget_type(data.getId()));
  
  m.create(type, Idx(space));
  
  hvl_t *v = Mat_H5vlenbuf_alloc(m);
  if (!v)
    data.read(m.data(), toH5NativeDataType(type));
  else {
    //FIXME
    H5::DataType native = toH5NativeDataType(type);
    data.read(v, native);
    Mat_H5vlenbuf_read(m, v);
    H5Dvlen_reclaim(native.getId(), space.getId(), H5P_DEFAULT, v);
    free(v);
  }
}

//FIXME dim_order must be sorted atm!
//FIXME ignore vlen reads!
void read_full_subdims(H5::DataSet &data, Mat &m, std::vector<int> dim_order, Idx offset)
{
  int dims = offset.size();
  H5::DataSpace space = data.getSpace();
  Idx data_size(space);
  Idx size(dim_order.size());
  hsize_t *count_h = new hsize_t[dims];
  hsize_t *offset_h = new hsize_t[dims];
  hsize_t *size_h = new hsize_t[dim_order.size()];
  
  assert(size.size() == dim_order.size());
  
  //FIXME assert offset == space dims #
  
  BaseType type = toBaseType(H5Dget_type(data.getId()));
  
  for(int i=0;i<dims;i++) {
    offset_h[dims-i-1] = offset[i];
    count_h[dims-i-1] = 1;
  }
  
  for(int i=0;i<dim_order.size();i++) {
    if (dim_order[i] > dims)
      abort();
    
    size[i] = data_size[dim_order[i]] - offset[dim_order[i]];
    size_h[dim_order.size()-i-1] = size[i];
    
    count_h[dims-dim_order[i]-1] = size[i];
  }
  
  m.create(type, size);
  
  space.selectHyperslab(H5S_SELECT_SET, count_h, offset_h);
  H5::DataSpace memspace(size.size(), size_h);
  
  data.read(m.data(), toH5DataType(type), memspace, space);
  
  
  delete[] offset_h;
  delete[] count_h;
}
/*
void write_full_subdims(H5::DataSet &data, Mat &m, std::vector<int> dim_order, Idx offset)
{
  int dims = offset.size();
  H5::DataSpace space = data.getSpace();
  Idx data_size(space);
  Idx extend_size(space);
  Idx size(dim_order.size());
  hsize_t *count_h = new hsize_t[dims];
  hsize_t *offset_h = new hsize_t[dims];
  hsize_t *size_h = new hsize_t[dim_order.size()];
  
  assert(size.size() == dim_order.size());
  
  //FIXME assert offset == space dims #
  
  BaseType type = toBaseType(H5Dget_type(data.getId()));
  
  for(int i=0;i<dims;i++) {
    offset_h[dims-i-1] = offset[i];
    count_h[dims-i-1] = 1;
    extend_size[i] = offset[i]+1;
  }
  
  bool extend_required = false;
  for(int i=0;i<dim_order.size();i++) {
    if (dim_order[i] > dims)
      abort();
    if (i>0 && dim_order[i-1] >= dim_order[i])
      abort();
    
    extend_size[dim_order[i]] = m[i] + offset[dim_order[i]];
    size_h[dim_order.size()-i-1] = m[i];
    count_h[dims-dim_order[i]-1] = m[i];
    if extend_size[i] >= 
  }
  
  for(int i=0;i<dims;i++) {
    
  }
  
  space.selectHyperslab(H5S_SELECT_SET, count_h, offset_h);
  H5::DataSpace memspace(size.size(), size_h);
  
  data.read(m.data(), toH5DataType(type), memspace, space);
  
  
  delete[] offset_h;
  delete[] count_h;
}*/

//FIXME avoid new/delete!
//FIXME implement re-conversion if clif::Mat was created from cv::Mat
cv::Mat cvMat(Mat &m)
{
  cv::Mat tmp;
  int *idx = new int[m.size()];
  size_t *step = new size_t[m.size()-1];
  
  for(int i=0;i<m.size();i++) {
    idx[i] = m[m.size()-i-1];
    if (i)
      step[m.size()-i-1] = m.step()[i-1]*baseType_size(m.type());
  }
  
  tmp = cv::Mat(m.size(), idx, BaseType2CvDepth(m.type()), m.data(), step);
  
  delete[] idx;
  delete[] step;
  
  return tmp;
}

const std::vector<int> & Mat::step() const
{
  return _step;
}
  
} //namespace clif
