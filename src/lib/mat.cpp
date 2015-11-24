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

Idx::Idx(int size) : std::vector<int>(size) {};

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
  
  for(int i=0;i<m.dims;i++)
    operator[](i) = m.size[m.dims-i-1];
  
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
  _type = type;
  
  _data = std::shared_ptr<void>(BaseType_new(type, count), BaseType_deleter(count, type));
}


int Mat::write(const char *path)
{
  FILE *f = fopen(path, "w");
  
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
  FILE *f = fopen(path, "r");
  
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
      v[i].p = &m->operator()<std::vector<T>>(i)[0];
    }
  }
};

template<typename T> class _vdata_read_dispatcher<std::vector<T>>{
public:
  void operator()(hvl_t *v, Mat *m)
  {
    for(int i=0;i<m->total();i++) {
      m->operator()<std::vector<T>>(i).resize(v[i].len);
      memcpy(&m->operator()<std::vector<T>>(i)[0], v[i].p, sizeof(T)*v[i].len);
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
  
//FIXME implement non-dense matrix!
cv::Mat cvMat(Mat &m)
{
  int *idx = new int[m.size()];
  
  for(int i=0;i<m.size();i++)
    idx[i] = m[m.size()-i-1];
  
  return cv::Mat(m.size(), idx, BaseType2CvDepth(m.type()), m.data());
}
  
} //namespace clif
