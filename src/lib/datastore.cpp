#include "datastore.hpp"

#include "clif.hpp"
#include "dataset.hpp"
#include "clif_cv.hpp"
#include "hdf5.hpp"

#define CACHE_CONT_MAT_CHANNEL 1
#define CACHE_CONT_MAT_IMG 2

#include "opencv2/imgproc/imgproc.hpp"
namespace clif {

typedef unsigned int uint;

//FIXME wether dataset already exists and overwrite?
//FIXME set remaining members!
void Datastore::create(std::string path, Dataset *dataset, const std::string format_group)
{
  assert(dataset);
  
  _type = BaseType::INVALID; 
  _org = DataOrg(-1);
  _order = DataOrder(-1); 
    
  _data = H5::DataSet();
  _path = path;
  _dataset = dataset;
  
  _format_group = format_group;
  
  _readonly = false;
  _memonly = false;
}


//create new datastore with specified size and type
/*void Datastore::create(std::string path, Dataset *dataset, BaseType type, const int dimcount, const int *size)
{
  assert(dataset);
  
  _type = BaseType(type); 
  _org = DataOrg(-1);
  _order = DataOrder(-1); 
    
  _data = H5::DataSet();
  _path = path;
  _dataset = dataset;
  
  //TODO create the hdf5 dataset
    
  hsize_t dims[dimcount];
  
  for(int i=0;i<dimcount;i++)
   dims[i] = size[i];

  path dataset_path = _dataset->path() / _path;
  
  if (h5_obj_exists(_dataset->f, dataset_path.c_str())) {
    printf("TODO overwrite!\n");
    abort();
    _data = _dataset->f.openDataSet(dataset_path.c_str());
    return;
  }
  
  h5_create_path_groups(_dataset->f, path(dataset_path.c_str()).parent_path());
  
  //chunking fixed as image now
  hsize_t chunk_dims[dimcount];
  chunk_dims[0] = dims[0];
  chunk_dims[1] = dims[1];
  
  for(int i=2;i<dimcount;i++)
    dims[i] = 1;
  
  H5::DSetCreatPropList prop;    
  prop.setChunk(dimcount, chunk_dims);
  
  H5::DataSpace space(dimcount, dims, dims);
  
  _data = _dataset->f.createDataSet(dataset_path.c_str(), 
                      toH5DataType(_type), space, prop);
}*/

void Datastore::create(std::string path, Dataset *dataset, cv::Mat &m, const std::string format_group)
{  
  create(path, dataset, format_group);
  
  _readonly = false;
  _memonly = true;
  
  _mat = m;
}

//read store into m 
void Datastore::read(cv::Mat &m)
{
  assert(_memonly);
  
  m = cvMat(_mat);
}

//read store into m 
void Datastore::read(clif::Mat &m)
{
  assert(_memonly);
  
  m = _mat;
}
    
//write m into store
void Datastore::write(cv::Mat &m)
{
  assert(!_readonly);
  
  if (!_memonly) {
    assert(_type == BaseType::INVALID);
    
    _memonly = true;
    
    _type = CvDepth2BaseType(m.depth());
    assert(m.channels() == 1);
  }
  
  assert(_type == CvDepth2BaseType(m.depth()));
  
  _mat = Mat(m);
}

void Datastore::write(clif::Mat &m)
{
  assert(!_readonly);
  
  if (!_memonly) {
    assert(_type == BaseType::INVALID);
    
    _memonly = true;
    
    _type = m.type();
  }
  
  assert(_type == m.type());
  
  _mat = m;
}

void Datastore::write(clif::Mat *m)
{
  write(*m);
}

void Datastore::reset()
{
  flush();
  
  this->~Datastore();
  new(this) Datastore();
}

//FIXME check for actual h5datasets in mem-only stores and force copy on delete!
void Datastore::link(const Datastore *other, Dataset *dataset)
{
  assert(dataset);
  
  reset();
  
  if (!other->valid())
    return;
  
  _readonly = true;
  
  _type = other->_type; 
  _org = other->_org;
  _order = other->_order; 
    
  _data = other->_data;
  _path = other->_path;
  _dataset = dataset;
  
  _extent = other->_extent;
  _basesize = other->_basesize;
  
  _format_group = other->_format_group;
  
  //just copy opencv matrix - storage is shared (data must no be written from original owner afterwards!)
  if (other->_memonly) {
    //just copy over
    _mat = other->_mat;
    _readonly = true;
    _memonly = true;
  }
  else { //link to an actual dataset in a file
    _memonly = false;
    if (other->_link_file.size()) {
      _link_file = other->_link_file;
      _link_path = other->_link_path;
    }
    else {
      _link_file = other->_dataset->file().path().string();
      _link_path = (other->_dataset->path() / other->_path).generic_string();
    }
    
    h5_create_path_groups(dataset->f(), path(_link_path).parent_path().generic_string().c_str());
    H5Lcreate_external(_link_file.c_str(), _link_path.c_str(), dataset->f().getId(), _link_path.c_str(), H5P_DEFAULT, H5P_DEFAULT);
  }  
}

static void get_format_fields(Dataset *set, path format_group, DataOrg &org, DataOrder &order, int &channels)
{
  channels = 0;
  set->getEnum(format_group / "organisation", org);
  set->getEnum(format_group / "order",        order);
  
  if (org <= DataOrg::INVALID)
    org = DataOrg::PLANAR;
  
  if (order <= DataOrder::INVALID)
    order = DataOrder::INVALID;
  
  if (org == DataOrg::PLANAR)
    switch (order) {
      case DataOrder::RGB : channels = 3; break;
      case DataOrder::SINGLE : channels = 1;
      case DataOrder::RGGB :
      case DataOrder::BGGR :
      case DataOrder::GBRG :
      case DataOrder::GRBG :
        printf("ERROR: use of planar organisation with bayer pattern ordering!");
        abort();
    }
  else if (org == DataOrg::BAYER_2x2)
    channels = 1;
}

//generic init
//all dimensions which are zero will be set to H5S_UNLIMITED rest is fixed
//per default will chunk using first two dims
//FIXME difference between create/open
void Datastore::create_store()
{
  path dataset_path = _dataset->path() / _path;
  
  if (h5_obj_exists(_dataset->f(), dataset_path.generic_string())) {
    //FIXME
    printf("FIXME dataset already exists, check size?");
    _data = _dataset->f().openDataSet(dataset_path.generic_string());
    abort();
  }
  
  hsize_t *dims = new hsize_t[_extent.size()];
  hsize_t *maxdims = new hsize_t[_extent.size()];
  
  for(int i=0;i<_extent.size();i++)
    if (_extent[i]) {
      dims[_extent.size()-i-1] = _extent[i];
      maxdims[_extent.size()-i-1] = _extent[i];
    }
    else {
      dims[_extent.size()-i-1] = 0;
      maxdims[_extent.size()-i-1] = H5S_UNLIMITED;
    }
  
  h5_create_path_groups(_dataset->f(), path(dataset_path.generic_string().c_str()).parent_path());
  
  H5::DSetCreatPropList prop; 
  
  //chunking fixed for now
  if (_extent.size() >= 3 && _extent[0] > 1 && _extent[1] > 1) {
    hsize_t *chunk_dims = new hsize_t[_extent.size()];
    for(int i=0;i<2;i++)
      chunk_dims[_extent.size()-i-1] = _extent[i];
    for(int i=2;i<_extent.size();i++)
      chunk_dims[_extent.size()-i-1] = 1;
        
    prop.setChunk(_extent.size(), chunk_dims);
    delete chunk_dims;
  }
  //prop.setDeflate(6);
  /*unsigned int szip_options_mask = H5_SZIP_NN_OPTION_MASK;
  unsigned int szip_pixels_per_block = 16;
  prop.setSzip(szip_options_mask, szip_pixels_per_block);*/
  
  H5::DataSpace space(_extent.size(), dims, maxdims);
  
  _data = _dataset->f().createDataSet(dataset_path.generic_string(), 
                      toH5DataType(_type), space, prop);
  
  delete dims;
  delete maxdims;
}

void Datastore::create_types(BaseType type)
{
  if (_format_group.size()) {
    int channels;
    get_format_fields(_dataset, _format_group, _org, _order, channels);
    if (_extent.size() >= 3 && channels && _extent[2] && _extent[2] != channels) {
      printf("ERROR: channel count mismatch detected!\n");
      abort();
    }
    if (type > BaseType::INVALID && type != _type) {
      printf("ERROR: supplied type differs from specification in format group!\n");
      abort();
    }
  }
  else {
    if (type <= BaseType::INVALID) {
      printf("ERROR: no type specifid on type init!\n");
      abort();
    }
    _type = type;
    _org = DataOrg::PLANAR;
    _order = DataOrder::SINGLE;
  }
}

//init store for image storage
void Datastore::create_dims_imgs(int w, int h, int chs)
{
  assert(_extent.size());
  
  _extent[0] = w;
  _extent[1] = h;
  _extent[2] = chs;
  
  for(int i=0;i<_extent.size()-3;i++)
    _extent[i+3] = 0;
  
  _basesize = _extent;
}

//FIXME set all members!
/*void Datastore::init(hsize_t w, hsize_t h)
{
  _dataset->getEnum("format/type",         _type);
  _dataset->getEnum("format/organisation", _org);
  _dataset->getEnum("format/order",        _order);
  
  if (int(_type) == -1 || int(_org) == -1 || int(_order) == -1) {
    printf("ERROR: unsupported dataset format!\n");
    return;
  }
  
  hsize_t comb_w = w*combinedTypeElementCount(_type,_org,_order);
  hsize_t comb_h = h*combinedTypePlaneCount(_type,_org,_order);
  
  hsize_t dims[3] = {0, comb_h,comb_w};
  hsize_t maxdims[3] = {H5S_UNLIMITED,comb_h,comb_w}; 
  path dataset_path = _dataset->path() / _path;
  
  if (h5_obj_exists(_dataset->f, dataset_path.generic_string())) {
    _data = _dataset->f.openDataSet(dataset_path.generic_string());
    return;
  }
  
  h5_create_path_groups(_dataset->f, path(dataset_path.generic_string().c_str()).parent_path());
  
  //chunking fixed for now
  hsize_t chunk_dims[3] = {1,comb_h,comb_w};
  H5::DSetCreatPropList prop;    
  prop.setChunk(3, chunk_dims);
  //prop.setDeflate(6);
  /*unsigned int szip_options_mask = H5_SZIP_NN_OPTION_MASK;
  unsigned int szip_pixels_per_block = 16;
  prop.setSzip(szip_options_mask, szip_pixels_per_block);*/
  
