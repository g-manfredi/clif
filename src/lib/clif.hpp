#include "H5Cpp.h"
#include "H5File.h"

#include <vector>

#include "enumtypes.hpp"

namespace clif {

#define CLIF_DEMOSAIC  1
#define CLIF_CVT_8BIT  2
#define CLIF_UNDISTORT 4
  
  //representation of a "raw" clif datset - the images
  class Datastore {
    public:
      Datastore() {};
      
      //open datastore for writing
      Datastore(H5::H5File &f, const std::string parent_group_str, uint width, uint height, uint count, DataType datatype, DataOrg dataorg, DataOrder dataorder);
      //open datastor for reading
      Datastore(H5::H5File &f, const std::string parent_group_str);
      
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