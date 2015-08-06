#include "clif.hpp"

#include <string.h>
#include <string>
#include <assert.h>

#include <iostream>
#include <exception>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

static bool _hdf5_obj_exists(H5::H5File &f, const char * const group_str)
{
  H5E_auto2_t  oldfunc;
  void *old_client_data;
  
  H5Eget_auto(H5E_DEFAULT, &oldfunc, &old_client_data);
  H5Eset_auto(H5E_DEFAULT, NULL, NULL);
  
  int status = H5Gget_objinfo(f.getId(), group_str, 0, NULL);
  
  H5Eset_auto(H5E_DEFAULT, oldfunc, old_client_data);
  
  if (status == 0)
    return true;
  return false;
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

    printf("check %s\n", buf);
    if (!_hdf5_obj_exists(f, buf)) {
      printf("create %s\n", buf);
      f.createGroup(buf);
    }

    next_ptr = strchr(next_ptr+1, '/');
  }
  
  buf[last_pos] = '/';
  if (!_hdf5_obj_exists(f, buf))
    f.createGroup(buf);
  
  free(buf);
}

static std::string appendToPath(std::string str, std::string append)
{
  if (str.back() != '/')
    str = str.append("/");
    
  return str.append(append);
}

void save_string_attr(H5::Group &g, const char *name, const char *val)
{
  H5::DataSpace string_space(H5S_SCALAR);
  H5::StrType strdatatype(H5::PredType::C_S1, strlen(val)+1);
  H5::Attribute attr = g.createAttribute(name, strdatatype, string_space);
  attr.write(strdatatype, val);
}

std::string read_string_attr(H5::H5File &f, H5::Group &group, const char *name)
{
  std::string strreadbuf ("");
  
  H5::Attribute attr = group.openAttribute(name);
  
  H5::StrType strdatatype(H5::PredType::C_S1, attr.getStorageSize());
  
  attr.read(strdatatype, strreadbuf);
  
  return strreadbuf;
}

std::string read_string_attr(H5::H5File &f, const char *parent_group_str, const char *name)
{
  H5::Group group = f.openGroup(parent_group_str);
  return read_string_attr(f, group, name);
}

int parse_string_enum(std::string &str, const char **enumstrs)
{
  int i=0;
  while (enumstrs[i]) {
    if (!str.compare(enumstrs[i]))
      return i;
    i++;
  }
  
  return -1;
}

namespace clif {
  Datastore::Datastore(H5::H5File &f, const std::string parent_group_str, uint width, uint height, uint count, DataType datatype, DataOrg dataorg, DataOrder dataorder)
  : type(datatype), org(dataorg), order(dataorder) {
    hsize_t dims[3] = {width,height, 0};
    hsize_t maxdims[3] = {width,height,H5S_UNLIMITED}; 
    std::string dataset_str = parent_group_str;
    dataset_str = appendToPath(dataset_str, "data");
    
    printf("type: %s\n", ClifEnumString(DataType,type));
    
    if (_hdf5_obj_exists(f, parent_group_str.c_str()))
      f.unlink(parent_group_str);
    
    printf("make %s\n", parent_group_str.c_str());
    _rec_make_groups(f, parent_group_str.c_str());
    
    H5::Group parent_group = f.openGroup(parent_group_str.c_str());
    H5::Group format_group = f.createGroup(appendToPath(parent_group_str, "format"));
    
    save_string_attr(format_group, "type", ClifEnumString(DataType,type));
    save_string_attr(format_group, "organsiation", ClifEnumString(DataOrg,org));
    save_string_attr(format_group, "order", ClifEnumString(DataOrder,order));
    
    //chunking fixed for now
    hsize_t chunk_dims[3] = {width,height,1};
    H5::DSetCreatPropList prop;
    prop.setChunk(3, chunk_dims);
    
    H5::DataSpace space(3, dims, maxdims);
    
    data = f.createDataSet(dataset_str, 
	               H5PredType(type), space, prop);
  }
  
  Datastore::Datastore(H5::H5File &f, const std::string parent_group_str)
  {
    std::string attr_str;
    std::string dataset_str = appendToPath(parent_group_str, "data");
    int res;
    
    H5::Group format = f.openGroup(appendToPath(parent_group_str, "format"));
    
    //FIXME too much copy paste, need error convention
    attr_str = read_string_attr(f,format, "type");
    res = parse_string_enum(attr_str, DataTypeStr);
    if (res == -1) {
      printf("error: invlid enum string %s for enum %s!\n", attr_str.c_str(), "DataType");
      return;
    }
    type = (DataType)res;
    
    attr_str = read_string_attr(f,format, "organsiation");
    res = parse_string_enum(attr_str, DataOrgStr);
    if (res == -1) {
      printf("error: invlid enum string %s for enum %s!\n", attr_str.c_str(), "DataOrg");
      return;
    }
    org = (DataOrg)res;
    
    attr_str = read_string_attr(f,format, "order");
    res = parse_string_enum(attr_str, DataOrderStr);
    if (res == -1) {
      printf("error: invlid enum string %s for enum %s!\n", attr_str.c_str(), "DataOrder");
      return;
    }
    order = (DataOrder)res;
    
    if (!_hdf5_obj_exists(f, dataset_str.c_str()))
      return;

    data = f.openDataSet(dataset_str);
  }

  
  void Datastore::writeRawImage(uint idx, void *imgdata)
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
    
