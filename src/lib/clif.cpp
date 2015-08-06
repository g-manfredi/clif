#include "clif.hpp"

#include <string.h>
#include <string>
#include <assert.h>

#include <iostream>
#include <exception>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "cliini.h"

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

namespace clif {
  
  BaseType cliini_type_to_BaseType(int type)
  {
    switch (type) {
      case CLIINI_STRING : return BaseType::STRING;
      case CLIINI_DOUBLE : return BaseType::DOUBLE;
      case CLIINI_INT : return BaseType::INT;
    }
    
    printf("ERROR: unknown argument type!\n");
    abort();
  }
  
  BaseType PredType_to_native_BaseType(H5::PredType type)
  {    
    switch (type.getClass()) {
      case H5T_STRING : return BaseType::STRING;
      case H5T_INTEGER : return BaseType::INT;
      case H5T_FLOAT: return BaseType::DOUBLE;
    }
    
    printf("ERROR: unknown argument type!\n");
    abort();
  }
  
  BaseType hid_t_to_native_BaseType(hid_t type)
  {
    switch (H5Tget_class(type)) {
      case H5T_STRING : return BaseType::STRING;
      case H5T_INTEGER : return BaseType::INT;
      case H5T_FLOAT: return BaseType::DOUBLE;
    }
    
    printf("ERROR: unknown argument type!\n");
    abort();
  }
  
