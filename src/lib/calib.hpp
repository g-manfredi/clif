#ifndef _CLIFCALIB_H
#define _CLIFCALIB_H

#include "core.hpp"
#include "clif.hpp"

//TODO add progress reporting

namespace clif {
  
  //detect and store the specified calibration pattern
  bool pattern_detect(Dataset *s, cpath imgset = cpath(), cpath calibset = cpath(), bool write_debug_imgs = true);
  
  //calibrate a stored flat calibration target (img/world correspondance) using opencv
  bool opencv_calibrate(Dataset *f, int flags = 0, cpath map = cpath(), cpath calib = cpath());
  bool ucalib_calibrate(Dataset *set, cpath proxy = cpath(), cpath calib = cpath());
  
  //fit grid of img/world mappings to calibration stack
  bool generate_proxy_loess(Dataset *set, int proxy_w, int proxy_h , cpath map = cpath(), cpath proxy = cpath());
}

#endif