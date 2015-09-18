#ifndef _CLIF3DSUBSET_H
#define _CLIF3DSUBSET_H

#include "clif_cv.hpp"

#include <opencv2/imgproc/imgproc.hpp>

namespace clif {

class Subset3d {
public:
  Subset3d() {};
  //takes the line'nth line definition found in calibration.extrinsincs
  Subset3d(clif::Dataset *data, std::string extr_group = std::string());
  
  void readEPI(cv::Mat &m, int line, double disparity, ClifUnit unit = ClifUnit::PIXELS, int flags = 0, Interpolation interp = Interpolation::LINEAR, float scale = 1.0);
  void readEPI(std::vector<cv::Mat> &channels, int line, double disparity, ClifUnit unit = ClifUnit::PIXELS, int flags = 0, Interpolation interp = Interpolation::LINEAR, float scale = 1.0);
  
  double depth2disparity(double depth, double scale = 1.0)
  {
    return f[0]*step_length/depth/scale;
  }
  
  double disparity2depth(double disparity, double scale = 1.0)
  {
    return f[0]*step_length/disparity*scale;
  }
  
  clif::Dataset *dataset() { return _data; }
  
private:
  clif::Dataset *_data = NULL;
  std::vector<std::pair<int,int>> indizes;
  
  //TODO more generic model!
  double step_length;
  double f[2];
};

}

#endif