  H5::PredType BaseType_to_PredType(BaseType type)
  {    
    switch (type) {
      case BaseType::STRING : return H5::PredType::C_S1;
      case BaseType::INT : return H5::PredType::NATIVE_INT;
      case BaseType::DOUBLE: return H5::PredType::NATIVE_DOUBLE;
    }
    
    printf("ERROR: unknown argument type!\n");
    abort();
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
  
  //FIXME single strings only atm!
  int basetype_size(BaseType type)
  { 
    switch (type) {
      case BaseType::STRING : return sizeof(char);
      case BaseType::INT :    return sizeof(int);
      case BaseType::DOUBLE : return sizeof(double);
      default:
        printf("invalid type!\n");
        abort();
    }
  }

  void *read_attr(H5::Group g, std::string name, BaseType &type, int *size)
  {
    H5::Attribute attr = g.openAttribute(name);
    
    type =  hid_t_to_native_BaseType(H5Aget_type(attr.getId()));
    
    //FIXME 1D only atm!
    
    H5::DataSpace space = attr.getSpace();
    int dimcount = space.getSimpleExtentNdims();
    assert(dimcount == 1);
    hsize_t dims[1];
    hsize_t maxdims[1];
    space.getSimpleExtentDims(dims, maxdims);
    
    void *buf = malloc(basetype_size(type)*dims[1]);
    
    attr.read(BaseType_to_PredType(type), buf);
    
    return buf;
  }

  std::string read_string_attr(H5::H5File &f, const char *parent_group_str, const char *name)
  {
    H5::Group group = f.openGroup(parent_group_str);
    return read_string_attr(f, group, name);
  }

  int parse_string_enum(const char *str, const char **enumstrs)
  {
    int i=0;
    while (enumstrs[i]) {
      if (!strcmp(str,enumstrs[i]))
        return i;
      i++;
    }
    
    printf("unknown enum str %s\n", str);
    
    return -1;
  }
  
  int parse_string_enum(std::string &str, const char **enumstrs)
  {
    int i=0;
    while (enumstrs[i]) {
      if (!str.compare(enumstrs[i]))
        return i;
      i++;
    }
    
    printf("unknown enum str %s\n", str.c_str());
    
    return -1;
  }
  
  Attributes::Attributes(const char *inifile, const char *typefile) 
  {
    cliini_optgroup group = {
      NULL,
      NULL,
      0,
      0,
      CLIINI_ALLOW_UNKNOWN_OPT
    };
    
    cliini_args *attr_args = cliini_parsefile(inifile, &group);
    cliini_args *attr_types = cliini_parsefile(typefile, &group);
    
    attrs.resize(cliargs_count(attr_args));
    
    //FIXME for now only 1d attributes :-P
    for(int i=0;i<cliargs_count(attr_args);i++) {
      cliini_arg *arg = cliargs_nth(attr_args, i);
      
      int dims = 1;
      int size = cliarg_sum(arg);
      
      attrs[i].Set<int>(arg->opt->longflag, dims, &size, cliini_type_to_BaseType(arg->opt->type), arg->vals);
    }
      
      
    //FIXME free cliini allocated memory!
  }
  
  Attribute *Attributes::get(const char *name)
  {
    for(int i=0;i<attrs.size();i++)
      if (!attrs[i].name.compare(name))
        return &attrs[i];
      
    return NULL;
  }
  
  
  void Attributes::write(H5::H5File &f, std::string &name)
  {
    if (!attrs.size())
      return;
    
    for(int i=0;i<attrs.size();i++)
      printf("TODO: save %s under %s\n", attrs[i].name.c_str(), name.c_str());
    
    //for all attributes
    //create path 
    //create hdf5 attr
    //write attr
  }
  
  Dataset::~Dataset()
  {
    attrs.write(f, name);
  }
  
  static void attributes_append_group(Attributes &attrs, H5::Group &g, std::string group_path)
  {
    for(int i=0;i<g.getNumObjs();i++) {
      H5G_obj_t type = g.getObjTypeByIdx(i);
      
      std::string name = appendToPath(appendToPath(group_path, "/"), g.getObjnameByIdx(hsize_t(i)));
      
      if (type == H5G_GROUP)
        attributes_append_group(attrs, g, name);
    }
    
    
    for(int i=0;i<g.getNumAttrs();i++) {
      H5::Attribute h5attr = g.openAttribute(i);
      Attribute attr;
      void *data;
      int size[1];
      BaseType type;
      
      std::string name = appendToPath(appendToPath(group_path, "/"), h5attr.getName());
      
      data = read_attr(g, h5attr.getName(),type,size);
      
      attr.Set<int>(name, 1, size, type, data);
      attrs.append(attr);
    }
  }
  
  /*Attributes::Attributes(H5::H5File &f, std::string &name)
  {
    H5::Group group = f.openGroup(name.c_str());
    
    
  }*/

  
  void Attributes::append(Attribute &attr)
  {
    attrs.push_back(attr);
  }
      
  
  Datastore::Datastore(H5::H5File &f, const std::string parent_group_str, const std::string name, uint width, uint height, uint count, DataType datatype, DataOrg dataorg, DataOrder dataorder)
  : type(datatype), org(dataorg), order(dataorder)
  {
    hsize_t dims[3] = {width,height, 0};
    hsize_t maxdims[3] = {width,height,H5S_UNLIMITED}; 
    std::string dataset_str = parent_group_str;
    dataset_str = appendToPath(dataset_str, name);
    
    //printf("type: %s\n", ClifEnumString(DataType,type));
    
    if (_hdf5_obj_exists(f, dataset_str.c_str())) {
      data = f.openDataSet(dataset_str);
      return;
    }
    
    _rec_make_groups(f, parent_group_str.c_str());
    
    /*printf("make %s\n", parent_group_str.c_str());
    _rec_make_groups(f, parent_group_str.c_str());
    
    H5::Group parent_group = f.openGroup(parent_group_str.c_str());
    H5::Group format_group = f.createGroup(appendToPath(parent_group_str, "format"));
    
    save_string_attr(format_group, "type", ClifEnumString(DataType,type));
    save_string_attr(format_group, "organsiation", ClifEnumString(DataOrg,org));
    save_string_attr(format_group, "order", ClifEnumString(DataOrder,order));*/
    
    //chunking fixed for now
    hsize_t chunk_dims[3] = {width,height,1};
    H5::DSetCreatPropList prop;
    prop.setChunk(3, chunk_dims);
    
    H5::DataSpace space(3, dims, maxdims);
    
    data = f.createDataSet(dataset_str, 
	               H5PredType(type), space, prop);
  }
  
  /*Datastore::Datastore(H5::H5File &f, const std::string dataset_str, DataType datatype, DataOrg dataorg, DataOrder dataorder)
  : type(datatype), org(dataorg), order(dataorder)
  {
    std::string attr_str;*/
    //int res;
    
    /*H5::Group format = f.openGroup(appendToPath(parent_group_str, "format"));
    
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
    order = (DataOrder)res;*/
    
    /*if (!_hdf5_obj_exists(f, dataset_str.c_str()))
      return;

    data = f.openDataSet(dataset_str);
  }*/

  
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
        assert(type != DataType::UINT16);
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

