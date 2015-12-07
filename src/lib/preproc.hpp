#ifndef _CLIF_PREPROC_H
#define _CLIF_PREPROC_H

#include <limits>

#include "mat.hpp"

namespace clif {
  
enum Improc {
  DEMOSAIC = 1, 
  CVT_8U = 2,
  CVT_GRAY = 8,
  EXP_STACK = 16,
  UNDISTORT = 32,
  NO_DISK_CACHE = 64,
  NO_MEM_CACHE = 128,
  FORCE_REUSE = 256,
  MAX = 128
};

//if input != output then max defaults to input max type
//flt/dbl output is scaled then to 0..1
//void proc_image(Mat &in, Mat &out, int flags, int scaledown, double min = std::numeric_limits<double>::quiet_NaN(), double max = std::numeric_limits<double>::quiet_NaN());

} //namespace clif

#endif //_CLIF_PREPROC_H