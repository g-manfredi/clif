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

  class Attribute {
    public:
      Attribute() {};
      template<typename T> void Set(std::string name_, int dims_, T *size_, BaseType type_, void *data_);
      
      template<typename T> T get();

      void write(H5::H5File &f, std::string dataset_name);
      
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
      void write(H5::H5File &f, std::string &name);
      
    protected:
      std::vector<Attribute> attrs; 
  };
  
  //representation of a "raw" clif datset - mostly the images
  class Datastore {
    public:
      Datastore() {};
      
      //open datastore for writing
      Datastore(H5::H5File &f, const std::string parent_group_str, const std::string name, uint width, uint height, uint count, DataType datatype, DataOrg dataorg, DataOrder dataorder);
      //open datastore for writing
      //Datastore(H5::H5File &f, const std::string parent_group_str, uint width, uint height, uint count, Attributes &attrs);
      
      void writeRawImage(uint idx, void *data);
      void readRawImage(uint idx, void *data);
      
      int imgMemSize();
      
      bool isValid();
      int count();
      
      H5::DataSet data;
    protected:
      DataType type; 
      DataOrg org;
      DataOrder order;
  };
  
  class Dataset {
    public:
      Dataset(H5::H5File &f_, std::string name_) : f(f_), name(name_) {};
      
      void set(Datastore &data_) { data = data_; };
      void set(Attributes &attrs_) { attrs = attrs_; };
      
      //save attributes!
      ~Dataset();
      
    private:
      H5::H5File f;
      std::string name;
      Datastore data;
      Attributes attrs;
  };
  
  H5::PredType H5PredType(DataType type);
  
  std::vector<std::string> Datasets(H5::H5File &f);
}

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