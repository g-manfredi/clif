#include "enumtypes.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

namespace clif {

int order2cv_conf_flag(DataOrder order)
{
  switch (order) {
    case DataOrder::RGGB :
      return CV_BayerRG2BGR;
    case DataOrder::BGGR :
      return CV_BayerBG2BGR;
    case DataOrder::GBRG :
      return CV_BayerGB2BGR;
    case DataOrder::GRBG :
      return CV_BayerGR2BGR;
    default :
      abort();
  }
}

}
