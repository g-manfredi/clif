#include "mat.hpp"

#ifdef CLIF_BUILD_QT
  #include <QFile>
#endif
  
#include "enumtypes.hpp"

//FIXME move functions in extra header...
#include "hdf5.hpp"

//FIXME opencv step calculation is definately not working!

namespace clif {
  
typedef unsigned int uint;
 
Idx::Idx() : std::vector<int>() {};

Idx::Idx(std::initializer_list<int> l) : std::vector<int>(l) {};

Idx::Idx(std::initializer_list<IdxRange> l)
{
  int sum = 0;
  for(auto it : l)
    sum += it.end-it.start+1;
  
  resize(sum);
  
  int idx = 0;
  for(auto it : l)
    if (!it.src) {
      operator[](idx) = it.val;
      if (it.name.size())
        name(idx, it.name);
      idx++;
    }
    else {
      for(int i=it.start;i<=it.end;i++) {
        operator[](idx) = it.src->operator[](i);
        if (it.src->name(i).size())
          name(idx, it.src->name(i));
        idx++;
      }
    }
}

int DimSpec::get(const Idx *ref) const
{
  if (_dim == INT_MIN)
    return ref->dim(_name)+_offset;
  else
    return _dim+_offset;
}

DimSpec DimSpec::operator+(const int& rhs) const
{
  DimSpec tmp(*this);
  tmp._offset += rhs;
  return tmp;
}

DimSpec DimSpec::operator-(const int& rhs) const
{
  DimSpec tmp(*this);
  tmp._offset -= rhs;
  return tmp;
}

int Idx::dim(const std::string &name) const
{
  if (_name_map.count(name))
    return _name_map.find(name)->second;
  else
    return -1;
}

static Idx _empty();

Idx::Idx(int size) : std::vector<int>(size) {};

Idx& Idx::operator+=(const Idx& rhs)
{
  for(int i=0;i<size();i++)
    operator[](i) += rhs[i];
  return *this;
}

void Idx::step(int dim, const Idx& max)
{
  operator[](dim)++;
  
  while(max[dim] > 0 && operator[](dim) >= max[dim] && dim < max.size()-1) {
    operator[](dim) = 0;
    dim++;
    operator[](dim)++;
  }
}

void Idx::step(const std::string& name, const Idx& max)
{
  int idim = dim(name);
  
  operator[](idim)++;
  
  while(max[idim] > 0 && operator[](idim) >= max[idim] && idim < max.size()-1) {
    operator[](idim) = 0;
    idim++;
    operator[](idim)++;
  }
}

void Idx::name(int i, const std::string& name)
{
  if (_names.size() <= i)
    _names.resize(i+1);
  _names[i] = name;
  _name_map[name] = i;
}

static const std::string _empty_str = "";

const std::string& Idx::name(int i) const
{
  if (_names.size() <= i)
    return _empty_str;
  
  return _names[i];
}

//FIXME inefficient
void Idx::names(std::initializer_list<std::string> l)
{
  _names.resize(l.size());
  for(int i=0;i<l.size();i++)
    name(i, l.begin()[i]);
}

void Idx::names(const std::vector<std::string> &l)
{
  _names.resize(l.size());
  for(int i=0;i<l.size();i++)
    name(i, l[i]);
}

const std::vector<std::string>& Idx::names() const
{
  return _names;
}

IdxRange::IdxRange(const Idx *src_, int start_, int end_)
{
  start = start_;
  end = end_;
  if (end == INT_MIN)
    end = start;
  
  if (src_->size()) {
    src = src_;
    if (end < 0)
      end = src->size()+end;
  }
  val = 0;
}

IdxRange::IdxRange(int val_, const std::string& name_)
{
  start = 0;
  end = 0;
  name = name_;
  val = val_;
}

IdxRange IR(int i, const std::string& name)
{
  return IdxRange(i, name);
}
     
IdxRange Idx::r(const DimSpec &start, const DimSpec &end) const
{
  int ds, de;
  
  ds = start.get(this);
  
  if (!end.valid())
    de = INT_MIN;
  else
    de = end.get(this);
  
  return IdxRange(this, ds, de);
}

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


std::ostream& operator<<(std::ostream& out, const Idx& idx)
{
  for(int i=0;i<idx.size();i++)
    out << idx.name(i) << ":" << idx[i] << " ";
  return out;
}

void *BaseType_new(BaseType type, off_t count)
{
  void *ptr = malloc(count*baseType_size(type));
  assert(ptr);
  
  if (type > BaseTypeMaxAtomicType)
    callByBaseType<basetype_create_class_instances>(type, ptr, count);
  
  return ptr;
}

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

void* Mat::data() const
{
  return _data;
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

    if (i)
      _step[i] = m.step[m.dims-i-1];
    else
      _step[0] = baseType_size(_type);
  }
  
