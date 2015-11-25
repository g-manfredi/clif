#include "attribute.hpp"

#include <fnmatch.h>
#include <sstream>
#include <fstream>

#include "cliini.h"

#include "hdf5.hpp"
#include "types.hpp"

#include <opencv2/core/core.hpp>

namespace clif {
  
using boost::filesystem::path;

typedef unsigned int uint;
  
static void read_attr(Attribute *attr, H5::Group g, std::string basename, std::string group_path, std::string name, BaseType &type)
{
  int total = 1;
  H5::Attribute h5attr = g.openAttribute(name.c_str());
    
  type =  toBaseType(H5Aget_type(h5attr.getId()));
  
  H5::DataSpace space = h5attr.getSpace();
  int dimcount = space.getSimpleExtentNdims();

  hsize_t *dims = new hsize_t[dimcount];
  hsize_t *maxdims = new hsize_t[dimcount];
  
  space.getSimpleExtentDims(dims, maxdims);
  for(int i=0;i<dimcount;i++)
    total *= dims[i];
  
  group_path = group_path.substr(basename.length()+1, group_path.length()-basename.length()-1);
  name = group_path + '/' + name;
  
  //legacy attribute reading
  if (dimcount == 1) {
    void *buf = malloc(baseType_size(type)*total);
    
    h5attr.read(toH5NativeDataType(type), buf);
    
    attr->set<hsize_t>(name, dimcount, dims, type, buf);
  }
  else {
    Mat m;
    Mat_H5AttrRead(m, h5attr);
    attr->setName(name);
    attr->set(m);
  }

  delete[] dims;
  delete[] maxdims;
}

void Attribute::setLink(const boost::filesystem::path &l)
{
  _link = l.generic_string();
  
  data = NULL;
  _m = Mat();
  dims = 0;
  size.resize(0);
}

const std::string& Attribute::link() const
{
  return _link;
}

//FIXME make faster!
static Attribute *_search_attr(std::vector<Attribute> &attrs, std::string name)
{
  for (uint i=0;i<attrs.size();i++)
    if (!attrs[i].name.compare(name))
      return &attrs[i];
    
    return NULL;
}

path Attributes::resolve(path name)
{
  Attribute *a = _search_attr(attrs, name.generic_string());
  
  if (a && a->link().size() == 0)
    return name;
  
  path partial;
  for(auto it=name.begin();it!=name.end();++it) {
    partial /= *it;
    a = _search_attr(attrs, partial.generic_string());
    if (a) {
      if (a->link().size()) {
        path rest(a->link());
        for(++it;it!=name.end();++it)
          rest /= *it;
        return resolve(rest);
      }
      else //name is no valid path!
        return name;
    }
  }
  
  return name;
}

Attribute *Attributes::get(boost::filesystem::path name)
{    
  name = resolve(name);
  
  return _search_attr(attrs, name.generic_string());
}
  
void Attributes::open(const char *inifile, cliini_args *types) 
{
  cliini_optgroup group = {
    NULL,
    NULL,
    0,
    0,
    CLIINI_ALLOW_UNKNOWN_OPT
  };
  
  if (!types)
    types = default_types();
  
  cliini_args *attr_args = cliini_parsefile(inifile, &group);
  if (attr_args && types)
	  cliini_fit_typeopts(attr_args, types);
  
  attrs.resize(0);
  
  
  if (attr_args) {
    attrs.resize(cliargs_count(attr_args));
    
    //FIXME for now only 1d attributes :-P
    for (int i = 0; i < cliargs_count(attr_args); i++) {
      cliini_arg *arg = cliargs_nth(attr_args, i);
      
      //int dims = 1;
      int size = cliarg_sum(arg);
      
      //FIXME check link/name clash!
      attrs[i].setName(arg->opt->longflag);
      
      if (!strcmp(arg->opt->longflag, "source")) {
        assert(arg->opt->type == CLIINI_STRING);
        attrs[i].setLink(((char**)(arg->vals))[0]);
      }
      else 
        switch (arg->opt->type) {
          case CLIINI_ENUM:
          case CLIINI_STRING:
            assert(size == 1);
            attrs[i].set(((char**)(arg->vals))[0], strlen(((char**)(arg->vals))[0]) + 1);
            break;
          case CLIINI_INT:
            attrs[i].set((int*)(arg->vals), size);
            break;
          case CLIINI_DOUBLE:
            attrs[i].set((double*)(arg->vals), size);
            break;
          default:
            abort();
        }
    }
  }
  
  //FIXME free cliini allocated memory!
}
  
void Attributes::open(const char *inifile, const char *typefile) 
{  
  cliini_optgroup group = {
    NULL,
    NULL,
    0,
    0,
    CLIINI_ALLOW_UNKNOWN_OPT
  };
  
  cliini_args *types = cliini_parsefile(typefile, &group);
  
  open(inifile, types);
}

void Attributes::reset()
{
  attrs.resize(0);
}


void Attributes::write(H5::H5File f, std::string &name)
{
  for(uint i=0;i<attrs.size();i++)
    attrs[i].write(f, name);
  
  f.flush(H5F_SCOPE_GLOBAL);
}

void Attributes::writeIni(std::string &filename)
{
  std::ofstream f;
  f.open (filename);
  
  std::string currsection;
  std::string nextsection;
  
  for(uint i=0;i<attrs.size();i++) {
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
  
  for(uint i=0;i<attrs.size();i++) {
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
  for(uint i=0;i<g.getNumObjs();i++) {
    
    char g_name[1024];
    g.getObjnameByIdx(hsize_t(i), g_name, 1024);
    std::string name = appendToPath(group_path, g_name);
    
    H5G_stat_t stat;
    g.getObjinfo(g_name, false, stat);
    printf("%s type: %d\n", g_name, stat.type);
    
    if (stat.type == H5G_GROUP) {
      H5::Group sub = g.openGroup(g_name);
      attributes_append_group(attrs, sub, basename, name);
    }
    else if (stat.type == H5G_LINK)
    {
      //FIXME we assume soft link!
      Attribute attr;
      char attr_name[1024];
      char link[1024];
      
      attr.setName(remove_prefix(name, basename));
      
      H5L_info_t info;
      H5Lget_val(g.getId(), g_name, link, 1024, H5P_DEFAULT);
      attr.setLink(remove_prefix(link, basename));
      
      attrs.append(attr);
    }
  }
  
  
  for(int i=0;i<g.getNumAttrs();i++) {
    H5::Attribute h5attr = g.openAttribute(i);
    Attribute attr;
    BaseType type;
    char attr_name[1024];
    
    H5Aget_name(h5attr.getId(), 1024, attr_name);
    std::string name = appendToPath(group_path, attr_name);
    read_attr(&attr, g, basename, group_path, attr_name, type);
    
    attrs.append(attr);
  }
}

void Attributes::open(H5::H5File &f, std::string &path)
{
  attrs.resize(0);
  
  H5::Group group = f.openGroup(path.c_str());
  
  attributes_append_group(*this, group, path, path);
}

void Attributes::append(Attribute attr)
{
  Attribute *at = get(attr.name);
  if (!at) {
    attr.setName(resolve(attr.name).generic_string());
    attrs.push_back(attr);
  }
  else 
    *at = attr;
}

void Attributes::append(Attribute *attr)
{
  attr->name = resolve(attr->name).generic_string();
  append(*attr);
}

void Attributes::addLink(path name, path to)
{
  Attribute a(resolve(name));
  a.setLink(to.generic_string());
  
  append(a);
}

//TODO document:
//overwrites existing attributes of same name
void Attributes::append(Attributes other)
{
  for(uint i=0;i<other.attrs.size();i++)
    append(other.attrs[i]);
}

void Attributes::append(Attributes *other)
{
  append(*other);
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

template<typename T> class insertionDispatcher<std::vector<T>> {
public:
  void operator()(std::ostream *stream, void *val, int idx)
  {
    printf("FIXME: vector string conversion!");
    //abort();
    //*stream << ((T*)val)[idx];
  }
};

template<typename T> void printthis(std::ostream *stream, void *val, int idx)
  {
    *stream << ((T*)val)[idx];
  }

std::ostream& operator<<(std::ostream& out, const Attribute& a)
{
  if (a._m.total()) {
    printf("FIXME: print N-D attr\n");
  }
  else if (a.type == BaseType::STRING) {
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

int Attribute::total()
{
  int t = 0;
  
  if (size.size())
    t = size[0];
  
  for(int i=1;i<size.size();i++)
    t *= size[i];
  
  return t;
}

void Attribute::write(H5::H5File f, std::string dataset_name)
{  
  //FIXME we should have a path?
  std::string attr_path = name;
  std::replace(attr_path.begin(), attr_path.end(), '.', '/');
  std::string fullpath = appendToPath(dataset_name, attr_path);
  std::string grouppath = remove_last_part(fullpath, '/');
  std::string attr_name = get_last_part(fullpath, '/');
  
  if (_link.size())
  {
    printf("write link %s\n", (path(dataset_name) / _link).generic_string().c_str());
    
    if (!h5_obj_exists(f, grouppath))
      h5_create_path_groups(f, grouppath.c_str());
    
    H5::Group g = f.openGroup(grouppath.c_str());
    g.link(H5G_LINK_SOFT, (path(dataset_name) / _link).generic_string().c_str(), name.c_str());
  }
  else if (_m.total() == 0) {
    //FIXME remove this (legacy) case
    hsize_t *dim = new hsize_t[size.size()+1];
    for(uint i=0;i<size.size();i++)
      dim[i] = size[i];
    H5::DataSpace space(size.size(), dim);
    H5::Attribute attr;
    H5::Group g;

    delete[] dim;
    
    if (!h5_obj_exists(f, grouppath))
      h5_create_path_groups(f, grouppath.c_str());
    
    g = f.openGroup(grouppath.c_str());
    
    uint min, max;
    
    H5Pget_attr_phase_change(H5Gget_create_plist(g.getId()), &max, &min);
    
    if (min || max)
      printf("WARNING: could not set dense storage on group, may not be able to write large attributes\n");
    
    if (H5Aexists(g.getId(), attr_name.c_str()))
      g.removeAttr(attr_name.c_str());
      
    attr = g.createAttribute(attr_name.c_str(), toH5DataType(type), space);
        
    attr.write(toH5NativeDataType(type), data);
  }
  else
    Mat_H5AttrWrite(_m, f, fullpath);
}

std::string read_string_attr(H5::H5File &f, const char *parent_group_str, const char *name)
{
  H5::Group group = f.openGroup(parent_group_str);
  return read_string_attr(f, group, name);
}
  
}