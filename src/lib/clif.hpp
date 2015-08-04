#include "H5Cpp.h"

namespace clif {

  enum class DataType {UINT8}; 
  enum class DataOrg {BAYER_2x2};
  enum class DataOrder {RGGB,BGGR,GBRG,GRBG,
                        RGB};

  //representation of a "raw" clif datset - the images
  class Datastore {
    public:
      Datastore(H5::H5File &f, const std::string parent_group_str, uint width, uint height, uint count, DataType datatype, DataOrg dataorg, DataOrder dataorder);
      void writeImage(uint idx, void *data);
    private:
      H5::DataSet data;
      DataType type; 
      DataOrg org;
      DataOrder order;
  };
}