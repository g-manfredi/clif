#include "H5Cpp.h"
#include "H5File.h"

#include <vector>

#include "enumtypes.hpp"

namespace clif {
  
//base type for elements
enum class BaseType {INVALID,INT,DOUBLE,STRING};

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
      
      template<typename T> T get();

      void write(H5::H5File &f, std::string dataset_name);
      std::string toString();
      
      std::string name;
    private:
      
      //HDF5 already has a type system - use it.
      BaseType type = BaseType::INVALID;
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
  
 template<typename T> T Attribute::get()
 {
   return (T)data;
  }
      
  class Attributes {
    public:
      Attributes() {};
      
      //get attributes from ini file(s) TODO second represents types for now!
      Attributes(const char *inifile, const char *typefile);
      Attributes(H5::H5File &f, std::string &name);
      
      Attribute *get(const char *name);
      
      
      void append(Attribute &attr);
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
      Datastore(Dataset *dataset, std::string path, int w, int h, int count);
      
      //open existing datastore
      Datastore(Dataset *dataset, std::string path);
      
      
      void writeRawImage(uint idx, void *data);
      void readRawImage(uint idx, void *data);
      
      int imgMemSize();
      
      bool valid();
      int count();
      
      H5::DataSet data;
    protected:
      DataType type; 
      DataOrg org;
      DataOrder order;
  };
  
  class Dataset {
    public:
      Dataset() {};
      Dataset(H5::H5File &f_, std::string name_);
      
      //void set(Datastore &data_) { data = data_; };
      //TODO should this call writeAttributes (and we completely hide io?)
      void setAttributes(Attributes &attrs_) { attrs = attrs_; };
      
      //directly pass on some Attribute functions
      Attribute *getAttribute(const char *name) { attrs.get(name); };
      template<typename T> T getEnum(const char *name) { string_to_enum<T>(attrs.get(name)->get<char*>()); };
      template<typename T> void readEnum(const char *name, T &val) { val = getEnum<T>(name); };
      
      void writeAttributes();
      
      bool valid();
      
      H5::H5File f;
      std::string name;
      Attributes attrs;

    private:
      //Datastore data;
  };
  
  H5::PredType H5PredType(DataType type);
  
  std::vector<std::string> Datasets(H5::H5File &f);
    
}

//specific (high-level) Clif handling - uses Dataset and Datastore to access
//the attributes and the "main" dataStore
//plus addtitional functions which interpret those.
class ClifDataset : public clif::Dataset, public clif::Datastore
{
public:
  ClifDataset(H5::H5File &f, std::string name);
  
  bool valid();
  
  //TODO for future:
  //clif::Datastore calibrationImages;
};

class ClifFile
{
  ClifFile();
  ClifFile(std::string &filename, unsigned int flags);
  
  void open(std::string &filename, unsigned int flags);
  void close();
  
  ClifDataset openDataset(int idx);
  ClifDataset openDataset(std::string name);
  int datasetCount();
  std::vector<std::string> datasetList();
};

//TODO here start public cv stuff -> move to extra header files
#include "opencv2/core/core.hpp"

//only adds methods
namespace clif_cv {
  
  using namespace clif;
  
  DataType CvDepth2DataType(int cv_type);
  int DataType2CvDepth(DataType t);
  
  class CvDatastore : public clif::Datastore
  {
  public:
    using Datastore::Datastore;
    
    cv::Size imgSize();
    
    void writeCvMat(uint idx, cv::Mat &m);
    void readCvMat(uint idx, cv::Mat &m, int flags = 0);
  };
}