#include "clif.hpp"

#include <string.h>
#include <string>
#include <assert.h>

#include "H5Cpp.h"
#include "H5File.h"

static bool _hdf5_group_exists(H5::H5File &f, const char * const group_str)
{
  try {
    f.openGroup(group_str);
  }
  catch(H5::FileIException not_found_error) {
    return false;
  }
  return true;
}

static void _rec_make_groups(H5::H5File &f, const char * const group_str) 
{
  int last_pos = 0, next_pos;
  char *buf = strdup(group_str);
  const char *next_ptr = strchr(group_str+1, '/');
  herr_t status;
  
  while(next_ptr != NULL) {
    next_pos = next_ptr-group_str;
    //restore
    buf[last_pos] = '/';
    //limit
    buf[next_pos] = '\0';
    last_pos = next_pos;

    if (!_hdf5_group_exists(f, buf))
      f.createGroup(buf);

    next_ptr = strchr(next_ptr+1, '/');
  }
  
  buf[last_pos] = '/';
  if (!_hdf5_group_exists(f, buf))
    f.createGroup(buf);
  
  free(buf);
}

namespace clif {
  Datastore::Datastore(H5::H5File &f, const std::string parent_group_str, uint width, uint height, uint count, DataType datatype, DataOrg dataorg, DataOrder dataorder)
  : type(datatype), org(dataorg), order(dataorder) {
    hsize_t dims[3] = {width,height, 0};
    hsize_t maxdims[3] = {width,height,H5S_UNLIMITED}; 
    std::string dataset_str = parent_group_str;
    if (dataset_str.back() == '/')
      dataset_str.append("data");
    else
      dataset_str.append("/data");
    
    assert(type == DataType::UINT8);
    assert(org == DataOrg::BAYER_2x2);
    
    
    _rec_make_groups(f, parent_group_str.c_str());
    
    //chunking fixed for now
    hsize_t chunk_dims[3] = {width,height,1};
    H5::DSetCreatPropList prop;
    prop.setChunk(2, chunk_dims);
    
    H5::DataSpace space(3, dims, maxdims);
    
    data = f.createDataSet(parent_group_str, 
	               H5::PredType::STD_U8LE, space, prop);

  }

  
  void Datastore::writeImage(uint idx, void *imgdata)
  {
    H5::DataSpace space = data.getSpace();
    hsize_t dims[3];
    hsize_t maxdims[3];
    
    space.getSimpleExtentDims(dims, maxdims);
    
    if (dims[2] <= idx) {
      dims[2] = idx+1;
      data.extend(dims);
      space = data.getSpace();
    }
    
    hsize_t size[3] = {dims[0],dims[1],1};
    hsize_t start[3] = {0,0,idx};
    space.selectHyperslab(H5S_SELECT_SET, size, start);
    
    H5::DataSpace imgspace(1, size);
    
    data.write(imgdata, H5::PredType::NATIVE_UINT8, imgspace, space);
  }
}
  