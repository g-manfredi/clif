#include "clif.hpp"

#include <string.h>
#include <string>
#include <assert.h>

#include <exception>
#include <fstream>
#include <fnmatch.h>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "cliini.h"


namespace clif {
  
  
  /*std::string path_element(boost::filesystem::path path, int idx)
  {    
    auto it = path.begin();
    for(int i=0;i<idx;i++,++it) {
      if (it == path.end())
        throw std::invalid_argument("requested path element too large");
      if (!(*it).generic_string().compare("/"))
        ++it;
    }
    
    return (*it).generic_string();
  }*/
  
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
        if (i == 10)
          break;
        callByBaseType<insertionDispatcher>(a.type, &out, a.data, i);
        out << ",";
      }
      callByBaseType<insertionDispatcher>(a.type, &out, a.data, i);
      if (i==10)
        out << ", ... ]";
      else
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
    
    if (!h5_obj_exists(f, grouppath))
      h5_create_path_groups(f, grouppath);
    
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

  /*int parse_string_enum(const char *str, const char **enumstrs)
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
  }*/
  
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
  
  /*Dataset::Dataset(H5::H5File &f_, std::string path)
  : f(f_), _path(path)
  {
    if (h5_obj_exists(f, _path.c_str())) {
      static_cast<Attributes&>(*this) = Attributes(f, _path);
      
      //FIXME specificy which one!
      load_intrinsics();
    }
  }*/
  
  
  
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
  
  void Attributes::open(H5::H5File &f, std::string &name)
  {
    attrs.resize(0);
    
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


clif::Dataset* ClifFile::openDataset(std::string name)
{
  clif::Dataset *set = new clif::Dataset();
  set->open(f, name);
  return set;
}

clif::Dataset* ClifFile::createDataset(std::string name)
{
  clif::Dataset *set = new clif::Dataset();
  set->create(f, name);
  return set;
}

clif::Dataset* ClifFile::openDataset(int idx)
{
  return openDataset(datasets[idx]);
}

bool ClifFile::valid()
{
  return f.getId() != H5I_INVALID_HID;
}

}