  /*H5::DataSpace space(3, dims, maxdims);
  
  _data = _dataset->f.createDataSet(dataset_path.generic_string(), 
                      toH5DataType(_type), space, prop);
}*/

//FIXME scale!
//WARNING idx will change if size changes!
void * Datastore::cache_get(const std::vector<int> idx, int flags, int extra_flags, float scale)
{
  uint64_t idx_sum = 0;
  uint64_t mul = 1;
  for(int i=2;i<idx.size();i++) {
    idx_sum += idx[i]*mul;
    mul *= _extent[i];
  }
  
  uint64_t key = (idx_sum * PROCESS_FLAGS_MAX) | flags | (extra_flags << 16);
  auto it_find = image_cache.find(key);
  
  if (it_find == image_cache.end())
    return NULL;
  else
    return it_find->second;
}

void Datastore::cache_set(const std::vector<int> idx, int flags, int extra_flags, float scale, void *data)
{  
  uint64_t idx_sum = 0;
  uint64_t mul = 1;
  for(int i=2;i<idx.size();i++) {
    idx_sum += idx[i]*mul;
    mul *= _extent[i];
  }
  
  uint64_t key = (idx_sum * PROCESS_FLAGS_MAX) | flags | (extra_flags << 16);
#pragma omp critical(datastore_cache)
  image_cache[key] = data;
}

void Datastore::open(Dataset *dataset, std::string path_, const std::string format_group)
{
  //only fills in internal data
  create(path_, dataset, format_group);
      
  path dataset_path = dataset->path() / path_;
      
  if (!h5_obj_exists(dataset->f(), dataset_path.generic_string().c_str())) {
    printf("error: could not find requested datset: %s\n", dataset_path.generic_string().c_str());
    return;
  }
  
  _data = dataset->f().openDataSet(dataset_path.generic_string().c_str());
  
  H5::DataSpace space = _data.getSpace();
  
  _extent.resize(space.getSimpleExtentNdims());
  _basesize.resize(space.getSimpleExtentNdims());
  
  hsize_t *dims = new hsize_t[_extent.size()];
  hsize_t *maxdims = new hsize_t[_extent.size()];
  
  space.getSimpleExtentDims(dims, maxdims);
  
  for(int i=0;i<_extent.size();i++) {
    _extent[i] = dims[_extent.size()-i-1];
    if (maxdims[_extent.size()-i-1] == H5S_UNLIMITED)
      _basesize[i] = 0;
    else
      _basesize[i] = maxdims[_extent.size()-i-1];
  }

  int channels;
  if (_format_group.size()) {
    get_format_fields(dataset, format_group, _org, _order, channels);
    if (_basesize.size() >= 2 && _basesize[2] && channels && channels != _basesize[2]) {
      printf("ERROR: channel count mismatch detected!\n");
      abort();
    }
  }
  _type = toBaseType(H5Dget_type(_data.getId()));
  
  assert(_type > BaseType::INVALID);
}

//FIXME chekc w,h?
void Datastore::writeRawImage(int idx, hsize_t w, hsize_t h, void *imgdata)
{
  assert(!_readonly);
  
  //if (!valid())
    //FIXME how to specify dimensionality?
    //init_from_attributes(w, h, 1);
  //FIXME FIXTHIS
  
  H5::DataSpace space = _data.getSpace();
  hsize_t dims[3];
  hsize_t maxdims[3];
  
  space.getSimpleExtentDims(dims, maxdims);
  
  if (dims[0] <= idx) {
    dims[0] = idx+1;
    _data.extend(dims);
    space = _data.getSpace();
  }
  
  hsize_t size[3] = {1, dims[1],dims[2]};
  hsize_t start[3] = {idx, 0,0};
  space.selectHyperslab(H5S_SELECT_SET, size, start);
  
  H5::DataSpace imgspace(3, size);
  
  _data.write(imgdata, toH5DataType(_type), imgspace, space);
}

void Datastore::appendRawImage(hsize_t w, hsize_t h, void *imgdata)
{
  int idx = 0;
  if (valid()) {
    H5::DataSpace space = _data.getSpace();
    hsize_t dims[3];
    hsize_t maxdims[3];
    
    space.getSimpleExtentDims(dims, maxdims);
    
    idx = dims[0];
  }
  
  writeRawImage(idx, w, h, imgdata);
}

hsize_t *new_h5_dim_vec_from_extent(std::vector<int> extent)
{
  hsize_t *v = new hsize_t[extent.size()];
  
  for(int i=0;i<extent.size();i++)
    v[i] = extent[extent.size()-i-1];
  
  return v;
}

void Datastore::writeChannel(const std::vector<int> &idx, cv::Mat *channel)
{
  if (_type <= BaseType::INVALID) {
    setDims(idx.size());
    create_types(CvDepth2BaseType(channel->depth()));
    create_dims_imgs(channel->size().width, channel->size().height, 1);
    create_store();
  }
  
  
  hsize_t *dims = NULL;
  H5::DataSpace space = _data.getSpace();
  bool extend = false;
  hsize_t *size;
  hsize_t *start;
  std::vector<int> ch_size(idx.size(), 1);
  
  assert(idx.size() == _extent.size());
  assert(channel->isContinuous());
  
  for(int i=0;i<idx.size();i++)
    if (_extent[i] <= idx[i]) {
      extend = true;
      _extent[i] = idx[i]+1;
    }
  
  if (extend) {
    dims = new_h5_dim_vec_from_extent(_extent);
    _data.extend(dims);
    space = _data.getSpace();
  }
  
  ch_size[0] = _basesize[0];
  ch_size[1] = _basesize[1];
  
  size = new_h5_dim_vec_from_extent(ch_size);
  start = new_h5_dim_vec_from_extent(idx);
  
  space.selectHyperslab(H5S_SELECT_SET, size, start);
  
  H5::DataSpace imgspace(idx.size(), size);
  
  _data.write(channel->data, toH5DataType(_type), imgspace, space);
}

bool Datastore::mat_cache_get(cv::Mat *m, const std::vector<int> idx, int flags, int extra_flags, float scale)
{
  if (flags & NO_MEM_CACHE)
    return false;
  
  cv::Mat *cached = (cv::Mat*)cache_get(idx,flags,extra_flags,scale);
  
  if (!cached)
    return false;
  
  if (flags & FORCE_REUSE)
    cached->copyTo(*m);
  else
    *m = *cached;
  
  return true;
}

void Datastore::mat_cache_set(cv::Mat *m, const std::vector<int> idx, int flags, int extra_flags, float scale)
{
  if (flags & NO_MEM_CACHE)
    return;
  
  cv::Mat *cached = new cv::Mat();
  //operator= should references if self-managed matrix, else copies
#if CV_MAJOR_VERSION >= 3
  bool shared = m->u;
#else
  bool shared = m->refcount;
#endif

  if (shared)
    *cached = *m;
  else
    m->copyTo(*cached);
  cache_set(idx,flags,extra_flags,scale,cached);
}


void apply_flags_channel(Datastore *store, cv::Mat *in, cv::Mat *out, int flags, double min = std::numeric_limits<float>::quiet_NaN(), double max = std::numeric_limits<float>::quiet_NaN())
{
  //for format conversion
  cv::Mat *curr = in;
  
  if (flags & CVT_8U) {
    if (curr->depth() == CV_8U) {} //do nothing
    else if (curr->depth() == CV_16U) {
      if (isnan(min) || isnan(max))
        *curr *= 1.0/256.0;
      else {
        *curr -= min;
        *curr *= 256.0 / (max-min);
      }
      curr->convertTo(*out, CV_8U);
      curr = out;
    }
    else {
      if (!isnan(min) && !isnan(max)) {
        *curr -= min;
        *curr *= 256.0 / (max-min);
      }
      curr->convertTo(*out, CV_8U);
      curr = out;
    }
  }
  
  if (flags & UNDISTORT) {
    Intrinsics *i = &store->getDataset()->intrinsics;
    //FIXMe getundistmap (should be) generic!
    if (i->model == DistModel::INVALID) {} // do nothing
    else if (i->model == DistModel::CV8) {
      cv::Mat *chap = i->getUndistMap(0, in->size[1], in->size[0]);
      //cv::undistort(*ch,newm, i->cv_cam, i->cv_dist);
      //cv::setNumThreads(0);
      remap(*curr, *out, *chap, cv::noArray(), cv::INTER_LINEAR);
    }
    else
      printf("distortion model not supported: %s\n", enum_to_string(i->model));
  }
  else if (curr->data != out->data)
    curr->copyTo(*out);
}

static cv::Mat mat_2d_from_3d(const cv::Mat &m, int ch)
{
  //step is already in bytes!
  return cv::Mat(m.size[1], m.size[2], m.depth(), m.data + ch*m.step[0]);
}

void apply_flags_image(cv::Mat *in, cv::Mat *out, int flags)
{
  if (flags & CVT_GRAY) {
    int outsize[3];
    outsize[2] = in->size[2];
    outsize[1] = in->size[1];
    outsize[0] = 1;
    
    out->create(3, outsize, in->depth());
    
    cv::Mat sum(out->size[1], out->size[2], CV_32F, cv::Scalar(0.0));
    
    for(int i=0;i<in->size[0];i++) {
      cv::Mat channel = mat_2d_from_3d(*in, i);
      cv::add(sum, channel, sum, cv::noArray(), sum.type());
    }
    
    sum *= 1.0/in->size[0];
    cv::Mat out2d = mat_2d_from_3d(*out, 0);
    sum.convertTo(out2d, out->type());
  }
  else if (out->data != in->data) {
    *out = *in;
  }
}

void Datastore::readChannel(const std::vector<int> &idx, cv::Mat *channel, int flags)
{
  std::vector<int> ch_size(idx.size(), 1);
  float scale = -1.0;
  
  if (mat_cache_get(channel,idx,flags,CACHE_CONT_MAT_CHANNEL,scale))
    return; 

  hsize_t *dims = NULL;
  bool extend = false;
  hsize_t *size;
  hsize_t *start;

  
  assert(idx.size() == _extent.size());
  
  
  ch_size[0] = _basesize[0];
  ch_size[1] = _basesize[1];
  
  size = new_h5_dim_vec_from_extent(ch_size);
  start = new_h5_dim_vec_from_extent(idx);
  
  
  //just stores memory buffer again
  uchar *data = channel->data;
  cv::Mat reader = *channel;
  
  //if reader has right format nothing will happens else, we can read correct format and convert later
  reader.create(_basesize[1], _basesize[0], BaseType2CvDepth(_type));

  //find out wether format changed?
//protect hdf5
#pragma omp critical(hdf5)
  {
  H5::DataSpace imgspace(idx.size(), size);
  H5::DataSpace space;
  space = _data.getSpace();
  space.selectHyperslab(H5S_SELECT_SET, size, start);
  _data.read(reader.data, toH5DataType(_type), imgspace, space);
  }
  
  //FIXME create channel wit correct parameters!
  apply_flags_channel(this, &reader, channel, flags);
  if ((data != channel->data) && (flags & FORCE_REUSE)) {
    printf("memcpy desired but image data in wrong format!\n");
    abort();
  }
  
  mat_cache_set(channel,idx,flags,CACHE_CONT_MAT_CHANNEL,scale);
}

int order2cv_conf_flag(DataOrder order)
{
  switch (order) {
    case DataOrder::RGGB :
      return CV_BayerRG2BGR;
    case DataOrder::BGGR :
      return CV_BayerBG2BGR;
    case DataOrder::GBRG :
      return CV_BayerGB2BGR;
    case DataOrder::GRBG :
      return CV_BayerGR2BGR;
    default :
      abort();
  }
}

//this is always a 3d mat!
void Datastore::readImage(const std::vector<int> &idx, cv::Mat *img, int flags, double min, double max)
{
  float scale = -1.0;
  int imgsize[3], output_size[3];
  imgsize[2] = _basesize[0];
  imgsize[1] = _basesize[1];
  //FIXME may be changed by flags!
  imgsize[0] = _basesize[2];
  output_size[2] = imgsize[2];
  output_size[1] = imgsize[1];
  output_size[0] = imgsize[0];
  int depth = BaseType2CvDepth(_type);
  int output_depth = depth;
  int channel_flags = flags;
  bool demosaic = false;
  bool disable_cache = false;
  
  if (!isnan(min) && !isnan(max))
    disable_cache = true;
  
  if (flags & CVT_GRAY)
    flags |= DEMOSAIC;
  if (flags & UNDISTORT)
    flags |= DEMOSAIC;
  if (disable_cache)
    flags |= NO_MEM_CACHE;
    
  if (mat_cache_get(img,idx,flags,CACHE_CONT_MAT_IMG,scale))
    return;
  
  printf("read idx %d\n", idx[3]);
  
  if ((flags & DEMOSAIC) && _org == DataOrg::BAYER_2x2)
    demosaic = true;
  
  assert((flags & FORCE_REUSE) == 0);
  
  if (flags & CVT_8U)
    output_depth = CV_8U;
  if (flags & CVT_GRAY)
    output_size[0] = 1;
  else if (demosaic)
    output_size[0] = 3;
  
  img->create(3,output_size,output_depth);
  //for now
  assert(img->isContinuous());
  
  std::vector<int> ch_idx = idx;
  
  cv::Mat reader = *img;
  //if input and output are the same type this is a no-op
  reader.create(3,imgsize,depth);
  
  cv::Mat channel_in, channel_out, tmp;
    
  if (demosaic) {
    printf("demosaic!\n");
    cv::Mat channel, img_rgb;
    
    //read raw channels
    readChannel(ch_idx, &channel, NO_MEM_CACHE);
    cv::cvtColor(channel, channel, order2cv_conf_flag(_order));
    
    cv2ClifMat(&channel, &img_rgb);
    
    apply_flags_image(&img_rgb, &tmp, flags);
    
    assert(tmp.size[0] = output_size[0]);
    
    //convert
    for(int i=0;i<tmp.size[0];i++) {
      channel_in = mat_2d_from_3d(tmp, i);
      channel_out = mat_2d_from_3d(*img, i);
      apply_flags_channel(this, &channel_in, &channel_out, flags, min, max);
    }
  }
  else {
    for(int i=0;i<_extent[2];i++) {
      channel_in = mat_2d_from_3d(reader, i);
      ch_idx[2] = i;    
      readChannel(ch_idx, &channel_in, NO_MEM_CACHE | FORCE_REUSE);
    }
  
    apply_flags_image(&reader, &tmp, flags);
    
    for(int i=0;i<output_size[0];i++) {
      channel_in = mat_2d_from_3d(tmp, i);
      channel_out = mat_2d_from_3d(*img, i);
      apply_flags_channel(this, &channel_in, &channel_out, flags, min, max);
    }
  }
  
  mat_cache_set(img,idx,flags,CACHE_CONT_MAT_IMG,scale);
}

void Datastore::setDims(int dims)
{
  _extent.resize(0);
  _extent.resize(dims);
  
  _basesize = _extent;
}

void Datastore::init_write(const std::vector<int> &idx, cv::Mat *img)
{
  if (_type <= BaseType::INVALID) {
    setDims(idx.size());
    create_types(CvDepth2BaseType(img->depth()));
    create_dims_imgs(img->size().width, img->size().height, img->channels());
    create_store();
  }
}

void Datastore::writeImage(const std::vector<int> &idx, cv::Mat *img)
{
  init_write(idx, img);
  
  assert(idx[0] == 0);
  assert(idx[1] == 0);
  assert(idx[2] == 0);
  
  if (img->dims == 2) {
    if (img->channels() == 1)
      writeChannel(idx, img);
    else {
      std::vector<cv::Mat> channels;
      std::vector<int> ch_idx = idx;
      
      cv::split(*img,channels);
      for(int i=0;i<channels.size();i++) {
        ch_idx[2] = i;
        writeChannel(ch_idx, &channels[i]);
      }
    }
  }
  else {
    printf("FIXME direct 3-d img write!\n");
    abort();
  }
}


void Datastore::appendImage(cv::Mat *img)
{  
  if (!_extent.size()) {
    printf("ERROR datastore must have dimension!\n");
    abort();
  }
  
  std::vector<int> idx(_extent.size(), 0);  
  
  init_write(idx, img);
  
  //default to append at idx 3 (next image)
  idx[3] = _extent[3];
  writeImage(idx, img);
}

//FIXME implement 8-bit conversion if requested
void Datastore::readRawImage(int idx, hsize_t w, hsize_t h, void *imgdata)
{
  H5::DataSpace space = _data.getSpace();
  hsize_t dims[3];
  hsize_t maxdims[3];
  
  space.getSimpleExtentDims(dims, maxdims);
  
  if (dims[0] <= idx)
    throw std::invalid_argument("requested index out or range");
  
  hsize_t size[3] = {1, dims[1],dims[2]};
  hsize_t start[3] = {idx, 0,0};
  space.selectHyperslab(H5S_SELECT_SET, size, start);
  
  H5::DataSpace imgspace(3, size);
  
  _data.read(imgdata, toH5DataType(_type), imgspace, space);
}

bool Datastore::valid() const
{
  //FIXME memonly?
  if (_type <= BaseType::INVALID)
    return false;
  if (!_dataset)
    return false;
  return true;
}

int Datastore::dims() const
{
  return _extent.size();
}

//when intepreting as image store
int Datastore::imgCount()
{
  int count = 1;
  for(int i=3;i<_extent.size();i++)
    count *= _extent[i];
  
  return count;
}
// 
//when intepreting as image store
int Datastore::imgChannels()
{
  //FIXME bayer pattern!
  return _extent[2];
}

const std::vector<int>& Datastore::extent() const
{
  return _extent;
}

std::ostream& operator<<(std::ostream& out, const Datastore& a)
{  
  if(!a.valid() || !a._extent.size())
    return out;

  for(int i=0;i<a._extent.size()-1;i++)
    out << a._extent[i] << " x ";
  out << a._extent[a._extent.size()-1];
  
  return out;
}

void Datastore::flush()
{
  if (!valid())
    return;
  
  if (_memonly) {
    if (_dataset->memoryFile()) {
      _mat.release();
    }
    else {
      printf("flush %s\n", _path.c_str());
      //write memory-only data into file (and convert datastore to regular)
      //FIXME _data should be empty/invalid h5dataset!
      
      path fullpath = _dataset->path() / _path;
      
      if (h5_obj_exists(_dataset->f(), fullpath)) {
        printf("TODO overwrite (store %s exists)!\n", fullpath.c_str());
        abort();
      }
      
      h5_create_path_groups(_dataset->f(), fullpath.parent_path());
      
      hsize_t *dims = new hsize_t[_mat.size()];
      for(int i=0;i<_mat.size();i++)
        dims[i] = _mat[_mat.size()-i-1];
      
      //H5::DSetCreatPropList prop;    
      //prop.setChunk(dimcount, chunk_dims);
      
      H5::DataSpace space(_mat.size(), dims, dims);
      
      _data = _dataset->f().createDataSet(fullpath.generic_string().c_str(), 
                                          toH5DataType(_mat.type()), space);
      
      _data.write(_mat.data(), H5::DataType(toH5DataType(_mat.type())), space, space);
      
      _readonly = false;
      _memonly = false;
      _mat.release();
      delete dims;
    }
  }
  else
    assert(_mat.total() == 0);
  
}

Datastore::~Datastore()
{
  flush();
  
  for(auto i=image_cache.begin();i!=image_cache.end();++i) {
    if ((i->first >> 16) & (CACHE_CONT_MAT_CHANNEL | CACHE_CONT_MAT_IMG))
      delete (cv::Mat*)i->second;
  }
}

}