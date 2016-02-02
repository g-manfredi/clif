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
  Subset3d(clif::Dataset *data, cpath extr_group = cpath(), const ProcData & proc = ProcData());
  void create(clif::Dataset *data, cpath extr_group = cpath(), const ProcData & proc = ProcData());
  
  void readEPI(cv::Mat *epi, int line, double disparity, Unit unit = Unit::PIXELS);
  void unshift_epi(cv::Mat *epi, cv::Mat *slice, int line, double disparity, Unit unit = Unit::PIXELS);
  
  inline double depth2disparity(double depth)
  {
    return f()*step_length/depth;
  }
  
  double disparity2depth(double disparity)
  {
    return f()*step_length/disparity;
  }
  
  int EPICount();
  int EPIWidth();
  int EPIHeight();
  int EPIDepth();
  
  ExtrType type();
  
  clif::Dataset *dataset() { return _data; }
  
  inline double f(int dim = 0)
  {
    return _proc.scale(_f[dim]);
  }
  
  double _f[2];
  double step_length;
  //double world_to_camera[6];
private:
  clif::Dataset *_data = NULL;
  clif::Datastore *_store = NULL;
  std::vector<std::pair<int,int>> indizes;
  
  ProcData _proc;
  
  //TODO more generic model!
  ExtrType _type;
};

}

#endif