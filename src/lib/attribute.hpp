#ifndef _CLIF_ATTRIBUTE_H
#define _CLIF_ATTRIBUTE_H

#include "mat.hpp"

#include <opencv2/core/core.hpp>
#include "fnmatch.h"

#include "core.hpp"
#include "stringtree.hpp"
#include "enumtypes.hpp"


namespace clif {
  
/** %Attribute (Metadata) handling for CLIF
This class implements templated attribute access. Most times access will be trough the Attributes class, which uses the methods of this class internally, therefore more detailed documentation is available for the Attributes class.
*/

namespace {
  //TODO all possible output types ...
  template <typename T> struct _type_is_number { static const bool value = false; };
  template<> struct _type_is_number<int> { static const bool value = true; };
  template<> struct _type_is_number<float> { static const bool value = true; };
  template<> struct _type_is_number<double> { static const bool value = true; };
  
  template<typename CT, typename T> class convertFromBaseTypeDispatcher {
  public:
    void operator()(BaseType type, void *data, CT *convertto)
    {
      if (type == BaseType::STRING && _type_is_number<CT>::value)
        //convert string to numbers with atof
        *convertto = atof((char*)data);
      else
        //try simple implicit cast
        *convertto = *(T*)data;
    }
  };
  
  template<typename CT> class convertFromBaseTypeDispatcher<CT, cv::Point2f> {
  public:
    void operator()(BaseType type, void *data, CT *convertto)
    {
      abort();
    }
  };
  
  template<typename CT, typename T> class convertFromBaseTypeDispatcher<CT, std::vector<T>> {
  public:
    void operator()(BaseType type, void *data, CT *convertto)
    {
      abort();
    }
  };
  
  
  /*template<typename CT, typename T, typename std::enable_if<std::is_convertible<CT,T>::value, int>::type* = nullptr> class convertFromBaseTypeDispatcher {
  public:
    void operator()(BaseType type, void *data, CT *convertto)
    {
      if (type == BaseType::STRING && _type_is_number<CT>::value)
        //convert string to numbers with atof
        *convertto = atof((char*)data);
      else
        //try simple implicit cast
        *convertto = *(T*)data;
    }
  };*/
  
  /*template<typename CT, typename T, class = typename std::enable_if<std::is_integral<T>::value>::type> class convertFromBaseTypeDispatcher {
  public:
    void operator()(BaseType type, void *data, CT *convertto)
    {
      if (type == BaseType::STRING && _type_is_number<CT>::value)
        //convert string to numbers with atof
        *convertto = atof((char*)data);
      else
        //try simple implicit cast
        *convertto = *(T*)data;
    }
  };*/
}

class Attribute {  
  public:
    template<typename T> void set(cpath name_, int dims_, T *size_, BaseType type_, void *data_);
    
    void setName(boost::filesystem::path name_) { name = name_.generic_string(); };
    
    Attribute() {};
    Attribute(const cpath &name_)  { name = name_; };
    
    const char *getStr()
    {
      if (type != BaseType::STRING)
        throw std::invalid_argument("Attribute type doesn't match requested type");
      return (char*)data;
    };
    
    template<typename T> T getEnum()
    {
      T val = string_to_enum<T>(getStr());
      if (int(val) == -1)
        throw std::invalid_argument("could not parse enum.");
      
      return val;
    };
    
    template<typename T> void readEnum(T &val)
    {
      val = getEnum<T>();
    };
    
    template<typename T> void get(T &val)
    {
      if (BaseType2typeid(type) != std::type_index(typeid(T)))
        throw std::invalid_argument("Attribute type doesn't match requested type.");
      
      val = *(T*)data;
    };
    
    void get(Mat &m)
    {    
      if (!_m.total())
        abort();
      
      m = _m;
    }
    
    template<typename T> void get(Mat_<T> &m)
    {    
      if (!_m.total())
        abort();
      if (m.type() != _m.type())
        throw std::invalid_argument("Attribute type doesn't match requested type.");
      
      m = Mat_<T>();
      m = *(Mat_<T>*)&_m;
    }
    
    template<typename T> void convert(T *val)
    {
      callByBaseType<T,convertFromBaseTypeDispatcher>(type, type, data, val);
    };
    
    template<typename T> void get(T *val, int count)
    {
      if (BaseType2typeid(type) != std::type_index(typeid(T)))
        throw std::invalid_argument("Attribute type doesn't match requested type.");
      
      if (size[0] != count)
        throw std::invalid_argument("Attribute size doesn't match requested size.");
      
      for(int i=0;i<size[0];i++)
        val[i] = ((T*)data)[i];
    };
    
    template<typename T> void get(std::vector<T> &val)
    {
      if (BaseType2typeid(type) != std::type_index(typeid(T)))
        throw std::invalid_argument("Attribute type doesn't match requested type.");
      
      //TODO n-D handling!
      val.resize(size[0]);
      for(int i=0;i<size[0];i++)
        val[i] = ((T*)data)[i];
    };
    
