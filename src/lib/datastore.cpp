#include "datastore.hpp"

#include "dataset.hpp"

namespace clif {

//FIXME wether dataset already exists and overwrite?
void Datastore::create(std::string path, Dataset *dataset)
{
  assert(dataset);
  
  _type = DataType(-1); 
  _org = DataOrg(-1);
  _order = DataOrder(-1); 
    
  _data = H5::DataSet();
  _path = path;
  _dataset = dataset;
}

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
  std::string dataset_str = _dataset->_path;
  dataset_str = appendToPath(dataset_str, _path);
  
  if (h5_obj_exists(_dataset->f, dataset_str.c_str())) {
    _data = _dataset->f.openDataSet(dataset_str);
    return;
  }
  
  h5_create_path_groups(_dataset->f, path(dataset_str).parent_path());
  //_rec_make_groups(_dataset->f, remove_last_part(dataset_str, '/').c_str());
  
  //chunking fixed for now
  hsize_t chunk_dims[3] = {comb_w,comb_h,1};
  H5::DSetCreatPropList prop;    
  prop.setChunk(3, chunk_dims);
  //prop.setDeflate(6);
  /*unsigned int szip_options_mask = H5_SZIP_NN_OPTION_MASK;
  unsigned int szip_pixels_per_block = 16;
  prop.setSzip(szip_options_mask, szip_pixels_per_block);*/
  
  H5::DataSpace space(3, dims, maxdims);
  
  _data = _dataset->f.createDataSet(dataset_str, 
                      H5PredType(_type), space, prop);
}

void * Datastore::cache_get(std::string key)
{
  auto it_find = image_cache.find(key);
  
  if (it_find == image_cache.end())
    return NULL;
  else
    return it_find->second;
}

void Datastore::cache_set(std::string key, void *data)
{
  image_cache[key] = data;
}

void Datastore::open(Dataset *dataset, std::string path_)
{    
  //only fills in internal data
  create(path_, dataset);
      
  std::string dataset_str = appendToPath(dataset->_path, path_);
      
  if (!h5_obj_exists(dataset->f, dataset_str.c_str())) {
    printf("error: could not find requrested datset: %s\n", dataset_str.c_str());
    return;
  }
  
  dataset->getEnum("format/type",         _type);
  dataset->getEnum("format/organisation", _org);
  dataset->getEnum("format/order",        _order);

  if (int(_type) == -1 || int(_org) == -1 || int(_order) == -1) {
    printf("ERROR: unsupported dataset format!\n");
    return;
  }
  
  _data = dataset->f.openDataSet(dataset_str);
  printf("opened h5 dataset %s\n", dataset_str.c_str());
  
  //printf("Datastore open %s: %s %s %s\n", dataset_str.c_str(), ClifEnumString(DataType,type),ClifEnumString(DataOrg,org),ClifEnumString(DataOrder,order));
}

//FIXME chekc w,h?
void Datastore::writeRawImage(uint idx, hsize_t w, hsize_t h, void *imgdata)
{
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

void Datastore::imgsize(int s[2])
{
  int store_size[3];
  size(store_size);
  
  s[0] = store_size[0]/combinedTypeElementCount(_type,_org,_order);
  s[1] = store_size[1]/combinedTypePlaneCount(_type,_org,_order); 
}

}