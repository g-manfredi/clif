#ifndef _CLIF_H
#define _CLIF_H

#include "H5Cpp.h"
#include "H5File.h"

#include <vector>
#include <iostream>

#include "enumtypes.hpp"

#include "stringtree.hpp"

namespace clif {
  
  
class InvalidBaseType {};
  
//base type for elements
enum class BaseType : int {INVALID,INT,DOUBLE,STRING};

static std::type_index BaseTypeTypes[] = {std::type_index(typeid(InvalidBaseType)), std::type_index(typeid(int)), std::type_index(typeid(double)), std::type_index(typeid(char))};

static std::unordered_map<std::type_index, BaseType> BaseTypeMap = { 
    {std::type_index(typeid(char)), BaseType::STRING},
    {std::type_index(typeid(int)), BaseType::INT},
    {std::type_index(typeid(double)), BaseType::DOUBLE}
};
  
template<typename T> BaseType toBaseType()
{
  return BaseTypeMap[std::type_index(typeid(T))];
}
  
//deep dark black c++ magic :-D
template<template<typename> class F, typename ... ArgTypes> void callByBaseType(BaseType type, ArgTypes ... args)
{
  switch (type) {
    case BaseType::INT : F<int>()(args...); break;
    case BaseType::DOUBLE : F<double>()(args...); break;
    case BaseType::STRING : F<char>()(args...); break;
    default:
      abort();
  }
}

template<template<typename> class F, typename R, typename ... ArgTypes> R callByBaseType(BaseType type, ArgTypes ... args)
{
  switch (type) {
    case BaseType::INT : return F<int>()(args...); break;
    case BaseType::DOUBLE : return F<double>()(args...); break;
    case BaseType::STRING : return F<char>()(args...); break;
    default:
      abort();
  }
}
  
#define CLIF_DEMOSAIC  1
#define CLIF_CVT_8U  2
#define CLIF_UNDISTORT 4
#define CLIF_PROCESS_FLAGS_MAX 8

  int parse_string_enum(std::string &str, const char **enumstrs);
  int parse_string_enum(const char *str, const char **enumstrs);
  
  class Attribute {
    public:
      template<typename T> void Set(std::string name_, int dims_, T *size_, BaseType type_, void *data_);
      
      void setName(std::string name_) { name = name_; };
      
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
      };
      
      template<typename T> void greadEnum(T &val)
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
        
        data = new T[1];
        
        size.resize(1);
        size[0] = 1;
        
