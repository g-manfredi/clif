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
  
  void undistort(const clif::Mat & src, clif::Mat & dst);

private:
  double _depth = -1.0;
  cv::Mat _map;
  DistModel _type;
};

}

#endif