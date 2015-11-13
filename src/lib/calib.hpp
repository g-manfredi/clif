#ifndef _CLIFCALIB_H
#define _CLIFCALIB_H

#include "clif.hpp"

//TODO add progress reporting

namespace clif {
  
  //detect and store the specified calibration pattern
  bool pattern_detect(Dataset *f, int imgset = 0, bool write_debug_imgs = true);
  
  //calibrate a stored flat calibration target (img/world correspondance) using opencv
  bool opencv_calibrate(Dataset *f, int flags = 0, std::string imgset = std::string(), std::string calibset = std::string());
  
  //fit grid of img/world mappings to calibration stack
  bool generate_proxy_loess(Dataset *set, int proxy_w, int proxy_h , std::string imgset = std::string(), std::string calibset = std::string());
}

#endif