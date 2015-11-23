#ifndef _CLIF3DSUBSET_H
#define _CLIF3DSUBSET_H

#include "dataset.hpp"

#include <opencv2/imgproc/imgproc.hpp>

#include "enumtypes.hpp"
#include "clif_cv.hpp"
#include "core.hpp"

namespace clif {

class Subset3d {
public:
  Subset3d() {};
  //takes the line'nth line definition found in calibration.extrinsincs
  Subset3d(clif::Dataset *data, const int idx);
  Subset3d(clif::Dataset *data, std::string extr_group = std::string());
  void create(clif::Dataset *data, std::string extr_group = std::string());
  
  void readEPI(cv::Mat *epi, int line, double disparity, Unit unit = Unit::PIXELS, int flags = 0, Interpolation interp = Interpolation::LINEAR, float scale = 1.0);
  
  double depth2disparity(double depth, double scale = 1.0)
  {
    return f[0]*step_length/depth/scale;
  }
  
  double disparity2depth(double disparity, double scale = 1.0)
  {
    return f[0]*step_length/disparity*scale;
  }
  
  int EPICount();
  int EPIWidth();
  int EPIHeight();
  
  clif::Dataset *dataset() { return _data; }
  
  double f[2];
private:
  clif::Dataset *_data = NULL;
  std::vector<std::pair<int,int>> indizes;
  
  //TODO more generic model!
  double step_length;
};

}

#endif