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
  //from is is either extrinsics group OR subset group
  Subset3d(clif::Dataset *set, const cpath & from = cpath(), const ProcData & proc = ProcData());
  bool create(clif::Dataset *set, const cpath & from = cpath(), const ProcData & proc = ProcData());
  
  cpath save(clif::Dataset *set, const cpath & to = cpath());
  
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
  
  const ProcData & proc() const
  {
    return _proc;
  }
  
  double _f[2];
  double step_length;
  
  cpath extrinsics_group();
  //double world_to_camera[6];
private:
  clif::Dataset *_data = NULL;
  clif::Datastore *_store = NULL;
  std::vector<std::pair<int,int>> indizes;
  
  ProcData _proc;
  
  //TODO more generic model!
  ExtrType _type;
  
  cpath _root; //extrinsics group
};

}

#endif