  _data = m.data;
  _mem = std::shared_ptr<void>(_data, cvMat_deleter(m));
}

Mat::Mat(cv::Mat *m)
: Mat(*m)
{
}

//WARNING keep Mat_<T>::create in sync!
void Mat::create(BaseType type, Idx newsize)
{
  //FIXME check labels!
  if (type == _type && newsize.total() == total())
  {
    static_cast<Idx&>(*this) = newsize;
    return;
  }
  
  _type = type;
  static_cast<Idx&>(*this) = newsize;
  _step.resize(size());
  
  _step[0] = baseType_size(_type);
  for(int i=1;i<size();i++)
    _step[i] = _step[i-1]*newsize[i-1];
  _type = type;
  
  _data = BaseType_new(type, total());
  _mem = std::shared_ptr<void>(_data, BaseType_deleter(total(), type));
}

//FIXME check for class type!
//FIXME not working :-(
void Mat::copyTo(Mat &m)
{
  if (size() != m.size() || total() != m.total())
    abort();
  
  int direct_dim;
  
  off_t full_size = baseType_size(_type);
  
  for(direct_dim=0;direct_dim<size();direct_dim++) {
    if (m._step[direct_dim] != _step[direct_dim] || full_size*operator[](direct_dim) != _step[direct_dim])
      break;
    full_size = _step[direct_dim]*operator[](direct_dim);
  }
  
  
  if (direct_dim == size()) {
    memcpy(m._data, _data, full_size);
    return;
  }
    
  int curr_dim;
  Idx pos(size());
  
  while (true) {
    memcpy(m.ptr(pos), ptr(pos), full_size);
    //copy or whatever
    curr_dim = direct_dim;
    pos[curr_dim]++;
    while(pos[curr_dim] == operator[](curr_dim)) {
      pos[curr_dim] = 0;
      curr_dim++;
      
      if (curr_dim == size())
        return;
      
      pos[curr_dim]++;
    }
  }
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
#ifndef CLIF_BUILD_QT 
  FILE *f = fopen(path, "rb");
  
  create(type(), *(Idx*)this);
  
  if (!f)
    return -1;
  
  int len = fread(data(), 1, baseType_size(_type)*total(), f);
  fclose(f);
  
  if (len != baseType_size(_type)*total())
    return -1;
  
  return 0;
#else
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
  
  _data = cdata;
  _mem = std::shared_ptr<void>(cdata, QFile_deleter(f));
  
  return 0;
#endif
}

void Mat::release()
{
  _mem.reset();
  _data = NULL;
  resize(0);
  _step.resize(0);
  _type = BaseType::INVALID;
}

void Mat::reshape(const Idx &newsize)
{
  //FIXME add more checks (for stride/step)
  
  if (total() != newsize.total())
    abort();
  
  static_cast<Idx&>(*this) = newsize;
  
  abort();
  //FIXME calc step!
  //_step = newsize;
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
  H5::DataSet data = f.openDataSet(path.generic_string().c_str());
  read(data, m);
}

void read(H5::DataSet &data, Mat &m)
{
#pragma omp critical(hdf5)
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
}

//FIXME dim_order must be sorted atm!
//FIXME ignore vlen reads!
void read_full_subdims(H5::DataSet &data, Mat &m, const Idx& dim_order, const Idx& offset)
{
  int dims = offset.size();
  hsize_t *count_h = new hsize_t[dims];
  hsize_t *offset_h = new hsize_t[dims];
  hsize_t *size_h = new hsize_t[dim_order.size()];
#pragma omp critical(hdf5)
  {
  H5::DataSpace space = data.getSpace();
  Idx data_size(space);
  Idx size(dim_order.size());
  
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
  
  data.read(m.data(), toH5NativeDataType(type), memspace, space);
  }

  delete[] offset_h;
  delete[] count_h;
}

