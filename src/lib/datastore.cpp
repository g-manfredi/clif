#include "datastore.hpp"

#include "clif.hpp"

#include "dataset.hpp"

//#include "clif.hpp"

namespace clif {

//FIXME wether dataset already exists and overwrite?
//FIXME set remaining members!
void Datastore::create(std::string path, Dataset *dataset)
{
  assert(dataset);
  
  _type = BaseType::INVALID; 
  _org = DataOrg(-1);
  _order = DataOrder(-1); 
    
  _data = H5::DataSet();
  _path = path;
  _dataset = dataset;
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
                      H5PredType(_type), space, prop);
}*/

void Datastore::create(std::string path, Dataset *dataset, cv::Mat &m)
{  
  _type = BaseType(CvDepth2BaseType(m.depth())); 
  _org = DataOrg(-1);
  _order = DataOrder(-1); 
    
  _data = H5::DataSet();
  _path = path;
  _dataset = dataset;
  
  _readonly = false;
  _memonly = true;
  
  _mat = m;
}

//read store into m 
void Datastore::read(cv::Mat &m)
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
  }
  
  _mat = m;
}

void Datastore::link(const Datastore *other, Dataset *dataset)
{
  assert(dataset);
  
  _readonly = true;
  
  _type = other->_type; 
  _org = other->_org;
  _order = other->_order; 
    
  //TODO is this ok?
  _data = other->_data;
  _path = other->_path;
  _dataset = dataset;
  
  //just copy opencv matrix - storage is shared (data must no be written from original owner afterwards!)
  if (other->_memonly) {
    _mat = other->_mat;
    _readonly = true;
    _memonly = true;
  }
  else { //link to an actual dataset in a file
    if (other->_link_file.size()) {
      _link_file = other->_link_file;
      _link_path = other->_link_path;
    }
    else {
      _link_file = other->_dataset->f.getFileName().c_str();
      _link_path = (other->_dataset->path() / other->_path).string();
    }
    
    h5_create_path_groups(dataset->f, path(_link_path).parent_path().c_str());
    
    H5Lcreate_external(_link_file.c_str(), _link_path.c_str(), dataset->f.getId(), _link_path.c_str(), H5P_DEFAULT, H5P_DEFAULT);
  }  
}

//FIXME set all members!
void Datastore::init(hsize_t w, hsize_t h)
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
  
  hsize_t dims[3] = {comb_w,comb_h,0};
  hsize_t maxdims[3] = {comb_w,comb_h,H5S_UNLIMITED}; 
  path dataset_path = _dataset->path() / _path;
  
  if (h5_obj_exists(_dataset->f, dataset_path.c_str())) {
    _data = _dataset->f.openDataSet(dataset_path.c_str());
    return;
  }
  
  h5_create_path_groups(_dataset->f, path(dataset_path.c_str()).parent_path());
  
  //chunking fixed for now
  hsize_t chunk_dims[3] = {comb_w,comb_h,1};
  H5::DSetCreatPropList prop;    
  prop.setChunk(3, chunk_dims);
  //prop.setDeflate(6);
  /*unsigned int szip_options_mask = H5_SZIP_NN_OPTION_MASK;
  unsigned int szip_pixels_per_block = 16;
  prop.setSzip(szip_options_mask, szip_pixels_per_block);*/
  
  H5::DataSpace space(3, dims, maxdims);
  
  _data = _dataset->f.createDataSet(dataset_path.c_str(), 
                      H5PredType(_type), space, prop);
}

//FIXME scale!
void * Datastore::cache_get(int idx, int flags, float scale)
{
  uint64_t key = (idx * PROCESS_FLAGS_MAX) | flags;
  auto it_find = image_cache.find(key);
  
  if (it_find == image_cache.end())
    return NULL;
  else
    return it_find->second;
}

void Datastore::cache_set(int idx, int flags, float scale, void *data)
{
  uint64_t key = (idx * PROCESS_FLAGS_MAX) | flags;
  image_cache[key] = data;
}

