#include "H5Cpp.h"
#include "H5File.h"

#include <vector>
#include <iostream>

#include "enumtypes.hpp"

namespace clif {
  
class InvalidBaseType {};
  
//base type for elements
enum class BaseType : int {INVALID,INT,DOUBLE,STRING};

std::type_index BaseTypeTypes[] = {std::type_index(typeid(InvalidBaseType)), std::type_index(typeid(int)), std::type_index(typeid(double)), std::type_index(typeid(char))};

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
#define CLIF_CVT_8BIT  2
#define CLIF_UNDISTORT 4

  int parse_string_enum(std::string &str, const char **enumstrs);
  int parse_string_enum(const char *str, const char **enumstrs);

  class StringTree {
  public:
    StringTree() {};
    StringTree(std::string name, void *data);
    
    void print(int depth = 0);
    
    void add(std::string str, void *data, char delim);
    int childCount();
    StringTree *operator[](int idx);
    
    std::pair<std::string, void*> *search(std::string str, char delim);
    
    std::pair<std::string, void*> val;
    std::vector<StringTree> childs;
  };
  
  class Attribute {
    public:
      Attribute() {};
      template<typename T> void Set(std::string name_, int dims_, T *size_, BaseType type_, void *data_);
      
      void setName(std::string name_) { name = name_; };
      
      const char *get()
      {
        if (type != BaseType::STRING)
          throw std::invalid_argument("Attribute type doesn't match requested type.");
        return (char*)data;
      };
      
      template<typename T> T getEnum()
      {
        T val = string_to_enum<T>(get());
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
      
      template<typename STRINGTYPE> Attribute *getAttribute(STRINGTYPE name)
      {    
        for(int i=0;i<attrs.size();i++)
          if (!attrs[i].name.compare(name))
            return &attrs[i];

        return NULL;
      }
      
      template<typename T> void getAttribute(const char *name, T &val) { getAttribute(name)->get(val); };
      template<typename T> void getAttribute(const char *name, std::vector<T> &val) { getAttribute(name)->get(val); };
      
      template<typename T> void setAttribute(const char *name, T &val);
      template<typename T> void setAttribute(const char *name, std::vector<T> &val);
      
      template<typename T> T getEnum(const char *name) { getAttribute(name)->getEnum<T>(); };
      template<typename T> void readEnum(const char *name, T &val) { val = getEnum<T>(name); };
      
      
      void append(Attribute &attr);
      void append(Attributes &attrs);
      int count();
      Attribute operator[](int pos);
      void write(H5::H5File &f, std::string &name);
      StringTree getTree();
      
    protected:
      std::vector<Attribute> attrs; 
  };
  
  class Dataset;
  
  //representation of a "raw" clif datset - mostly the images
  class Datastore {
    public:
      Datastore() {};
      
      //create new datastore
      void create(Dataset *dataset, std::string path);
      
      //open existing datastore
      void open(Dataset *dataset, std::string path);
      
      
      void writeRawImage(uint idx, hsize_t w, hsize_t h, void *data);
      void appendRawImage(hsize_t w, hsize_t h, void *data);
      void readRawImage(uint idx, hsize_t w, hsize_t h, void *data);
      
      int imgMemSize();
      
      bool valid();
      int count();

    protected:
      void initialize_internal(hsize_t w, hsize_t h);
      
      DataType type; 
      DataOrg org;
      DataOrder order;
      
      H5::DataSet data;
      clif::Dataset *parent_set;
      std::string path;
      
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

//specific (high-level) Clif handling - uses Dataset and Datastore to access
//the attributes and the "main" dataStore
//plus addtitional functions which interpret those.
class ClifDataset : public clif::Dataset, public virtual clif::Datastore
{
public:
  ClifDataset() {};
  //TODO maybe no special constructors but open/create methods?
  //open existing dataset
  void open(H5::H5File &f, std::string name);
  //create new dataset
  void create(H5::H5File &f, std::string name);
  
  int imgCount() { clif::Datastore::count(); };
  int attributeCount() { clif::Attributes::count(); };
  
  bool valid() { return clif::Dataset::valid() && clif::Datastore::valid(); };
  
  //TODO for future:
  //clif::Datastore calibrationImages;
};

class ClifFile
{
public:
  ClifFile() {};
  //TODO create file if not existing?
  ClifFile(std::string &filename, unsigned int flags);
  
  void open(std::string &filename, unsigned int flags);
  void create(std::string &filename);
  //void close();
  
  ClifDataset openDataset(int idx);
  ClifDataset openDataset(std::string name);

  ClifDataset createDataset(std::string name);
  
  int datasetCount();
  
  bool valid();
  
  std::vector<std::string> datasetList() {return datasets;};
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
  
  class CvDatastore : public virtual clif::Datastore
  {
  public:
    using Datastore::Datastore;
    
    cv::Size imgSize();
    
    void writeCvMat(uint idx, cv::Mat &m);
    void readCvMat(uint idx, cv::Mat &m, int flags = 0);
  };
}

//the datastore in inherited via ClifDataset iw the same as the on inherited in CvDatastore, allowing to apply the respective opencv methods
class CvClifDataset : public ClifDataset, public clif_cv::CvDatastore
{
public:
  using ClifDataset::ClifDataset;
  
  //ClifDataset::valid calls Datastore::valid anyway
  bool valid() { ClifDataset::valid(); };
};

//convenience class get casting
class CvClifFile : public ClifFile
{
public:
  using ClifFile::ClifFile;
  
  //FIXME there has to be a better way than those temporaries
  template<typename ... Ts> CvClifDataset openDataset(Ts ... args)
  {
    ClifDataset tmp1 = ClifFile::openDataset(args...);
    CvClifDataset tmp2 = static_cast<CvClifDataset&>(tmp1);
    return tmp2;
  };
  
    //FIXME there has to be a better way than those temporaries
  template<typename ... Ts> CvClifDataset createDataset(Ts ... args)
  {
    ClifDataset tmp1 = ClifFile::createDataset(args...);
    CvClifDataset tmp2 = static_cast<CvClifDataset&>(tmp1);
    return tmp2;
  };
};