    void get(cv::Mat &val)
    {
      int *sizes = new int[size.size()+1];
      for(unsigned int i=0;i<size.size();i++)
        sizes[i] = size[i];
      
      val = cv::Mat(size.size(), sizes, BaseType2CvDepth(type), data);
	  delete sizes;
    };
    
    template<typename T> void set(T &val)
    {
      type = toBaseType<T>();
      assert(type != BaseType::INVALID);
      
      data = new T[1];
      
      size.resize(1);
      size[0] = 1;
      
      ((T*)data)[0] = val;
      
    };
    
    template<typename T> void set(std::vector<T> &val)
    {
      type = toBaseType<T>();
      assert(type != BaseType::INVALID);
      
      //TODO n-D handling!
      
      //FIXME delete!
      data = new T[val.size()];
      
      size.resize(1);
      size[0] = val.size();
      
      for(int i=0;i<size[0];i++)
        ((T*)data)[i] = val[i];
      
    };
    
    void setLink(const boost::filesystem::path &l);
    const std::string& link() const;
    
    void set(cv::Mat &val)
    {
      type = CvDepth2BaseType(val.depth());
      assert(type != BaseType::INVALID);
      
      data = malloc(val.total()*val.elemSize());
      
      size.resize(val.dims);
      //FIXME correct order?!?
      for(unsigned int i=0;i<size.size();i++)
        size[i] = val.size[size.size()-i-1];
      
      memcpy(data, val.data, val.total()*val.elemSize());
    };
    
    
    template<typename T> class vector_elements_from_mat_dispatcher {
    public:
      void* operator()(int count, cv::Mat *m)
      {
        std::vector<T> *v = new std::vector<T>[count];
        
        assert(m->isContinuous());
        
        for(int i=0;i<m->total();i++)
          v[i] = ((std::vector<T>*)m->data)[i];
        
        return v;
      }
    };
    
    template<typename T> class vector_elements_delete_dispatcher {
    public:
      void* operator()(int count, cv::Mat *m)
      {
        std::vector<T> *v = new std::vector<T>[count];
        
        assert(m->isContinuous());
        
        for(int i=0;i<m->total();i++)
          v[i] = ((std::vector<T>*)m->data)[i];
        
        return v;
      }
    };
    
    void set(Mat &m)
    {
      type = m.type();
      size = m;
      
      _m = m;
    };
    
    //FIXME correct handling of Mat_?
    template<typename T> void set(Mat_<T> &m)
    {
      type = m.type();
      size = m;
      
      _m = m;
    };
    
    
    template<typename T> void set(T *val, int count = 1)
    {
      type = toBaseType<T>();
      assert(type != BaseType::INVALID);

      //FIXME delete!
      data = new T[count];
      
      size.resize(1);
      size[0] = count;
      
      //TODO n-D handling!
      for(int i=0;i<count;i++)
        ((T*)data)[i] = val[i];
    };
    
    void set(const char *val)
    {
      type = toBaseType<char>();
      int count = strlen(val)+1;

      //FIXME delete!
      data = new char[count];
      
      size.resize(1);
      size[0] = count;
      
      //TODO n-D handling!
      for(int i=0;i<count;i++)
        ((char*)data)[i] = val[i];
    };

    void write(H5::H5File f, const cpath & dataset_root);
    std::string toString();
    
    friend std::ostream& operator<<(std::ostream& out, const Attribute& a);

    int total();
    
    cpath name;
    BaseType type = BaseType::INVALID;
  private:
    
    //HDF5 already has a type system - use it.
    int dims = 0;
    std::vector<int> size;
    void *data = NULL;
    
    std::string _link;
    
    Mat _m;
};

/** Main class for metadata handling.
 Stores an Attribute list and provides path based access. The [setAttribute](@ref Attributes::setAttribute) and [get](@ref getattribute_group) methods are templated and of the form <tt>function(pathtype, array_reference)</tt>, using the following types:

\anchor getattribute_table
pathtype                | Notes
-------------------     |--------------
\c const char *            | |
\c std::string             | |
\c clif::path              | this is the \c boost::filesystem::path class

array_reference (of type \c T) | Notes
-------------------     |--------------
\c T *, int                | int defaults to one (single element attributes)
\c vector<T>             | the only way (for [now](@ref todo_attribute_element_access)) to read attributes with unknown size

Supported types for \c T are: \c int, \c float, \c double, [TODO string types](@ref todo_attribute_string).

For known enum types (see [clif](@ref clif)) the setEnum and getEnum methods provide automatic parsing and string generation.
*/
class Attributes {
  public:
    Attributes() {};
    
    /** read Attributes from an ini file (using \c typefile for type interpretation)
     * @param inifile file name of the ini file to load
     * @param typefile file name of the ini file which provides type information (see [type system](@ref ini_type_system)).
     */
    void open(const char *inifile, const char *typefile);
    
    void open(const char *inifile, cliini_args *types = NULL);
    