void Datastore::open(Dataset *dataset, std::string path_)
{
  //only fills in internal data
  create(path_, dataset);
      
  path dataset_path = dataset->path() / path_;
      
  if (!h5_obj_exists(dataset->f, dataset_path.c_str())) {
    printf("error: could not find requrested datset: %s\n", dataset_path.c_str());
    return;
  }
  
  dataset->getEnum("format/type",         _type);
  dataset->getEnum("format/organisation", _org);
  dataset->getEnum("format/order",        _order);

  if (int(_type) == -1 || int(_org) == -1 || int(_order) == -1) {
    printf("ERROR: unsupported datastore format!\n");
    return;
  }
  
  _data = dataset->f.openDataSet(dataset_path.c_str());
  printf("opened h5 dataset %s id %d\n", dataset_path.c_str(), _data.getId());
  
  printf("Datastore open %s: %s %s %s\n", dataset_path.c_str(), ClifEnumString(BaseType,_type),ClifEnumString(DataOrg,_org),ClifEnumString(DataOrder,_order));
}

//FIXME chekc w,h?
void Datastore::writeRawImage(uint idx, hsize_t w, hsize_t h, void *imgdata)
{
  assert(!_readonly);
  
  if (!valid())
    init(w, h);
  
  H5::DataSpace space = _data.getSpace();
  hsize_t dims[3];
  hsize_t maxdims[3];
  
  space.getSimpleExtentDims(dims, maxdims);
  
  if (dims[2] <= idx) {
    dims[2] = idx+1;
    _data.extend(dims);
    space = _data.getSpace();
  }
  
  hsize_t size[3] = {dims[0],dims[1],1};
  hsize_t start[3] = {0,0,idx};
  space.selectHyperslab(H5S_SELECT_SET, size, start);
  
  H5::DataSpace imgspace(3, size);
  
  _data.write(imgdata, H5PredType(_type), imgspace, space);
}

void Datastore::appendRawImage(hsize_t w, hsize_t h, void *imgdata)
{
  int idx = 0;
  if (valid()) {
    H5::DataSpace space = _data.getSpace();
    hsize_t dims[3];
    hsize_t maxdims[3];
    
    space.getSimpleExtentDims(dims, maxdims);
    
    idx = dims[2];
  }
  
  writeRawImage(idx, w, h, imgdata);
}

//FIXME implement 8-bit conversion if requested
void Datastore::readRawImage(uint idx, hsize_t w, hsize_t h, void *imgdata)
{
  H5::DataSpace space = _data.getSpace();
  hsize_t dims[3];
  hsize_t maxdims[3];
  
  space.getSimpleExtentDims(dims, maxdims);
  
  if (dims[2] <= idx)
    throw std::invalid_argument("requested index out or range");
  
  hsize_t size[3] = {dims[0],dims[1],1};
  hsize_t start[3] = {0,0,idx};
  space.selectHyperslab(H5S_SELECT_SET, size, start);
  
  H5::DataSpace imgspace(3, size);
  
  _data.read(imgdata, H5PredType(_type), imgspace, space);
}

bool Datastore::valid()
{
  if (_data.getId() == H5I_INVALID_HID)
    return false;
  return true;
}

void Datastore::size(int s[3])
{
  H5::DataSpace space = _data.getSpace();
  hsize_t dims[3];
  
  space.getSimpleExtentDims(dims);
  
  s[0] = dims[0];
  s[1] = dims[1];
  s[2] = dims[2];
}

int Datastore::count()
{
  int store_size[3];
  size(store_size);
  
  return store_size[2];
}

void Datastore::imgSize(int s[2])
{
  if (_imgsize[0] == -1) {
    int store_size[3];
    size(store_size);
    
    s[0] = store_size[0]/combinedTypeElementCount(_type,_org,_order);
    s[1] = store_size[1]/combinedTypePlaneCount(_type,_org,_order); 
    _imgsize[0] = s[0];
    _imgsize[1] = s[1];
  }
  else {
    s[0] = _imgsize[0];
    s[1] = _imgsize[1];
  }
}

}