    H5::DataSpace imgspace(3, size);
    
    assert(org == DataOrg::BAYER_2x2);
    
    data.write(imgdata, H5PredType(type), imgspace, space);
  }
  
  void Datastore::readRawImage(uint idx, void *imgdata)
  {
    H5::DataSpace space = data.getSpace();
    hsize_t dims[3];
    hsize_t maxdims[3];
    
    space.getSimpleExtentDims(dims, maxdims);
    
    if (dims[2] <= idx)
      throw std::invalid_argument("requested index out or range");
    
    hsize_t size[3] = {dims[0],dims[1],1};
    hsize_t start[3] = {0,0,idx};
    space.selectHyperslab(H5S_SELECT_SET, size, start);
    
    H5::DataSpace imgspace(3, size);
    
    //FIXME other types need size calculation?
    assert(org == DataOrg::BAYER_2x2);
    
    data.read(imgdata, H5PredType(type), imgspace, space);
  }
  
  bool Datastore::isValid()
  {
    if (data.getId() == H5I_INVALID_HID)
      return false;
    return true;
  }
  
  
  /*void readOpenCvMat(uint idx, Mat &m);
        assert(w == img.size().width);
      assert(h = img.size().height);
      assert(depth = img.depth());
      printf("store idx %d: %s\n", i, in_names[i]);
      lfdata.writeRawImage(i, img.data);*/
  
  int Datastore::imgMemSize()
  {
    int size = 1;
    switch (type) {
      case DataType::UINT8 : break;
      case DataType::UINT16 : size *= 2; break;
    }
    switch (org) {
      case DataOrg::BAYER_2x2 : size *= 4; break;
    }
    
    H5::DataSpace space = data.getSpace();
    hsize_t dims[3];
    hsize_t maxdims[3];
    
    space.getSimpleExtentDims(dims, maxdims);
    
    return size*dims[0]*dims[1];
  }
  
  H5::PredType H5PredType(DataType type)
  {
    switch (type) {
      case DataType::UINT8 : return H5::PredType::STD_U8LE;
      case DataType::UINT16 : return H5::PredType::STD_U16LE;
      default :
        abort();
    }
  }
  
  std::vector<std::string> Datasets(H5::H5File &f)
  {
    std::vector<std::string> list(0);
    
    if (!_hdf5_obj_exists(f, "/clif"))
      return list;
    
    H5::Group g = f.openGroup("/clif");
    
    hsize_t count = g.getNumObjs();
    list.resize(count);
    
    for(int i=0;i<count;i++)
      list[i] = g.getObjnameByIdx(i);
    
    return list;
  }
  
  int Datastore::count()
  {
    H5::DataSpace space = data.getSpace();
    hsize_t dims[3];
    hsize_t maxdims[3];
    
    space.getSimpleExtentDims(dims, maxdims);
    
    return dims[2];
  }
}

namespace clif_cv {
  
  cv::Size CvDatastore::imgSize()
  {
    H5::DataSpace space = data.getSpace();
    hsize_t dims[3];
    hsize_t maxdims[3];
    
    space.getSimpleExtentDims(dims, maxdims);
    
    return cv::Size(dims[0],dims[1]);
  }
  
  //TODO create mappings with cmake?
  DataType CvDepth2DataType(int cv_type)
  {
    switch (cv_type) {
      case CV_8U : return clif::DataType::UINT8;
      case CV_16U : return clif::DataType::UINT16;
      default :
        abort();
    }
  }
  
  int DataType2CvDepth(DataType t)
  {
    switch (t) {
      case clif::DataType::UINT8 : return CV_8U;
      case clif::DataType::UINT16 : return CV_16U;
      default :
        abort();
    }
  }
    
  void CvDatastore::readCvMat(uint idx, cv::Mat &m, int flags)
  {
    assert(org == DataOrg::BAYER_2x2);
    
    //FIXME bayer only for now!
    m = cv::Mat(imgSize(), DataType2CvDepth(type));
    
    readRawImage(idx, m.data);
    
    if (org == DataOrg::BAYER_2x2 && flags & CLIF_DEMOSAIC) {
      switch (order) {
        case DataOrder::RGGB :
          cvtColor(m, m, CV_BayerBG2BGR);
          break;
        case DataOrder::BGGR :
          cvtColor(m, m, CV_BayerRG2BGR);
          break;
        case DataOrder::GBRG :
          cvtColor(m, m, CV_BayerGR2BGR);
          break;
        case DataOrder::GRBG :
          cvtColor(m, m, CV_BayerGB2BGR);
          break;
      }
    }
  }
}

