#ifndef _CLIF_CAM_H
#define _CLIF_CAM_H

#include "mat.hpp"
#include "tree_derived.hpp"

namespace clif {

class DepthDist : public Tree_Derived_Base<DepthDist>
{
public:
  DepthDist(const cpath& path, double at_depth, int w, int h);
  
  virtual bool load(Dataset *set);
  virtual bool operator==(const Tree_Derived & rhs) const;
  
  void undistort(const Mat & src, Mat & dst, const Idx & pos = Idx(), int interp = -1);
  
  DistModel type() { return _type; };
  
  double depth() { return _depth; };

private:
  double _depth = -1.0;
  Mat_<cv::Point2f> _maps;
  Mat_<cv::Point2f> _map;
  DistModel _type;

  int _w, _h;

  cv::Point2d _f;
  cv::Point2d _m;
  double _r;
};

class TD_DistType : public Tree_Derived_Base<TD_DistType>
{
public:
  TD_DistType(const cpath& path);
  
  virtual bool load(Dataset *set);
  virtual bool operator==(const Tree_Derived & rhs) const;
    
  DistModel type() { return _type; };
  bool includesRectification()
  {
    if (_type == DistModel::UCALIB)
      return true;
    return false;
    
  };

private:
  DistModel _type;
};

}

#endif