void write_full_subdims(H5::DataSet &data, const Mat &m, const Idx& offset, const Idx& dim_order)
{
  int dims = offset.size();
  
  Idx order = dim_order;
  if (!order.size()) {
    order.resize(m.size());
    for(int i=0;i<order.size();i++)
      order[i] = i;
  }
  
  hsize_t *count_h = new hsize_t[dims];
  hsize_t *offset_h = new hsize_t[dims];
  hsize_t *size_h = new hsize_t[order.size()];
  hsize_t *extend = new hsize_t[dims];
  
#pragma omp critical(hdf5)
  {
  H5::DataSpace space = data.getSpace();
  Idx data_size(space);
  
  //check wether hdf5 datasets needs to be enlarged
  bool extend_dataset = false;
  for(int i=0,h_i=dims-1;i<dims;i++,h_i--) {
    extend[h_i] = offset[i] + 1;
    if (extend[h_i] <= data_size[i])
      extend[h_i] = data_size[i];
    else
      extend_dataset = true;
  }
  for(int i=0;i<order.size();i++) {
    int idx = order[i];
    int h_idx = dims-idx-1;
    
    extend[h_idx] = offset[idx] + m[i];
    if (extend[h_idx] <= data_size[idx])
      extend[h_idx] = data_size[idx];
    else
      extend_dataset = true;
  }
  
  if (extend_dataset) {
    data.extend(extend);
    space = data.getSpace();
  }
  
  BaseType type = toBaseType(H5Dget_type(data.getId()));
  
  for(int i=0,h_i=dims-1;i<dims;i++,h_i--) {
    offset_h[h_i] = offset[i];
    count_h[h_i] = 1;
  }
  
  for(int i=0;i<order.size();i++) {
    int h_i = order.size()-i-1;
    if (order[i] > dims)
      abort();
    
    size_h[h_i] = m[i];
    count_h[dims-order[i]-1] = m[i];
  }
  
  space.selectHyperslab(H5S_SELECT_SET, count_h, offset_h);
  H5::DataSpace memspace(order.size(), size_h);
  
  data.write(m.data(), toH5NativeDataType(type), memspace, space);
  }

  delete[] offset_h;
  delete[] count_h;
  delete[] size_h;
  delete[] extend;
}

//FIXME avoid new/delete!
//FIXME implement re-conversion if clif::Mat was created from cv::Mat
cv::Mat cvMat(const Mat &m)
{  
  cv::Mat tmp;
  int *idx = new int[m.size()];
  size_t *step = new size_t[m.size()-1];
  
  for(int i=0;i<m.size();i++) {
    idx[i] = m[m.size()-i-1];
    if (i)
      step[m.size()-i-1] = m.step()[i];
  }
  
  int type = CV_32S;
  if (m.type() != BaseType::UINT32)
    type = BaseType2CvDepth(m.type());
  
  tmp = cv::Mat(m.size(), idx, type, m.data(), step);
  
  if (m.type() == BaseType::UINT32)
  {
    cv::Mat tmp_float;
    tmp_float.create(tmp.dims, tmp.size, CV_32F);
    //make continuous
    tmp = tmp.clone();
    printf("WARNING cvMat() encountered a 32 bit uint matrix - converting to float - this will be very slow!\n");
    //clone makes this mat continuous, so this works
    float *float_ptr = tmp_float.ptr<float>(0);
    uint32_t *int_ptr = tmp.ptr<uint32_t>(0);
    for(int i=0;i<tmp.total();i++)
      float_ptr[i] = int_ptr[i];
    tmp = tmp_float;
  }
  
  delete[] idx;
  delete[] step;
  
  return tmp;
}

static cv::Mat mat_2d_from_3d(const cv::Mat *m, int ch)
{
  //step is already in bytes!
  return cv::Mat(m->size[1], m->size[2], m->depth(), m->data + ch*m->step[0]);
}

cv::Mat cvImg(const Mat &m)
{
  cv::Mat in = cvMat(m);
  
  assert(m.size() == 3);
  
  std::vector<cv::Mat> channels(in.size[0]);

  for(int i=0;i<in.size[0];i++)
    //FIXME need clone else merge will memcpy overlaying memory areas. why?
    channels[i] = mat_2d_from_3d(&in, i);

  cv::Mat out;
  out.create(in.size[1], in.size[2], in.type());
  cv::merge(channels, out);
  
  return out;
}


Mat Mat::bind(int dim, int pos) const
{
  Mat b_mat = *this;
  
  b_mat.resize(size()-1);
  b_mat._step.resize(b_mat.size());
  
  for(int i=0;i<dim;i++) {
    b_mat[i] = operator[](i);
    b_mat._step[i] = _step[i];
  }
  //if dim is not last idx!
  /*if (dim < b_mat.size()) {
    b_mat[dim] = operator[](dim+1);
    b_mat._step[dim] = _step[dim]*_step[dim+1];
  }*/
  for(int i=dim;i<b_mat.size();i++) {
    b_mat[i] = operator[](i+1);
    b_mat._step[i] = _step[i+1];
  }
  
  b_mat._data = (char*)_data + _step[dim]*pos;
  
  return b_mat;
}

Mat Mat3d(const cv::Mat &img)
{
  Mat m(CvDepth2BaseType(img.depth()), Idx{img.size().width,img.size().height, img.channels()});
  
  if (img.dims != 2)
    abort();
  
  std::vector<cv::Mat> channels;
  cv::split(img, channels);

  for(int i=0;i<channels.size();i++)
    channels[i].copyTo(cvMat(m.bind(2, i)));
  
  return m;
}

const std::vector<off_t> & Mat::step() const
{
  return _step;
}
  
} //namespace clif
