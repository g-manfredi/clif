#ifndef _CLIF3DSUBSET_H
#define _CLIF3DSUBSET_H

#include "clif.hpp"

#include <opencv2/imgproc/imgproc.hpp>

namespace clif {

class Subset3d {
public:
  Subset3d() {};
  //takes the line'nth line definition found in calibration.extrinsincs
  Subset3d(clif::Dataset *data, std::string extr_group = std::string());
  
  void readEPI(cv::Mat &m, int line, double depth, int flags = 0, int interp = CV_INTER_LANCZOS4, float scale = 1.0, ClifUnit unit = ClifUnit::PIXELS);
  
private:
  clif::Dataset *_data = NULL;
  std::vector<std::pair<int,int>> indizes;
  
  //TODO more generic model!
  double step_length;
  double f[2];
};

}

#endif