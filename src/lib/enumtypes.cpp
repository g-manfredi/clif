#include "enumtypes.hpp"

#include <opencv2/core/core.hpp>

namespace clif {

//TODO create mappings with cmake?
//FIXME other types!
BaseType CvDepth2BaseType(int cv_type)
{
  switch (cv_type) {
    case CV_8U : return clif::BaseType::UINT8;
    case CV_16U : return clif::BaseType::UINT16;
    default :
      abort();
  }
}

int BaseType2CvDepth(BaseType t)
{
  switch (t) {
    case clif::BaseType::UINT8 : return CV_8U;
    case clif::BaseType::UINT16 : return CV_16U;
    default :
      abort();
  }
}

}