        ((T*)data)[0] = val;
        
      };
      
      template<typename T> void set(std::vector<T> &val)
      {
        type = toBaseType<T>();
        
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

        //FIXME delete!
        data = new T[count];
        
        size.resize(1);
        size[0] = count;
        
        //TODO n-D handling!
        for(int i=0;i<count;i++)
          ((T*)data)[i] = val[i];
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
      
  class Attributes {
    public:
      Attributes() {};
      
      //get attributes from ini file(s) TODO second represents types for now!
      Attributes(const char *inifile, const char *typefile);
      Attributes(H5::H5File &f, std::string &name);
      
      std::vector<std::string> extrinsicGroups();
      
      template<typename STRINGTYPE> Attribute *getAttribute(STRINGTYPE name)
      {    
        for(int i=0;i<attrs.size();i++)
          if (!attrs[i].name.compare(name))
            return &attrs[i];

        return NULL;
      }

      void writeIni(std::string &filename);
      
      //template<typename T> void getAttribute(const char *name, T &val) { getAttribute(name)->get(val); };
      //template<typename T> void getAttribute(const char *name, std::vector<T> &val) { getAttribute(name)->get(val); };
      
      template<typename S, typename T1, typename ...TS> void getAttribute(S name, T1 a1, TS...args) { getAttribute(name)->get(a1, args...); };
      //template<typename T1, typename ...TS> void getAttribute(const char *name, T1 a1, TS...args) { getAttribute(name)->get(a1, args...); };
      
      template<typename T> void setAttribute(const char *name, T &val);
      template<typename T> void setAttribute(const char *name, std::vector<T> &val);
      
      template<typename T> T getEnum(const char *name) { getAttribute(name)->getEnum<T>(); };
      template<typename T> void getEnum(const char *name, T &val) { val = getEnum<T>(name); };
      
      
      void append(Attribute &attr);
      void append(Attributes &attrs);
      int count();
      Attribute operator[](int pos);
      void write(H5::H5File &f, std::string &name);
      StringTree<Attribute*> getTree();
      
      //find all group nodes under filter
      std::vector<std::string> listSubGroups(std::string parent);
      
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
  
  class Dataset;
  
  //representation of a "raw" clif datset - mostly the images
  class Datastore {
    public:
      Datastore() {};
      
      //create new datastore
      void create(std::string path, Dataset *dataset);
      
      //open existing datastore
      void open(Dataset *dataset, std::string path);
      
      void writeRawImage(uint idx, hsize_t w, hsize_t h, void *data);
      void appendRawImage(hsize_t w, hsize_t h, void *data);
      void readRawImage(uint idx, hsize_t w, hsize_t h, void *data);
      
      int imgMemSize();
      
      bool valid();
      int count();
      
      const std::string& getDatastorePath() const { return _path; };
      
      const H5::DataSet & H5DataSet() const { return _data; };
      const DataType & type() const { return _type; };
      const DataOrg & org() const { return _org; };
      const DataOrder & order() const { return _order; };
      
      void *cache_get(uint64_t key);
      void cache_set(uint64_t, void *data);
      
    protected:
      void init(hsize_t w, hsize_t h);
      
      DataType _type; 
      DataOrg _org;
      DataOrder _order;
      
      H5::DataSet _data;
      std::string _path;
      
  private:
    std::unordered_map<uint64_t,void*> image_cache;
    
    Dataset *_dataset = NULL;
  };
  
  class Dataset : public Attributes {
    public:
      Dataset() {};
      //FIXME use open/create methods
      Dataset(H5::H5File &f_, std::string name_);
      
      //void set(Datastore &data_) { data = data_; };
      //TODO should this call writeAttributes (and we completely hide io?)
      void setAttributes(Attributes &attrs) { static_cast<Attributes&>(*this) = attrs; };   
      //writes only Attributes! FIXME hide Attributes::Write
      void writeAttributes() { Attributes::write(f, name); }
      
      bool valid();
      
      H5::H5File f;
      std::string name;
      //Attributes attrs;

    private:
      //Datastore data;
  };
  
  H5::PredType H5PredType(DataType type);
  
  std::vector<std::string> Datasets(H5::H5File &f);
}

//TODO organisation
class Clif3DSubset;

//specific (high-level) Clif handling - uses Dataset and Datastore to access
//the attributes and the "main" dataStore
//plus addtitional functions which interpret those.
class ClifDataset : public clif::Dataset, public virtual clif::Datastore
{
public:
  ClifDataset() {};
  //open existing dataset
  void open(H5::H5File &f, std::string name);
  //create new dataset
  void create(H5::H5File &f, std::string name);
  
  ClifDataset &operator=(const ClifDataset &other) { Dataset::operator=(other); Datastore::operator=(other); return *this; }
 
 
  template<template<typename> class F, typename R, typename ... ArgTypes> R callFunctor(ArgTypes ... args)
  {
    switch (type) {
      case clif::BaseType::INT : return F<int>()(this, args...); break;
      case clif::BaseType::DOUBLE : return F<double>()(this, args...); break;
      case clif::BaseType::STRING : return F<char>()(this, args...); break;
      default:
        abort();
    }
  }
  
  int imgCount() { clif::Datastore::count(); };
  int attributeCount() { clif::Attributes::count(); };
  
  bool valid() { return clif::Dataset::valid() && clif::Datastore::valid(); };
  
  Clif3DSubset *get3DSubset(int idx = 0);
  
  clif::Datastore *getCalibStore();
  clif::Datastore *createCalibStore();
  
private:
  
  ClifDataset(ClifDataset &other);
  ClifDataset &operator=(ClifDataset &other);
  Datastore *calib_images;
  
  //TODO for future:
  //clif::Datastore calibrationImages;
};

class ClifFile
{
public:
  ClifFile() {};
  //TODO create file if not existing?
  ClifFile(std::string &filename, unsigned int flags = H5F_ACC_RDONLY);
  
  void open(std::string &filename, unsigned int flags = H5F_ACC_RDONLY);
  void create(std::string &filename);
  //void close();
  
  ClifDataset* openDataset(int idx);
  ClifDataset* openDataset(std::string name);

  ClifDataset* createDataset(std::string name);
  
  int datasetCount();
  
  bool valid();
  
  const std::vector<std::string> & datasetList() const {return datasets;};
private:
  
  H5::H5File f;
  std::vector<std::string> datasets;
};

//TODO here start public cv stuff -> move to extra header files
#include "opencv2/core/core.hpp"

//only adds methods
namespace clif_cv {
  
  using namespace clif;
  
  DataType CvDepth2DataType(int cv_type);
  int DataType2CvDepth(DataType t);

  cv::Size imgSize(Datastore &store);
    
  void writeCvMat(Datastore &store, uint idx, cv::Mat &m);
  void readCvMat(Datastore &store, uint idx, cv::Mat &m, int flags = 0);
  
  void readEPI(ClifDataset &lf, cv::Mat &m, int line, double depth = 0, int flags = 0);

  void writeCvMat(Datastore &store, uint idx, hsize_t w, hsize_t h, void *data);
}

#endif