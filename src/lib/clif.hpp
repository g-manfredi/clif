#include "H5Cpp.h"
#include "H5File.h"

#include <vector>

#include "enumtypes.hpp"

namespace clif {
                        
  DataType CvDepth2DataType(int cv_type);

  //representation of a "raw" clif datset - the images
  class Datastore {
    public:
      //open datastore for writing
      Datastore(H5::H5File &f, const std::string parent_group_str, uint width, uint height, uint count, DataType datatype, DataOrg dataorg, DataOrder dataorder);
      //open datastor for reading
      Datastore(H5::H5File &f, const std::string parent_group_str);
      
      void writeRawImage(uint idx, void *data);
      void readRawImage(uint idx, void *data);
      
      int imgMemSize();
    private:
      H5::DataSet data;
      DataType type; 
      DataOrg org;
      DataOrder order;
  };
  
  H5::PredType H5PredType(DataType type);
  
  std::vector<std::string> Datasets(H5::H5File &f);
}

//TODO here start public cv stuff -> move to extra header files
#include "opencv2/core/core.hpp"

namespace clif_cv {
    class CvDatastore : public clif::Datastore
    {
    public:
      void writeOpenCvMat(uint idx, cv::Mat &m);
      void readOpenCvMat(uint idx, cv::Mat &m);
    };
}