#ifndef _CLIF_H
#define _CLIF_H

#include "H5Cpp.h"
#include "H5File.h"

#include <vector>
#include <iostream>

#include "enumtypes.hpp"

#include "stringtree.hpp"

#include <boost/filesystem.hpp>

namespace clif {
  
template<typename T> T clamp(T v, T l, T u)
{
  return std::min<T>(u, std::max<T>(v, l));
}
  
bool h5_obj_exists(H5::H5File &f, const char * const path);
bool h5_obj_exists(H5::H5File &f, const std::string path);
bool h5_obj_exists(H5::H5File &f, const boost::filesystem::path path);

void h5_create_path_groups(H5::H5File &f, boost::filesystem::path path);

std::string path_element(boost::filesystem::path path, int idx);
  
class InvalidBaseType {};
  
//base type for elements
enum class BaseType : int {INVALID,INT,FLOAT,DOUBLE,STRING};

static std::type_index BaseTypeTypes[] = {std::type_index(typeid(InvalidBaseType)), std::type_index(typeid(int)), std::type_index(typeid(float)), std::type_index(typeid(double)), std::type_index(typeid(char))};

static std::unordered_map<std::type_index, BaseType> BaseTypeMap = { 
    {std::type_index(typeid(char)), BaseType::STRING},
    {std::type_index(typeid(int)), BaseType::INT},
    {std::type_index(typeid(float)), BaseType::FLOAT},
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
    case BaseType::FLOAT : F<float>()(args...); break;
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
    case BaseType::FLOAT : return F<float>()(args...); break;
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
#define CLIF_CVT_GRAY 16

  int parse_string_enum(std::string &str, const char **enumstrs);
  int parse_string_enum(const char *str, const char **enumstrs);
  
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
  
  std::vector<std::string> listH5Datasets(H5::H5File &f, std::string parent);
      
  class Attributes {
    public:
      Attributes() {};
      
      //get attributes from ini file(s) TODO second represents types for now!
      Attributes(const char *inifile, const char *typefile);
      Attributes(H5::H5File &f, std::string &name);
      
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
      void size(int s[3]);
      void imgsize(int s[2]);
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
      Dataset(H5::H5File &f_, std::string path);
      
      //void set(Datastore &data_) { data = data_; };
      //TODO should this call writeAttributes (and we completely hide io?)
      void setAttributes(Attributes &attrs) { static_cast<Attributes&>(*this) = attrs; };   
      //writes only Attributes! FIXME hide Attributes::Write
      void writeAttributes() { Attributes::write(f, _path); }
      
      bool valid();
      
      boost::filesystem::path path();
      
      H5::H5File f;
      std::string _path;
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
      case clif::BaseType::FLOAT : return F<float>()(this, args...); break;
      case clif::BaseType::DOUBLE : return F<double>()(this, args...); break;
      case clif::BaseType::STRING : return F<char>()(this, args...); break;
      default:
        abort();
    }
  }
  
  int imgCount() { return clif::Datastore::count(); };
  int attributeCount() { return clif::Attributes::count(); };
  
  bool valid() { return clif::Dataset::valid() && clif::Datastore::valid(); };
  
  Clif3DSubset *get3DSubset(int idx = 0);
  
  clif::Datastore *getCalibStore();
  clif::Datastore *createCalibStore();
  
  boost::filesystem::path getpath(boost::filesystem::path parent, int idx);
  
private:
  
  ClifDataset(ClifDataset &other);
  ClifDataset &operator=(ClifDataset &other);
  Datastore *calib_images = NULL;
  
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
  
  H5::H5File f;
private:
  
  std::vector<std::string> datasets;
};

//TODO here start public cv stuff -> move to extra header files
#include "opencv2/core/core.hpp"

//only adds methods
//TODO use clif namespace! (?)
namespace clif_cv {
  
  using namespace clif;
  
  DataType CvDepth2DataType(int cv_type);
  int DataType2CvDepth(DataType t);

  cv::Size imgSize(Datastore *store);
    
  void writeCvMat(Datastore *store, uint idx, cv::Mat &m);
  void readCvMat(Datastore *store, uint idx, cv::Mat &m, int flags = 0);
  
  void readCalibPoints(ClifDataset *set, std::string calib_set_name, std::vector<std::vector<cv::Point2f>> &imgpoints, std::vector<std::vector<cv::Point2f>> &worldpoints);
  void writeCalibPoints(Dataset *set, std::string calib_set_name, std::vector<std::vector<cv::Point2f>> &imgpoints, std::vector<std::vector<cv::Point2f>> &worldpoints);
  
  //void readEPI(ClifDataset *lf, cv::Mat &m, int line, double depth = 0, int flags = 0);

  void writeCvMat(Datastore *store, uint idx, hsize_t w, hsize_t h, void *data);
}

#endif