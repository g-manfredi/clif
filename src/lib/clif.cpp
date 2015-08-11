#include "clif.hpp"

#include <string.h>
#include <string>
#include <assert.h>

#include <exception>
#include <fstream>

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

static bool _hdf5_obj_exists(H5::H5File &f, const std::string group_str)
{
  return _hdf5_obj_exists(f,group_str.c_str());
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

    if (!_hdf5_obj_exists(f, buf)) {
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
  
  template<typename T> void ostreamInsertArrrayIdx(std::ostream *stream, void *val, int idx)
  {
    *stream << ((T*)val)[idx];
  }
  
  template<typename T> class insertionDispatcher {
  public:
    void operator()(std::ostream *stream, void *val, int idx)
    {
      *stream << ((T*)val)[idx];
    }
  };
  
  std::ostream& operator<<(std::ostream& out, const Attribute& a)
  {
    if (a.type == BaseType::STRING) {
      out << (char*)a.data;
      return out;
    }
    else {
      if (a.size[0] == 1) {
        callByBaseType<insertionDispatcher>(a.type, &out, a.data, 0);
        return out;
      }
      out << "[";
      //FIXME dims!
      int i;
      for(i=0;i<a.size[0]-1;i++) {
        callByBaseType<insertionDispatcher>(a.type, &out, a.data, i);
        out << ",";
      }
      callByBaseType<insertionDispatcher>(a.type, &out, a.data, i);
      out << "]";
      return out;
    }
  }
  
  std::string Attribute::toString()
  {
    if (type == BaseType::STRING) {
      return std::string((char*)data);
    }
    else {
      std::ostringstream stream;
      if (size[0] == 1) {
        callByBaseType<insertionDispatcher>(type, &stream, data, 0);
        return stream.str();
      }
      stream << "[";
      //FIXME dims!
      int i;
      for(i=0;i<size[0]-1;i++) {
        callByBaseType<insertionDispatcher>(type, &stream, data, i);
        stream << ",";
      }
      callByBaseType<insertionDispatcher>(type, &stream, data, i);
      stream << "]";
      return stream.str();
    }
  }
  
  void Attribute::write(H5::H5File &f, std::string dataset_name)
  {  
    //FIXME we should have a path?
    std::string attr_path = name;
    std::replace(attr_path.begin(), attr_path.end(), '.', '/');
    std::string fullpath = appendToPath(dataset_name, attr_path);
    std::string grouppath = remove_last_part(fullpath, '/');
    std::string attr_name = get_last_part(fullpath, '/');
    
    hsize_t dim[1];
    dim[0] = size[0];
    H5::DataSpace space(1, dim);
    H5::Attribute attr;
    H5::Group g;
    bool attr_exists;
    
    if (!attr_exists)
      _rec_make_groups(f, grouppath.c_str());
    
    g = f.openGroup(grouppath);
    
    if (g.attrExists(attr_name))
      g.removeAttr(attr_name);
      
    attr = g.createAttribute(attr_name, BaseType_to_PredType(type), space);
       
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
    
    //FIXME 1D only atm
    *size = dims[0];
    
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
    cliini_fit_typeopts(attr_args, attr_types);
    
    attrs.resize(cliargs_count(attr_args));
    
    //FIXME for now only 1d attributes :-P
    for(int i=0;i<cliargs_count(attr_args);i++) {
      cliini_arg *arg = cliargs_nth(attr_args, i);
      
      int dims = 1;
      int size = cliarg_sum(arg);
      
      attrs[i].setName(arg->opt->longflag);
        
      //FIXME nice enum type conversion
      switch (arg->opt->type) {
        case CLIINI_ENUM:
        case CLIINI_STRING :
          assert(size == 1);
          attrs[i].set(((char**)(arg->vals))[0], strlen(((char**)(arg->vals))[0])+1);
          break;
        case CLIINI_INT :
          attrs[i].set((int*)(arg->vals), size);
          break;
        case CLIINI_DOUBLE :
          attrs[i].set((double*)(arg->vals), size);
          break;
        default :
          abort();
      }
      
      /*if (arg->opt->type == CLIINI_STRING) {
        assert(size == 1);
        //only single string supported!
        size = strlen(((char**)(arg->vals))[0])+1;
        attrs[i].Set<int>(arg->opt->longflag, dims, &size, cliini_type_to_BaseType(arg->opt->type), ((char**)(arg->vals))[0]);
      }
      if else (arg->opt->type == CLIINI_INT) {
        attrs[i].set( size);
        //attrs[i].Set<int>(arg->opt->longflag, dims, &size, cliini_type_to_BaseType(arg->opt->type), arg->vals);
      }*/
    }
      
      
    //FIXME free cliini allocated memory!
  }
  
  
  void Attributes::write(H5::H5File &f, std::string &name)
  {
    for(int i=0;i<attrs.size();i++)
      attrs[i].write(f, name);
  }
  
  void Attributes::writeIni(std::string &filename)
  {
    std::ofstream f;
    f.open (filename);
    
    std::string currsection;
    std::string nextsection;
    
    for(int i=0;i<attrs.size();i++) {
      nextsection = remove_last_part(attrs[i].name, '/');
      if (nextsection.compare(currsection)) {
        currsection = nextsection;
        std::replace(nextsection.begin(), nextsection.end(), '/', '.');
        f << std::endl << std::endl << "[" << nextsection << "]" << std::endl << std::endl;
      }
      f << get_last_part(attrs[i].name,'/') << " = " << attrs[i] << std::endl;
    }
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
      static_cast<Attributes&>(*this) = Attributes(f, name);
    }
  }
  
  bool Dataset::valid()
  {
    if (count()) 
      return true;
    return false;
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
  
  //TODO document:
  //overwrites existing attributes of same name
  void Attributes::append(Attributes &other)
  {
    Attribute *at;
    for(int i=0;i<other.attrs.size();i++) {
      at = getAttribute(other.attrs[i].name);
      if (!at)
        attrs.push_back(other.attrs[i]);
      else
        *at = other.attrs[i];
    }
  }
  
  //FIXME wether dataset already exists and overwrite?
  void Datastore::create(std::string path_, Dataset *dataset, hsize_t w, hsize_t h)
  {
    type = DataType(-1); 
    org = DataOrg(-1);
    order = DataOrder(-1); 
      
    data = H5::DataSet();
    path = path_;
    
    if (dataset && w && h)
      init_from_dataset(dataset,w,h);
  }
  
  
  void Datastore::init_from_dataset(Dataset *dataset, hsize_t w, hsize_t h)
  {
    dataset->readEnum("format/type",         type);
    dataset->readEnum("format/organisation", org);
    dataset->readEnum("format/order",        order);
    
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
    
    _rec_make_groups(dataset->f, remove_last_part(dataset_str, '/').c_str());
    
    //chunking fixed for now
    hsize_t chunk_dims[3] = {w,h,1};
    H5::DSetCreatPropList prop;
    prop.setChunk(3, chunk_dims);
    
    H5::DataSpace space(3, dims, maxdims);
    
    data = dataset->f.createDataSet(dataset_str, 
	               H5PredType(type), space, prop);
  }
  
  void Datastore::open(Dataset *dataset, std::string path_)
  {
    //only fills in internal data
    create(path_);
        
    std::string dataset_str = appendToPath(dataset->name, path_);
        
    if (!_hdf5_obj_exists(dataset->f, dataset_str.c_str())) {
      printf("error: could not find requrested datset: %s\n", dataset_str.c_str());
      return;
    }
    
    dataset->readEnum("format/type",         type);
    dataset->readEnum("format/organisation", org);
    dataset->readEnum("format/order",        order);

    if (int(type) == -1 || int(org) == -1 || int(order) == -1) {
      printf("ERROR: unsupported dataset format!\n");
      return;
    }
    
    data = dataset->f.openDataSet(dataset_str);
    
    //printf("Datastore open %s: %s %s %s\n", dataset_str.c_str(), ClifEnumString(DataType,type),ClifEnumString(DataOrg,org),ClifEnumString(DataOrder,order));
  }
  
  void Datastore::writeRawImage(uint idx, hsize_t w, hsize_t h, void *imgdata)
  {
    assert(valid());
    
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
  
  void Datastore::appendRawImage(hsize_t w, hsize_t h, void *imgdata)
  {
    int idx = 0;
    if (valid()) {
      H5::DataSpace space = data.getSpace();
      hsize_t dims[3];
      hsize_t maxdims[3];
      
      space.getSimpleExtentDims(dims, maxdims);
      
      idx = dims[2];
    }
    
    writeRawImage(idx, w, h, imgdata);
  }
  
  void Datastore::readRawImage(uint idx, hsize_t w, hsize_t h, void *imgdata)
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
    
    readRawImage(idx, m.size().width, m.size().height, m.data);
    
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


void ClifFile::open(std::string &filename, unsigned int flags)
{
  f.openFile(filename, flags);
  
  datasets.resize(0);
  
  if (f.getId() == H5I_INVALID_HID)
    return;
  
  if (!_hdf5_obj_exists(f, "/clif"))
      return;
    
  H5::Group g = f.openGroup("/clif");
  
  hsize_t count = g.getNumObjs();
  datasets.resize(count);
  
  for(int i=0;i<count;i++)
    datasets[i] = g.getObjnameByIdx(i);
}

void ClifFile::create(std::string &filename)
{
  f = H5::H5File(filename, H5F_ACC_TRUNC);
  
  datasets.resize(0);
  
  if (f.getId() == H5I_INVALID_HID)
    return;
  
  if (!_hdf5_obj_exists(f, "/clif"))
      return;
    
  H5::Group g = f.openGroup("/clif");
  
  hsize_t count = g.getNumObjs();
  datasets.resize(count);
  
  for(int i=0;i<count;i++)
    datasets[i] = g.getObjnameByIdx(i);
}

ClifFile::ClifFile(std::string &filename, unsigned int flags)
{
  open(filename, flags);
}

int ClifFile::datasetCount()
{
  return datasets.size();
}


ClifDataset ClifFile::openDataset(std::string name)
{
  ClifDataset set;
  set.open(f, name);
  return set;
}

ClifDataset ClifFile::createDataset(std::string name)
{
  ClifDataset set;
  set.create(f, name);
  return set;
}

ClifDataset ClifFile::openDataset(int idx)
{
  return openDataset(datasets[idx]);
}

bool ClifFile::valid()
{
  return f.getId() != H5I_INVALID_HID;
}

void ClifDataset::create(H5::H5File &f, std::string set_name)
{
  std::string fullpath("/clif/");
  fullpath = fullpath.append(set_name);
  
  //TODO create only
  static_cast<clif::Dataset&>(*this) = clif::Dataset(f, fullpath);
  
  clif::Datastore::create("data");
}

void ClifDataset::open(H5::H5File &f, std::string name)
{
  std::string fullpath("/clif/");
  fullpath = fullpath.append(name);
    
  static_cast<clif::Dataset&>(*this) = clif::Dataset(f, fullpath);
  if (!clif::Dataset::valid()) {
    printf("could not open dataset %s\n", fullpath.c_str());
    return;
  }
  
  clif::Datastore::open(this, "data");
}


void ClifDataset::writeRawImage(uint idx, hsize_t w, hsize_t h, void *data)
{
  if (!Datastore::valid())
    init_from_dataset(this, w, h);
  
  Datastore::writeRawImage(idx, w, h, data);
    
}
void ClifDataset::appendRawImage(hsize_t w, hsize_t h, void *data)
{
  if (!Datastore::valid())
    init_from_dataset(this, w, h);
  
  Datastore::appendRawImage(w, h, data);
}
