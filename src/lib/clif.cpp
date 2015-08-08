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
  
  std::string remove_last_part(std::string in, char c)
  {
    size_t pos = in.rfind(c);
    
    if (pos == std::string::npos)
      return std::string("");
    
    return in.substr(0, pos);
  }
  
  std::string get_last_part(std::string in, char c)
  {
    size_t pos = in.rfind(c);
    
    if (pos == std::string::npos)
      return in;
    
    return in.substr(pos+1, in.npos);
  }
  
  std::string Attribute::toString()
  {
    if (type == BaseType::STRING) {
      return std::string((char*)data);
    }
    else
      return std::string("TODO fix toString for type");
  }
  
  void Attribute::write(H5::H5File &f, std::string dataset_name)
  {
    std::string path = name;
    std::replace(path.begin(), path.end(), '.', '/');
    
    
    printf("dataset name %s\n", dataset_name.c_str());
    
    path = appendToPath(dataset_name, path);
    path = remove_last_part(path, '/');
    
    std::string attr_name = get_last_part(name, '.');
    
    printf("attribute group loc: %s attr name %s\n", path.c_str(), attr_name.c_str());
    
    _rec_make_groups(f, path.c_str());

    H5::Group g = f.openGroup(path);
    
    hsize_t dim[1];
    
    dim[0] = size[0];
    
    H5::DataSpace space(1, dim);
    
    H5::Attribute attr = g.createAttribute(attr_name, BaseType_to_PredType(type), space);
       
    attr.write(BaseType_to_PredType(type), data);
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
    
    void *buf = malloc(basetype_size(type)*dims[0]);
    
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
      
      if (arg->opt->type == CLIINI_STRING) {
        assert(size == 1);
        //only single string supported!
        size = strlen(((char**)(arg->vals))[0])+1;
        attrs[i].Set<int>(arg->opt->longflag, dims, &size, cliini_type_to_BaseType(arg->opt->type), ((char**)(arg->vals))[0]);
      }
      else
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
  
  StringTree Attributes::getTree()
  {
    StringTree tree;
    
    for(int i=0;i<attrs.size();i++)
      tree.add(attrs[i].name, &attrs[i], '.');
    
    return tree;
  }
  
  
  void Attributes::write(H5::H5File &f, std::string &name)
  {
    if (!attrs.size())
      return;
    
    for(int i=0;i<attrs.size();i++) {
      
      attrs[i].write(f, name);
      printf("TODO: save %s under %s\n", attrs[i].name.c_str(), name.c_str());
    }
    
    //for all attributes
    //create path 
    //create hdf5 attr
    //write attr
  }
  
  int Attributes::count()
  {
    return attrs.size();
  }
  
  Attribute Attributes::operator[](int pos)
  {
    return attrs[pos];
  }
  
  
  Dataset::Dataset(H5::H5File &f_, std::string name_)
  : f(f_), name(name_)
  {
    if (_hdf5_obj_exists(f, name.c_str())) {
      attrs = Attributes(f, name);
    }
  }
  
  bool Dataset::valid()
  {
    if (attrs.count()) 
      return true;
    return false;
  }

  void Dataset::writeAttributes()
  {
    attrs.write(f, name);
  }
  
  static void attributes_append_group(Attributes &attrs, H5::Group &g, std::string basename, std::string group_path)
  {    
    for(int i=0;i<g.getNumObjs();i++) {
      H5G_obj_t type = g.getObjTypeByIdx(i);
      
      std::string name = appendToPath(group_path, g.getObjnameByIdx(hsize_t(i)));
      
      
      
      if (type == H5G_GROUP) {
        H5::Group sub = g.openGroup(g.getObjnameByIdx(hsize_t(i)));
        attributes_append_group(attrs, sub, basename, name);
      }
    }
    
    
    for(int i=0;i<g.getNumAttrs();i++) {
      H5::Attribute h5attr = g.openAttribute(i);
      Attribute attr;
      void *data;
      int size[1];
      BaseType type;

      std::string name = appendToPath(group_path, h5attr.getName());
      
      data = read_attr(g, h5attr.getName(),type,size);
      
      name = name.substr(basename.length()+1, name.length()-basename.length()-1);
      std::replace(name.begin(), name.end(), '/', '.');
      
      attr.Set<int>(name, 1, size, type, data);
      attrs.append(attr);
    }
  }
  
  Attributes::Attributes(H5::H5File &f, std::string &name)
  {
    H5::Group group = f.openGroup(name.c_str());
    
    attributes_append_group(*this, group, name, name);
  }

  
  void Attributes::append(Attribute &attr)
  {
    attrs.push_back(attr);
  }
  
  Datastore::Datastore(Dataset *dataset, std::string path, int w, int h, int count)
  {
    dataset->readEnum("format.type",         type);
    dataset->readEnum("format.organisation", org);
    dataset->readEnum("format.order",        order);
    
    if (int(type) == -1 || int(org) == -1 || int(order) == -1) {
      printf("ERROR: unsupported dataset format!\n");
      return;
    }
    
    hsize_t dims[3] = {w,h, 0};
    hsize_t maxdims[3] = {w,h,H5S_UNLIMITED}; 
    std::string dataset_str = dataset->name;
    dataset_str = appendToPath(dataset_str, path);
    
    if (_hdf5_obj_exists(dataset->f, dataset_str.c_str())) {
      data = dataset->f.openDataSet(dataset_str);
      return;
    }
    
    _rec_make_groups(dataset->f, path.c_str());
    
    //chunking fixed for now
    hsize_t chunk_dims[3] = {w,h,1};
    H5::DSetCreatPropList prop;
    prop.setChunk(3, chunk_dims);
    
    H5::DataSpace space(3, dims, maxdims);
    
    data = dataset->f.createDataSet(dataset_str, 
	               H5PredType(type), space, prop);
  }
  
  Datastore::Datastore(Dataset *dataset, std::string path)
  {
    std::string dataset_str = appendToPath(dataset->name, path);
    
    if (!_hdf5_obj_exists(dataset->f, dataset_str.c_str())) 
      return;
    
    dataset->readEnum("format.type",         type);
    dataset->readEnum("format.organisation", org);
    dataset->readEnum("format.order",        order);

    if (int(type) == -1 || int(org) == -1 || int(order) == -1) {
      printf("ERROR: unsupported dataset format!\n");
      return;
    }
    
    data = dataset->f.openDataSet(dataset_str);
    
    //printf("Datastore open %s: %s %s %s\n", dataset_str.c_str(), ClifEnumString(DataType,type),ClifEnumString(DataOrg,org),ClifEnumString(DataOrder,order));
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
  
  bool Datastore::valid()
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
  
  StringTree::StringTree(std::string name, void *data)
  {
    val.first = name;
    val.second = data;
  }
  
  void StringTree::print(int depth)
  {
    for(int i=0;i<depth;i++)
      printf("   ");
    printf("%s (%d)\n", val.first.c_str(),childs.size());
    for(int i=0;i<childs.size();i++)
      childs[i].print(depth+1);
  }
  
  void StringTree::add(std::string str, void *data, char delim)
  {
    int found = str.find(delim);
    std::string name = str.substr(0, found);
    
    std::cout << "add: " << str << ":" << name << std::endl;
    
    for(int i=0;i<childs.size();i++)
      if (!name.compare(childs[i].val.first)) {
        if (found < str.length()-1) { //don't point to last letter or beyond (npos)
          printf("found!\n");
          childs[i].add(str.substr(found+1), data, delim);
          return;
        }
        else {
          printf("FIXME StringTree: handle existing elements in add!\n");
          return;
        }
      } 
    
    if (found < str.length()-1) {
      printf("not found add %s\n", name.c_str());
      childs.push_back(StringTree(name,NULL));
      childs.back().add(str.substr(found+1), data, delim);
    }
    else {
      std::cout << "actual add: " << str << ":" << name << std::endl;
      childs.push_back(StringTree(name,data));
    }
    
  }
    
  std::pair<std::string, void*> *StringTree::search(std::string str, char delim)
  {
    int found = str.find(delim);
    std::string name = str.substr(0, found);
    
    std::cout << "search: " << str << ":" << name << std::endl;
    
    for(int i=0;i<childs.size();i++)
      if (!name.compare(childs[i].val.first)) {
        if (found < str.length()-1) { //don't point to last letter or beyond (npos)
          return childs[i].search(str.substr(found+1), delim);
        }
        else
          return &val;
      } 
    
    std::cout << "not found: " << str << ":" << name << std::endl;
    return NULL;
  }
  
  
  StringTree *StringTree::operator[](int idx)
  {
    return &childs[idx];
  }
  
  int StringTree::childCount()
  {
    return childs.size();
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

bool ClifDataset::valid()
{
  if (clif::Dataset::valid() && clif::Datastore::valid())
    return true;
  
  return false;
}

