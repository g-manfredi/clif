#include "clif.hpp"

#include <string.h>
#include <string>
#include <assert.h>

#include <exception>
#include <fstream>
#include <fnmatch.h>

#include <opencv2/core/core.hpp>

#include "cliini.h"

#include "clif3dsubset.hpp"

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

    if (!clif::h5_obj_exists(f, buf)) {
      f.createGroup(buf);
    }

    next_ptr = strchr(next_ptr+1, '/');
  }
  
  buf[last_pos] = '/';
  if (!clif::h5_obj_exists(f, buf))
    f.createGroup(buf);
  
  free(buf);
}

namespace clif {
  
  std::string path_element(boost::filesystem::path path, int idx)
  {    
    auto it = path.begin();
    for(int i=0;i<idx;i++,++it) {
      if (it == path.end())
        throw std::invalid_argument("requested path element too large");
      if (!(*it).generic_string().compare("/"))
        ++it;
    }
    
    return (*it).generic_string();
  }
  
  void h5_create_path_groups(H5::H5File &f, boost::filesystem::path path) 
  {
    boost::filesystem::path part;
    
    for(auto it = path.begin(); it != path.end(); ++it) {
      part /= *it;
      if (!clif::h5_obj_exists(f, part)) {
        f.createGroup(part.c_str());
      }
    }
  }
  
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
      case H5T_FLOAT: 
        if (H5Tget_size(type) == 4)
          return BaseType::FLOAT;
        else if (H5Tget_size(type) == 8)
          return BaseType::DOUBLE;
        break;
    }
    
    printf("ERROR: unknown argument type!\n");
    abort();
  }
  
  H5::PredType BaseType_to_PredType(BaseType type)
  {    
    switch (type) {
      case BaseType::STRING : return H5::PredType::C_S1;
      case BaseType::INT : return H5::PredType::NATIVE_INT;
      case BaseType::FLOAT: return H5::PredType::NATIVE_FLOAT;
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
  
  bool h5_obj_exists(H5::H5File &f, const char * const path)
  {
    H5E_auto2_t  oldfunc;
    void *old_client_data;
    
    H5Eget_auto(H5E_DEFAULT, &oldfunc, &old_client_data);
    H5Eset_auto(H5E_DEFAULT, NULL, NULL);
    
    int status = H5Gget_objinfo(f.getId(), path, 0, NULL);
    
    H5Eset_auto(H5E_DEFAULT, oldfunc, old_client_data);
    
    if (status == 0)
      return true;
    return false;
  }
  
  bool h5_obj_exists(H5::H5File &f, const std::string path)
  {
    return h5_obj_exists(f,path.c_str());
  }
  
  bool h5_obj_exists(H5::H5File &f, const boost::filesystem::path path)
  {
    return h5_obj_exists(f, path.c_str());
  }
  
  static void datasetlist_append_group(std::vector<std::string> &list, H5::Group &g,  std::string group_path)
  {    
    for(int i=0;i<g.getNumObjs();i++) {
      H5G_obj_t type = g.getObjTypeByIdx(i);
      
      std::string name = appendToPath(group_path, g.getObjnameByIdx(hsize_t(i)));
      
      if (type == H5G_GROUP) {
        H5::Group sub = g.openGroup(g.getObjnameByIdx(hsize_t(i)));
        datasetlist_append_group(list, sub, name);
      }
      else if (type == H5G_DATASET)
        list.push_back(name);
    }
  }
  
  std::vector<std::string> listH5Datasets(H5::H5File &f, std::string parent)
  {
    std::vector<std::string> list;
    H5::Group group = f.openGroup(parent.c_str());
    
    datasetlist_append_group(list, group, parent);
    
    return list;
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
      case BaseType::FLOAT : return sizeof(float);
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
  
  std::string get_first_part(std::string in, char c)
  {
    size_t pos = in.find(c);
    
    if (pos == std::string::npos)
      return in;
    
    return in.substr(0, pos);
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
  
  template<typename T> void printthis(std::ostream *stream, void *val, int idx)
    {
      *stream << ((T*)val)[idx];
    }
  
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
    
    std::cout << "write " << attr_path << std::endl;
    
    hsize_t dim[1];
    dim[0] = size[0];
    H5::DataSpace space(1, dim);
    H5::Attribute attr;
    H5::Group g;
    
    if (!h5_obj_exists(f, grouppath))
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
  
  
  void Attributes::extrinsicGroups(std::vector<std::string> &groups)
  {
    listSubGroups("calibration/extrinsics", groups);
  }
  
  void Attributes::write(H5::H5File &f, std::string &name)
  {
    for(int i=0;i<attrs.size();i++)
      attrs[i].write(f, name);
    
    f.flush(H5F_SCOPE_GLOBAL);
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
  
  //TODO this is ugly!
  void Attributes::listSubGroups(std::string parent, std::vector<std::string> &matches)
  {
    std::unordered_map<std::string,int> match_map;
    std::string match;
    std::replace(parent.begin(), parent.end(), '.', '/');
    parent = appendToPath(parent, "*");
    
    for(int i=0;i<attrs.size();i++) {
      if (!fnmatch(parent.c_str(), attrs[i].name.c_str(), 0)) {
        match = attrs[i].name.substr(parent.length()-1, attrs[i].name.length());
        match = get_first_part(match, '/');
        match_map[match] = 1;
      }
    }
    
    for(auto it = match_map.begin(); it != match_map.end(); ++it )
      matches.push_back(it->first);
  }
  
  Attribute Attributes::operator[](int pos)
  {
    return attrs[pos];
  }
  
  Dataset::Dataset(H5::H5File &f_, std::string path)
  : f(f_), _path(path)
  {
    if (h5_obj_exists(f, _path.c_str())) {
      static_cast<Attributes&>(*this) = Attributes(f, _path);
    }
  }
  
  boost::filesystem::path Dataset::path()
  {
    return boost::filesystem::path(_path);
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
    Attribute *at = getAttribute(attr.name);
    if (!at)
      attrs.push_back(attr);
    else 
      *at = attr;
  }
  
  void Attributes::append(Attribute *attr)
  {
    append(*attr);
  }
  
  //TODO document:
  //overwrites existing attributes of same name
  void Attributes::append(Attributes &other)
  {
    for(int i=0;i<other.attrs.size();i++)
      append(other.attrs[i]);
  }
  
  void Attributes::append(Attributes *other)
  {
    append(*other);
  }
  
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
  
  int combinedTypeElementCount(DataType type, DataOrg org, DataOrder order)
  {
    switch (org) {
      case DataOrg::PLANAR : return 1;
      case DataOrg::INTERLEAVED :
        switch (order) {
          case DataOrder::RGB : return 3;
          default:
            abort();
        }
      case DataOrg::BAYER_2x2 : return 1;
      default :
        abort();
    }
  }
  
  int combinedTypePlaneCount(DataType type, DataOrg org, DataOrder order)
  {
    switch (org) {
      case DataOrg::PLANAR :
        switch (order) {
          case DataOrder::RGB   : return 3;
          default:
            abort();
        }
      case DataOrg::INTERLEAVED : return 1;
      case DataOrg::BAYER_2x2   : return 1;
    }
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
    
    _rec_make_groups(_dataset->f, remove_last_part(dataset_str, '/').c_str());
    
    //chunking fixed for now
    hsize_t chunk_dims[3] = {comb_w,comb_h,1};
    H5::DSetCreatPropList prop;
    prop.setChunk(3, chunk_dims);
    
    H5::DataSpace space(3, dims, maxdims);
    
    _data = _dataset->f.createDataSet(dataset_str, 
	               H5PredType(_type), space, prop);
  }
  
  void * Datastore::cache_get(uint64_t key)
  {
    auto it_find = image_cache.find(key);
    
    if (it_find == image_cache.end())
      return NULL;
    else
      return it_find->second;
  }
  
  void Datastore::cache_set(uint64_t key, void *data)
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
  
  
  /*void readOpenCvMat(uint idx, Mat &m);
        assert(w == img.size().width);
      assert(h = img.size().height);
      assert(depth = img.depth());
      printf("store idx %d: %s\n", i, in_names[i]);
      lfdata.writeRawImage(i, img.data);*/
  
  /*int Datastore::imgMemSize()
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
  }*/
  
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
    
    if (!h5_obj_exists(f, "/clif"))
      return list;
    
    H5::Group g = f.openGroup("/clif");
    
    hsize_t count = g.getNumObjs();
    list.resize(count);
    
    for(int i=0;i<count;i++)
      list[i] = g.getObjnameByIdx(i);
    
    return list;
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

namespace clif_cv {
  
  cv::Size imgSize(Datastore *store)
  {
    H5::DataSpace space = store->H5DataSet().getSpace();
    hsize_t dims[3];
    hsize_t maxdims[3];
    
    space.getSimpleExtentDims(dims, maxdims);
    
    dims[0] /= combinedTypeElementCount(store->type(), store->org(), store->order());
    dims[1] /= combinedTypePlaneCount(store->type(), store->org(), store->order());
    
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
  
  
  /*void readEPI(ClifDataset *lf, cv::Mat &m, int line, double depth, int flags)
  {

    //TODO maybe easier to read on image and just use that type...
    //or create a extra function to calc type in clif_cv
    
    cv::Mat tmp;
    readCvMat(lf, 0, tmp, flags | CLIF_DEMOSAIC);

    m = cv::Mat::zeros(cv::Size(imgSize(lf).width, lf->imgCount()), tmp.type());
    //tmp.row(line).copyTo(m.row(0));
    
    for(int i=0;i<lf->imgCount();i++)
    {      
      //FIXME rounding?
      double d = depth*(i-lf->imgCount()/2);
      readCvMat(lf, i, tmp, flags | CLIF_DEMOSAIC);
      if (d <= 0) {
        d = -d;
        int w = tmp.size().width - d;
        tmp.row(line).colRange(d, d+w).copyTo(m.row(i).colRange(0, w));
      }
      else {
        int w = tmp.size().width - d;
        tmp.row(line).colRange(0, w).copyTo(m.row(i).colRange(d, d+w));
      }
    }
  }*/
    
  void readCvMat(Datastore *store, uint idx, cv::Mat &outm, int flags)
  {   
    uint64_t key = idx*CLIF_PROCESS_FLAGS_MAX | flags;
    
    cv::Mat *m = static_cast<cv::Mat*>(store->cache_get(key));
    if (m) {
      outm = *m;//->clone();
      return;
    }
    
    if (store->org() == DataOrg::BAYER_2x2) {
      //FIXME bayer only for now!
      m = new cv::Mat(imgSize(store), DataType2CvDepth(store->type()));
      
      store->readRawImage(idx, m->size().width, m->size().height, m->data);
      
      if (store->org() == DataOrg::BAYER_2x2 && flags & CLIF_DEMOSAIC) {
        switch (store->order()) {
          case DataOrder::RGGB :
            cvtColor(*m, *m, CV_BayerBG2BGR);
            break;
          case DataOrder::BGGR :
            cvtColor(*m, *m, CV_BayerRG2BGR);
            break;
          case DataOrder::GBRG :
            cvtColor(*m, *m, CV_BayerGR2BGR);
            break;
          case DataOrder::GRBG :
            cvtColor(*m, *m, CV_BayerGB2BGR);
            break;
        }
      }
    }
    else if (store->org() == DataOrg::INTERLEAVED && store->order() == DataOrder::RGB) {
      m = new cv::Mat(imgSize(store), CV_MAKETYPE(DataType2CvDepth(store->type()), 3));
      store->readRawImage(idx, m->size().width, m->size().height, m->data);
    }
    
    if (m->depth() == CV_16U && flags & CLIF_CVT_8U) {
      *m *= 1.0/256.0;
      m->convertTo(*m, CV_8U);
    }
    
    if (m->channels() != 1 && flags & CLIF_CVT_GRAY) {
      cvtColor(*m, *m, CV_BGR2GRAY);
    }
    
    store->cache_set(key, m);
    
    outm = *m;//->clone();
  }
  
  
  void writeCalibPoints(Dataset *set, std::string calib_set_name, std::vector<std::vector<cv::Point2f>> &imgpoints, std::vector<std::vector<cv::Point2f>> &worldpoints)
  {
    int pointcount = 0;
    
    for(int i=0;i<imgpoints.size();i++)
      for(int j=0;j<imgpoints[i].size();j++)
        pointcount++;
      
    float *pointbuf = new float[4*pointcount];
    float *curpoint = pointbuf;
    int *sizebuf = new int[imgpoints.size()];
    
    
    for(int i=0;i<imgpoints.size();i++)
      for(int j=0;j<imgpoints[i].size();j++) {
        curpoint[0] = imgpoints[i][j].x;
        curpoint[1] = imgpoints[i][j].y;
        curpoint[2] = worldpoints[i][j].x;
        curpoint[3] = worldpoints[i][j].y;
        sizebuf[i] = imgpoints[i].size();
        curpoint += 4;
      }
      
    set->setAttribute(set->path() / "calibration/images/sets" / calib_set_name / "pointdata", pointbuf, 4*pointcount);
    set->setAttribute(set->path() / "calibration/images/sets" / calib_set_name / "pointcounts", sizebuf, imgpoints.size());
  }
}


void ClifFile::open(std::string &filename, unsigned int flags)
{
  f.openFile(filename, flags);
  
  datasets.resize(0);
  
  if (f.getId() == H5I_INVALID_HID)
    return;
  
  if (!clif::h5_obj_exists(f, "/clif"))
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
  
  if (!clif::h5_obj_exists(f, "/clif"))
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


ClifDataset* ClifFile::openDataset(std::string name)
{
  ClifDataset *set = new ClifDataset();
  set->open(f, name);
  return set;
}

ClifDataset* ClifFile::createDataset(std::string name)
{
  ClifDataset *set = new ClifDataset();
  set->create(f, name);
  return set;
}

ClifDataset* ClifFile::openDataset(int idx)
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
  
  clif::Datastore::create("data", this);
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

Clif3DSubset *ClifDataset::get3DSubset(int idx)
{
  std::vector<std::string> groups;
  extrinsicGroups(groups);
  return new Clif3DSubset(this, groups[idx]);
}

//return pointer to the calib image datastore - may be manipulated
clif::Datastore *ClifDataset::getCalibStore()
{
  boost::filesystem::path dataset_path;
  dataset_path = path() / "calibration/images/data";
  
  std::cout << dataset_path << clif::h5_obj_exists(f, dataset_path) << calib_images << std::endl;
  
  if (!calib_images && clif::h5_obj_exists(f, dataset_path)) {
    calib_images = new clif::Datastore();
    calib_images->open(this, "calibration/images/data");
  }
  
  return calib_images;
}

clif::Datastore *ClifDataset::createCalibStore()
{
  printf("create calib store\n");
  
  if (calib_images)
    return calib_images;
  
  getCalibStore();
  
  if (calib_images)
    return calib_images;
    
  calib_images = new clif::Datastore();
  calib_images->create("calibration/images/data", this);
  return calib_images;
}
