#ifndef _CLIFCALIB_H
#define _CLIFCALIB_H

#include "clif.hpp"

//TODO add progress reporting

namespace clif {
  
  //detect and store the specified calibration pattern
  bool pattern_detect(ClifDataset *f, int imgset = 0);
  
  //calibrate a stored flat calibration target (img/world correspondance) using opencv
  bool opencv_calibrate(ClifDataset *f, int flags = 0, std::string imgset = std::string(), std::string calibset = std::string());
}

#endif