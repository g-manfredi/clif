#ifndef _CLIF_ATTRIBUTE_H
#define _CLIF_ATTRIBUTE_H

#include "core.hpp"
#include "stringtree.hpp"

namespace clif {
  
class Attribute {
  public:
    template<typename T> void Set(std::string name_, int dims_, T *size_, BaseType type_, void *data_);
    
    void setName(std::string name_) { name = name_; };
    
    Attribute() {};
    Attribute(std::string name_)  { name = name_; };
    Attribute(const char *name_)  { name = std::string(name_); };
    Attribute(boost::filesystem::path &name_)  { name = name_.c_str(); };
    
    const char *getStr()
    {
      if (type != BaseType::STRING)
        throw std::invalid_argument("Attribute type doesn't match requested type.");
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
      if (BaseTypeTypes[int(type)] != std::type_index(typeid(T)))
        throw std::invalid_argument("Attribute type doesn't match requested type.");
      
      val = (T*)data;
    };
    
    template<typename T> void get(T *val, int count)
    {
      if (BaseTypeTypes[int(type)] != std::type_index(typeid(T)))
        throw std::invalid_argument("Attribute type doesn't match requested type.");
      
      if (size[0] != count)
        throw std::invalid_argument("Attribute size doesn't match requested size.");
      
      for(int i=0;i<size[0];i++)
        val[i] = ((T*)data)[i];
    };
    
    template<typename T> void get(std::vector<T> &val)
    {
      if (BaseTypeTypes[int(type)] != std::type_index(typeid(T)))
        throw std::invalid_argument("Attribute type doesn't match requested type.");
      
      //TODO n-D handling!
      val.resize(size[0]);
      for(int i=0;i<size[0];i++)
        val[i] = ((T*)data)[i];
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

    void write(H5::H5File &f, std::string dataset_name);
    std::string toString();
    
    friend std::ostream& operator<<(std::ostream& out, const Attribute& a);

    
    std::string name;
    BaseType type = BaseType::INVALID;
  private:
    
    //HDF5 already has a type system - use it.
    int dims = 0;
    std::vector<int> size;
    void *data = NULL;
};


class Attributes {
  public:
    Attributes() {};
    
    //get attributes from ini file(s) TODO second represents types for now!
    Attributes(const char *inifile, const char *typefile);
    //Attributes(H5::H5File &f, std::string &name);
    
    void open(H5::H5File &f, std::string &name);
    
    void extrinsicGroups(std::vector<std::string> &groups);
    
    //path type
    Attribute *getAttribute(boost::filesystem::path name)
    {    
      for(int i=0;i<attrs.size();i++)
        if (!attrs[i].name.compare(name.string()))
          return &attrs[i];
        
        return NULL;
    }
    
    Attribute *getAttribute(int idx)
    {    
      return &attrs[idx];
    }
    
    //other types
    template<typename STRINGTYPE> Attribute *getAttribute(STRINGTYPE name)
    {    
      for(int i=0;i<attrs.size();i++)
        if (!attrs[i].name.compare(name))
          return &attrs[i];

      return NULL;
    }

    void writeIni(std::string &filename);
    
    template<typename S, typename T1, typename ...TS> void getAttribute(S name, T1 &a1, TS...args)
    {
      Attribute *attr = getAttribute(name);
      if (!attr)
        throw std::invalid_argument("requested attribute does not exist!");
      attr->get(a1, args...);
    };
    
    
    template<typename S, typename ...TS> void setAttribute(S name, TS...args)
    {
      /*Attribute *a = getAttribute(name);
      
      //FIXME ugly!
      if (!a) {
        a = new Attribute(name);
        append(*a);
        delete a;
        a = getAttribute(name);
      }
      
      a->set(args...);*/
      Attribute a(name);
      a.set(args...);
      append(a);
    }
    
    template<typename S, typename T> T getEnum(S name) { return getAttribute(name)->getEnum<T>(); };
    template<typename S, typename T> void getEnum(S name, T &val) { val = getEnum<S,T>(name); };
    
    
    void append(Attribute &attr);
    void append(Attribute *attr);
    void append(Attributes &attrs);
    void append(Attributes *attrs);
    int count();
    Attribute operator[](int pos);
    void write(H5::H5File &f, std::string &name);
    StringTree<Attribute*> getTree();
    
    //find all group nodes under filter
    void listSubGroups(std::string parent, std::vector<std::string> &matches);
    void listSubGroups(boost::filesystem::path parent, std::vector<std::string> &matches) { listSubGroups(parent.generic_string(), matches); }
    void listSubGroups(const char *parent, std::vector<std::string> &matches) {listSubGroups(std::string(parent),matches); };
    
  protected:
    std::vector<Attribute> attrs; 
};

inline StringTree<Attribute*> Attributes::getTree()
{
  StringTree<Attribute*> tree;
  
  for(int i=0;i<attrs.size();i++)
    tree.add(attrs[i].name, &attrs[i], '/');
  
  return tree;
}

template<typename T> void Attribute::Set(std::string name_, int dims_, T *size_, BaseType type_, void *data_)
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