    /** read Attributes from and hdf5 file
     * @param f hdf5 file handler
     * @param path the dataset root path (e.g. "/clif/datasetname")
     */
    void open(H5::H5File &f, const cpath &path);
    
    /** get single attribute by name or index, not normally used directly \anchor getattribute_group
     * @name Read Attributes
     */
    //@{
    Attribute *get(int idx)
    {    
      return &attrs[idx];
    }
    //other types
    Attribute *get(boost::filesystem::path name);
    
    //TODO will not work with soft links (but is also not required?)
    Attribute *getMatch(const char *name)
    {    
      for(unsigned int i=0;i<attrs.size();i++)
        if (!fnmatch(attrs[i].name.generic_string().c_str(), name, FNM_PATHNAME))
          return &attrs[i];
        
        return NULL;
    }
    Attribute *getMatch(boost::filesystem::path name)
    {    
      return getMatch(name.generic_string().c_str());
    }
    Attribute *getMatch(std::string name)
    {    
      return getMatch(name.c_str());
    }
    //@}
    
    boost::filesystem::path resolve(boost::filesystem::path name);
    
    /** export attributes to an ini file which can reimported using open().
     */
    void writeIni(std::string &filename);
    
    /** search and read a single Attribute
     *  see [these tables](@ref getattribute_table) for possible arguments, or the [example](@ref getattribute_example).
     *  @throws std::invalid_argument if the wrong type is used
     *  @param name specifies the attribute path
     *  @param a1 is the actual array_reference (\c vector<T>  or <tt>T *</tt>)
     *  @param args array element count (as \c int) if \b a1 is of type <tt>T*</tt>.
     */
    template<typename S, typename T1, typename ...TS> void get(S name, T1 &a1, TS...args)
    {
      Attribute *attr = get(name);
      if (!attr)
        throw std::invalid_argument("attribute \""+name.generic_string()+"\" does not exist!");
      attr->get(a1, args...);
    };
    
    /** replace or add an attribute. Same usage as [get](@ref getattribute_group)
     */
    template<typename S, typename ...TS> void setAttribute(S name, TS...args)
    {
      Attribute a(name);
      a.set(args...);
      append(a);
    }
    
    /** search enum attribute and convert to the respective enum type \anchor getenum_group
     * the second form (getEnum(S,T)) allows the function to derive the enum type automatically,
     * see [example](@ref getattribute_example).
     * @param name specify the name as a [pathtype](@ref getattribute_table)
     * @param val the enum type
     * @name Reading Enums
     */
    //@{
    template<typename S, typename T> T getEnum(S name)
    { 
      Attribute *found = get(name);
      if (!found) return T(0);
      return found->getEnum<T>();
    };
    template<typename S, typename T> void getEnum(S name, T &val) { val = getEnum<S,T>(name); };
    //@}
    
     /** append either a single attribute or an Attributes attribute list
      * @name Append Attributes
     */ 
    //@{
    void append(Attribute attr);
    void append(Attribute *attr);
    void append(Attributes attrs);
    void append(Attributes *attrs);
    void addLink(cpath name, cpath to);
    //@}
    
    /** get number of attributes
     */
    int count();
    /** access the nth attribute
     */
    Attribute operator[](int pos);
    
    /** write all attributes under \b path into file \b f.
     * [TODO](@ref #todo_attr_write) see Dataset::writeAttributes for a more convenient method.
     */
    void write(H5::H5File f, const cpath & path);
    
    /** generate a StringTree of the stored Attributes
     */
    StringTree<Attribute*> getTree();
    
    /** list all sub-groups of a \b parent group and store in \b matches
     */
    //@{
    void listSubGroups(std::string parent, std::vector<std::string> &matches);
    void listSubGroups(boost::filesystem::path parent, std::vector<std::string> &matches) { listSubGroups(parent.generic_string(), matches); }
    void listSubGroups(const char *parent, std::vector<std::string> &matches) {listSubGroups(std::string(parent),matches); };
    //@}
    
    cpath getSubGroup(cpath parent, cpath child = cpath());
    //new group and link ->source to old group
    bool deriveGroup(const cpath & in_parent, cpath in_child, const cpath & out_parent, cpath out_child, cpath &in_root, cpath &out_root);
    bool groupExists(const cpath & path);

    
    void reset();
    
  protected:
    std::vector<Attribute> attrs; 
};

inline StringTree<Attribute*> Attributes::getTree()
{
  StringTree<Attribute*> tree;
  
  for (unsigned int i = 0; i<attrs.size(); i++)
    tree.add(attrs[i].name.generic_string(), &attrs[i], '/');
  
  return tree;
}

template<typename T> void Attribute::set(cpath name_, int dims_, T *size_, BaseType type_, void *data_)
{
  name = name_;
  type = type_;
  dims = dims_;
  size.resize(dims);
  for(int i=0;i<dims;i++)
    size[i] = size_[i];
  data = data_;
}

} //namespace clif

#endif