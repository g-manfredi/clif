#ifndef _CLIF_CAM_H
#define _CLIF_CAM_H

#include "tree_derived.hpp"
#include "mat.hpp"

namespace clif {

class DepthDist : public Tree_Derived_Base<DepthDist>
{
public:
  DepthDist(const cpath& path, double at_depth);
  
  virtual bool load(Dataset *set);
  virtual bool operator==(const Tree_Derived & rhs) const;
  
  void undistort(const Mat & src, Mat & dst, const Idx & pos = Idx(), int interp = -1);
  
  DistModel type() { return _type; };

private:
  double _depth = -1.0;
  Mat_<cv::Point2f> _maps;
  DistModel _type;
  
  Mat_<cv::Point2f> _ref_point_proj;
};

}

#endif